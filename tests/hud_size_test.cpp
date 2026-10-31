#ifdef NDEBUG
#undef NDEBUG
#endif
#include "../native/settings_io.h"
#include "../native/hud_size.h"
#include <cassert>
#include <sstream>
#include <iostream>
using namespace rchase2;
int main(){
 assert(Settings{}.hud_size==1);
 for(const char* invalid:{"","hud_size=nan\n","hud_size=inf\n","hud_size=0.49\n","hud_size=2.01\n","hud_size=1garbage\n"}){std::istringstream in(invalid);assert(read_settings(in).hud_size==1);}
 for(float size:{.5f,1.f,2.f}){Settings s;s.hud_size=size;std::ostringstream out;write_settings(out,s);std::istringstream in(out.str());auto loaded=read_settings(in);assert(loaded.hud_size==size);assert(effective_hud_size(size,true,true,false,true)==size);for(int gate=0;gate<4;gate++)assert(effective_hud_size(size,gate!=0,gate!=1,gate==2,gate!=3)==1);s.p1_hand=1;assert(s.hud_size==size);s=Settings{};std::ostringstream reset;write_settings(reset,s);std::istringstream re(reset.str());assert(read_settings(re).hud_size==1);}
 assert(effective_hud_size(NAN,true,true,false,true)==1);assert(effective_hud_size(INFINITY,true,true,false,true)==1);
 std::cout<<"HUD persistence, bounds, reset, hand independence and render gates pass\n";
}
