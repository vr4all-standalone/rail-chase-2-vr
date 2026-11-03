// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <array>
namespace rchase2 {
class MenuPointer {
 int owner_{-1};bool open_{};std::array<bool,2> released_{},previous_{};
public:
 struct Result {int owner{-1};bool activate{},changed{};};
 int owner() const{return owner_;}
 void suspend(){released_={};previous_={};open_=false;}
 Result update(bool open,bool focused,const std::array<bool,2>& tracked,const std::array<bool,2>& triggers,int fallback){
  if(!open||!focused){suspend();return {owner_,false,false};}
  bool entering=!open_;open_=true;int before=owner_;
  if(owner_<0||!tracked[owner_])owner_=(fallback>=0&&fallback<2&&tracked[fallback])?fallback:tracked[0]?0:tracked[1]?1:-1;
  std::array<bool,2> edge{};
  for(int h=0;h<2;h++){
   if(!tracked[h]){released_[h]=false;previous_[h]=false;continue;}
   if(!triggers[h])released_[h]=true;
   edge[h]=!entering&&released_[h]&&triggers[h]&&!previous_[h];previous_[h]=triggers[h];
  }
  if(edge[0]!=edge[1])owner_=edge[0]?0:1;
  const bool changed=owner_!=before;
  return {owner_,owner_>=0&&edge[owner_]&&!changed,changed};
 }
};
}
