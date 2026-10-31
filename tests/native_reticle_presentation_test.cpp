// SPDX-License-Identifier: GPL-2.0-or-later
#include "../native/presentation.h"
#include "../native/gl_context.h"
#include <cassert>
#include <iostream>
using namespace rchase2;
int main(){
    GLContext context;Presentation ui;PresentationInput input;Settings settings;SharedRuntime runtime;GameState game;
    game.valid=game.playing=game.ready=true;for(auto& p:game.players){p.state=1;p.health=1;}
    for(auto& h:input.hands)h.tracked=true;input.aim_targets={Vec3{-1,0,-2},Vec3{1,0,-4}};
    ui.update(input,settings,runtime,0);CabinetInput cabinet;cabinet.hand={0,1};ui.feedback(cabinet,0);
    assert(!ui.reticle_targets(game)[0]); // Initial controller help suppresses extraction.
    cabinet.cabinet.starts=1;ui.feedback(cabinet,.1);auto r=ui.reticle_targets(game);assert(r[0]&&r[0]->x==-1&&r[1]&&r[1]->x==1);
    cabinet.hand={1,0};ui.feedback(cabinet,.2);r=ui.reticle_targets(game);assert(r[0]&&r[0]->x==1&&r[1]&&r[1]->x==-1);
    game.players[1]={};cabinet.hand={0,-1};ui.feedback(cabinet,.3);r=ui.reticle_targets(game);assert(r[0]&&r[0]->x==-1&&!r[1]);
    cabinet.hand={1,-1};ui.feedback(cabinet,.4);r=ui.reticle_targets(game);assert(r[0]&&r[0]->x==1&&!r[1]); // Solo hand transfer keeps P1 identity.
    input.hands[1].tracked=false;ui.update(input,settings,runtime,.5);assert(!ui.reticle_targets(game)[0]);input.hands[1].tracked=true;
    input.aim_targets[1]=Vec3{NAN,0,-2};ui.update(input,settings,runtime,.6);assert(!ui.reticle_targets(game)[0]);input.aim_targets[1]=Vec3{1,0,-4};
    input.focused=false;ui.update(input,settings,runtime,.7);assert(!ui.reticle_targets(game)[0]);input.focused=true;
    ui.update(input,settings,runtime,.8);input.toggle=true;ui.update(input,settings,runtime,.9);assert(ui.open()&&!ui.reticle_targets(game)[0]);
    input.toggle=false;ui.update(input,settings,runtime,1);input.toggle=true;ui.update(input,settings,runtime,1.1);assert(!ui.open()&&ui.reticle_targets(game)[0]);
    game.service=true;assert(!ui.reticle_targets(game)[0]);game.service=false;game.valid=false;assert(!ui.reticle_targets(game)[0]);game.valid=true;
    game.players[1].state=1;game.players[1].health=1;cabinet.hand={1,1};ui.feedback(cabinet,1.2);r=ui.reticle_targets(game);assert(!r[0]&&!r[1]);
    ui.suspend();assert(!ui.reticle_targets(game)[0]);
    std::cout<<"Presentation reticles preserve native identity across hands, help, focus, menu and service\n";
}
