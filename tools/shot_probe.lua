-- Rail Chase 2 Revision A research only. Observe original program writes;
-- return no replacement data and never write RAM or program memory.
local root=assert(os.getenv('RCHASE2_RESEARCH_DIR'))
local machine=manager.machine
local cpu=assert(machine.devices[':maincpu'])
local program=cpu.spaces.program
local ports=machine.ioport.ports
local events=assert(io.open(root..'/projectile-writes.tsv','w'))
local frames=assert(io.open(root..'/shot-frames.tsv','w'))
events:write('time\tip\taddress\tdata\tmask\towner\towner_callback\tchild_callback\tmode\tactive\towner_child\tchild_owner\n')
frames:write('time\tmode\tsubmode\tactive\tinput\thealth1\thealth2\tgate\towner\tchild\ttrigger1\ttrigger2\n')
local function read(address) return program:read_u32(address) end
local function ram_pointer(address) return address>=0x500000 and address<=0x5fff00 and address%4==0 end
local function callback(address) return ram_pointer(address) and read(address+4) or 0 end
local function set(tag,mask,down)
    for _,field in pairs(assert(ports[tag]).fields) do
        if field.mask==mask then field:set_value(down and 1 or 0);return end
    end
    error('Missing cabinet field '..tag..' '..mask)
end
local function during(t,a,b) return t>=a and t<b end
rchase2_projectile_tap=program:install_write_tap(0x524990,0x524997,'rchase2_projectile_research',function(address,data,mask)
    local owner=address==0x524990 and data or read(0x524990)
    local child=address==0x524994 and data or read(0x524994)
    events:write(string.format('%.9f\t%08x\t%08x\t%08x\t%08x\t%08x\t%08x\t%08x\t%d\t%d\t%08x\t%08x\n',
        machine.time:as_double(),cpu.state['ip'].value,address,data,mask,owner,callback(owner),callback(child),read(0x501d84),read(0x501d88)&3,
        ram_pointer(owner) and read(owner+0x10) or 0,ram_pointer(child) and read(child+0x14) or 0))
    -- nil preserves the exact original native write.
end)
local previous=-1
rchase2_shot_frames=emu.add_machine_frame_notifier(function()
    local t=machine.time:as_double()
    local fire1=during(t,31,33) or during(t,34,36) or during(t,43,47) or during(t,51,53) or during(t,54,55) or during(t,68,70)
    local fire2=during(t,45,49) or during(t,51,53) or during(t,56,57) or during(t,68,70)
    set(':IN0',1,during(t,18,18.15) or during(t,19,19.15))
    set(':IN0',2,during(t,38,38.15) or during(t,39,39.15))
    set(':IN0',0x10,during(t,20,20.25))
    set(':IN0',0x20,during(t,40,40.25))
    set(':IN0',4,during(t,65,65.2))
    set(':IN1',1,fire1);set(':IN1',2,fire2)
    frames:write(string.format('%.9f\t%d\t%d\t%d\t%08x\t%d\t%d\t%d\t%08x\t%08x\t%d\t%d\n',
        t,read(0x501d84),read(0x501d90),read(0x501d88)&3,read(0x501d3c),
        program:read_i16(0x501e70),program:read_i16(0x501e72),read(0x501f2c),read(0x524990),read(0x524994),fire1 and 1 or 0,fire2 and 1 or 0))
    local second=math.floor(t)
    if second~=previous then
        previous=second;events:flush();frames:flush()
        if second==20 or second==32 or second==42 or second==46 or second==58 or second==67 then
            for _,screen in pairs(machine.screens) do screen:snapshot(root..string.format('/frame-%03d.png',second));break end
        end
    end
end)
