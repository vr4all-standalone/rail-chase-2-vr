// SPDX-License-Identifier: GPL-2.0-or-later
#include "../native/credit_policy.h"
#include <cassert>
int main() {
    using namespace rchase2;
    GameState game;game.valid=game.ready=game.pricing_valid=true;game.session=1;game.start_cost={2,2};
    Cabinet request;request.coins=1;NativeCreditPolicy policy;
    auto in=policy.update(0,request,game,true);assert(in.coins==1 && in.starts==0);
    request.coins=0;in=policy.update(.15,request,game,true);assert(in.coins==0 && in.starts==0);
    game.credits=1;in=policy.update(.31,request,game,true);assert(in.coins==1 && in.starts==0);
    game.credits=2;in=policy.update(.45,request,game,true);assert(in.coins==0 && in.starts==1);
    game.players[0].state=1;in=policy.update(.46,request,game,true);assert(in.coins==0 && in.starts==0);
    request.coins=2;game.start_cost[1]=5;in=policy.update(.5,request,game,true);assert(in.coins==2 && in.starts==0);
    in=policy.update(.6,request,game,false);assert(in.coins==0 && in.starts==0);
    game.pricing_valid=false;in=policy.update(.7,request,game,true);assert(in.coins==0 && in.starts==0);
    game.pricing_valid=true;request.coins=0;policy.update(.8,request,game,true);
    request.coins=2;game.credits=0;policy.update(.9,request,game,true);
    request.coins=0;in=policy.update(5,request,game,true);assert(in.coins==0 && in.starts==0);
    in=policy.update(5.31,request,game,true);assert(in.coins==0 && in.starts==0);
    request.coins=2;game.pricing_valid=false;in=policy.update(5.5,request,game,true);assert(in.coins==0 && in.starts==0);
}
