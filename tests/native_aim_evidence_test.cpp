// SPDX-License-Identifier: GPL-2.0-or-later
#include "../native/game_state.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <set>
#include <tuple>
int main(int argc,char** argv) {
    using namespace rchase2;
    if(argc!=2)return 2;
    std::ifstream in(argv[1]);std::string line;std::getline(in,line);
    unsigned count{};std::set<std::tuple<int,int,int>> distinct;double maximum{};
    std::array<int,4> previous{-1,-1,-1,-1};
    while(std::getline(in,line)) {
        std::istringstream row(line);double time;std::array<int,4> raw;std::array<double,4> world;
        if(!(row>>time>>raw[0]>>raw[1]>>raw[2]>>raw[3]>>world[0]>>world[1]>>world[2]>>world[3]))return 3;
        // Native output belongs to the preceding ADC latch; keep only a stable
        // second read of each sample, rather than assuming callback ordering.
        const bool stable=raw==previous;previous=raw;if(!stable)continue;
        for(unsigned player=0;player<2;player++) {
            NativeAimCalibration c;c.valid=c.screen_inverse_valid=true;c.player=player;c.left=c.top=192;c.right=c.bottom=64;
            const auto p=native_aim_point(c,raw[player*2],raw[player*2+1]);
            const double error=std::max(std::abs(p.x-world[player*2]),std::abs(p.y-world[player*2+1]));
            maximum=std::max(maximum,error);if(!p.valid||error>1e-6)return 4;
            const auto inverse=native_screen_to_adc(player,(248+128*p.x)/495.,(192-128*p.y)/383.,c);
            if(!inverse.valid||inverse.x!=raw[player*2]||inverse.y!=raw[player*2+1])return 5;
            distinct.insert({player,raw[player*2],raw[player*2+1]});++count;
        }
    }
    if(count<100||distinct.size()<50)return 6;
    std::cout<<count<<" native samples, "<<distinct.size()<<" distinct player/ADC positions, maximum world-coordinate error "<<maximum<<"; exact ADC inverse round trips\n";
}
