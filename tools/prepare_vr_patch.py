"""Generate pinned continuous Model 2 VR host hooks after diagnostic patch."""
import argparse,difflib,subprocess
from pathlib import Path
from prepare_capture_patch import COMMIT,modify as capture_modify

def replace(s,a,b):
    if s.count(a)!=1:raise ValueError('Expected one hook: '+a[:100]+' count='+str(s.count(a)))
    return s.replace(a,b)

def video(s):
    s=replace(s,'#include "../../lib/rchase2/model2_trace.h"','#include "../../lib/rchase2/model2_trace.h"\n#include "../../lib/rchase2/model2_live.h"')
    s=replace(s,'rchase2::trace_begin(m_screen->frame_number(), machine().time().as_double());','rchase2::trace_begin(m_screen->frame_number(), machine().time().as_double());\n        rchase2::live_begin(m_screen->frame_number(), machine().time().as_double());')
    s=replace(s,'if (rchase2::trace_active()) {\n        rchase2::NativePolygon','if (rchase2::trace_active() || rchase2::live_active()) {\n        rchase2::NativePolygon')
    s=replace(s,'        rchase2::trace_capture().append(native);','        if(rchase2::trace_active())rchase2::trace_capture().append(native);\n        if(rchase2::live_active())rchase2::live_capture().append(native);')
    s=s.replace('if(rchase2::trace_active())rchase2::trace_object()','if(rchase2::trace_active()||rchase2::live_active())rchase2::trace_object()')
    anchor='\t/* tell the rasterizer we\'re starting a frame */'
    s=replace(s,anchor,'''    if(rchase2::live_pending()) {
        auto& capture=rchase2::live_capture();
        const int width=m_screen->visible_area().width(),height=m_screen->visible_area().height();
        capture.display(m_crtc_xoffset,m_crtc_yoffset,width,height);
        capture.bytes("texture0",&m_textureram0[0],m_textureram0.bytes());
        capture.bytes("texture1",&m_textureram1[0],m_textureram1.bytes());
        capture.bytes("palette",m_palram.get(),0x4000);
        capture.bytes("colorxlat",m_colorxlat.get(),0xc000);
        capture.bytes("luma",m_lumaram.get(),0x8000);
        capture.bytes("gamma",m_gamma_table,sizeof(m_gamma_table));
        auto frame=std::make_unique<rchase2::NativeFrame>(capture.take());
        frame->complete=false;
        rchase2::copy_pixels(frame->background,width,height,[&](int x,int y){return bitmap.pix(y,x);});
        rchase2::live_raster()=std::move(frame);rchase2::live_pending()=false;
    }
'''+anchor)
    anchor='\tcopybitmap_trans(bitmap, m_sys24_bitmap, 0, 0, 0, 0, cliprect, 0);\n    if(!rchase2::trace_pending_raster().empty())'
    s=replace(s,anchor,anchor.split('\n')[0]+'''
    if(auto& frame=rchase2::live_raster();frame) {
        rchase2::copy_pixels(frame->foreground,frame->width,frame->height,[&](int x,int y){return m_sys24_bitmap.pix(y,x);});
        rchase2::copy_pixels(frame->native,frame->width,frame->height,[&](int x,int y){return bitmap.pix(y,x);});
        frame->complete=true;
        rchase2::shared_runtime().scene.store(std::shared_ptr<const rchase2::NativeFrame>(frame.release()));
    }
    if(!rchase2::trace_pending_raster().empty())''')
    anchor='    if(rchase2::trace_active()) {\n        rchase2::trace_geometry_pending()=true;'
    s=replace(s,anchor,'    if(rchase2::live_active()){rchase2::live_pending()=true;rchase2::live_active()=false;}\n'+anchor)
    return s

def driver(s):
    s=replace(s,'#include "model2.h"','#include "model2.h"\n#include "../../lib/rchase2/host_bridge.h"')
    s=replace(s,'u8 model2_state::in0_r()\n{\n\tu8 data = m_in0->read();','u8 model2_state::in0_r()\n{\n\tu8 data = rchase2::system_input(m_in0->read());')
    s=replace(s,'\tm_framenum = m_screen->frame_number();','    rchase2::native_tick([&](uint32_t address){return m_maincpu->space(AS_PROGRAM).read_dword(address);});\n\tm_framenum = m_screen->frame_number();')
    begin=s.index('void model2b_state::rchase2(machine_config &config)')
    end=s.index('\nvoid model2b_state::gunblade',begin)
    old=s[begin:end];new=old
    for channel,tag in enumerate(['P2_X','P1_X','P2_Y','P1_Y']):
        a=f'io.an_port_callback<{channel}>().set_ioport("{tag}");'
        b=f'io.an_port_callback<{channel}>().set([this](){{return rchase2::analog_input({channel},ioport("{tag}")->read());}});'
        new=replace(new,a,b)
    new=new.replace('\n}', '\n    io.in_pc_callback().set([this](){return rchase2::trigger_input(ioport("IN1")->read());});\n}')
    return replace(s,old,new)

def osd(s):
    s=replace(s,'#include "emu.h"','#include "emu.h"\n#include "rchase2/host_bridge.h"')
    s=replace(s,'void osd_common_t::update(bool skip_redraw)\n{','void osd_common_t::update(bool skip_redraw)\n{\n    rchase2::poll_host(machine());')
    s=replace(s,'void osd_common_t::osd_exit()\n{','void osd_common_t::osd_exit()\n{\n    rchase2::stop_host();')
    return s

def generate(source):
    patch=''
    for rel,fn,base in [('src/mame/sega/model2_v.cpp',video,capture_modify),('src/mame/sega/model2.cpp',driver,lambda s:s),('src/osd/modules/lib/osdobj_common.cpp',osd,lambda s:s)]:
        old=base(subprocess.check_output(['git','-C',str(source),'show',f'{COMMIT}:{rel}'],text=True))
        new=fn(old)
        patch+=''.join(difflib.unified_diff(old.splitlines(True),new.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel))
    return patch
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--source',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    args.output.write_text(generate(args.source),encoding='utf8',newline='\n')
