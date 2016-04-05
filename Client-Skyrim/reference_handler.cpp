/*
The MIT License (MIT)

Copyright (c) 2016 Matthew Jones

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include "reference_handler.h"
#include "skse\GameExtraData.h"

namespace Game {
ObjectReference::ObjectReference(TESObjectREFR* object_refr) 
    : object_refr_(object_refr) {
  if(!object_refr) return;

  object_refr_->handleRefObject.IncRef();
}
ObjectReference::ObjectReference(ScriptDragon::TESObjectREFR* object_refr)
  : object_refr_((TESObjectREFR*)object_refr) {
  if (!object_refr) return;

  object_refr_->handleRefObject.IncRef();
}
ObjectReference::ObjectReference(ObjectReference& object_refr)
  : object_refr_(object_refr.object_refr_) {
  if (!object_refr) return;

  object_refr_->handleRefObject.IncRef();
}
ObjectReference::ObjectReference(ObjectReference&& object_refr)
  : object_refr_(object_refr.object_refr_) {
  object_refr.object_refr_ = nullptr;
}
ObjectReference& ObjectReference::operator=(ObjectReference& object_refr) {
  Remove();
  object_refr_ = object_refr.object_refr_;
  object_refr_->handleRefObject.IncRef();

  return *this;
}
ObjectReference& ObjectReference::operator=(ObjectReference&& object_refr) {
  Remove();
  object_refr_ = object_refr.object_refr_;
  object_refr.object_refr_ = nullptr;

  return *this;
}
ObjectReference::operator bool() {
  return IsValid();
}
//Incase the game deletes the object on it's own.
bool ObjectReference::IsValid() {
  if(object_refr_ == nullptr) {
    return false;
  } else if(object_refr_->handleRefObject.GetRefCount() <= 0) {
    Remove();
    return false;
  } else {
    return true;
  }
}
bool ObjectReference::Compare(ObjectReference comparing_object_refr) {
  return comparing_object_refr == object_refr_;
}
void ObjectReference::Reset(TESObjectREFR* object_refr) {
  Remove();
  object_refr_ = object_refr;
  if(object_refr_) {
    object_refr_->handleRefObject.IncRef();
  }
}
void ObjectReference::Reset(ScriptDragon::TESObjectREFR* object_refr) {
  Remove();
  object_refr_ = (TESObjectREFR*)object_refr;
  if (object_refr_) {
    object_refr_->handleRefObject.IncRef();
  }
}
void ObjectReference::Remove() {
  if(object_refr_ != nullptr) object_refr_->handleRefObject.DecRefHandle();
  object_refr_ = nullptr;
}
Cell::Cell(Location location) {
  if (location.has_cell()) {
    auto cell_form = LookupFormByID(location.cell_id());
    if(cell_form->GetFormType() == kFormType_Cell) {
      cell_ = static_cast<TESObjectCELL*>(cell_form);
    }
  }
}
std::string Cell::Name() {
  if(*this) {
    return cell_->fullName.GetName();
  } else {
    return "No cell";
  }
}
WorldSpace::WorldSpace(Location location) {
  if (location.has_world_space()) {
    auto world_space_form = LookupFormByID(location.world_space_id());
    if (world_space_form->GetFormType() == kFormType_WorldSpace) {
      world_space_ = static_cast<TESWorldSpace*>(world_space_form);
    }
  }
}
std::string WorldSpace::Name() {
  if (*this) {
    return world_space_->fullName.GetName();
  } else {
    return "No World Space";
  }
}

ObjectReference PlaceAtMe(ObjectReference target,
                            UInt32 form_id,
                            bool force_presist,
                            bool initially_disabled) {
  if(target) {
    auto form = ScriptDragon::Game::GetFormById(form_id);
    return ScriptDragon::ObjectReference::PlaceAtMe(target,
                                                    form, 1,
                                                    force_presist,
                                                    initially_disabled);
  } else {
    return {};
  }
}
void TranslateTo(ObjectReference& object_refr,
                 Location location,
                 float speed) {
  if (object_refr && !location.is_empty()) {
    auto rot = object_refr->rot;
    ScriptDragon::ObjectReference::TranslateTo(object_refr,
                                               location.x(),
                                               location.y(),
                                               location.z(),
                                               rot.x,
                                               rot.y,
                                               rot.z,
                                               speed, 0);
  }
}
void ChangeCellTo(ObjectReference& object_refr, Location new_location) {
  UInt32 nullHandle = *g_invalidRefHandle;
  NiPoint3 pos{new_location.x(),
               new_location.y(),
               new_location.z()};
  if(object_refr && !new_location.is_empty()) {
    MoveRefrToPosition(object_refr, &nullHandle,
                       Cell{new_location}.skse(),
                       WorldSpace{new_location}.skse(),
                       &pos, &object_refr->rot);
  }
}
bool IsGameOn() {
  if ((g_thePlayer) && (*g_thePlayer)) {
    ObjectReference thePlayer {(TESObjectREFR*)(*g_thePlayer)};
    return thePlayer.IsValid();
  }
  return false;
}
}
