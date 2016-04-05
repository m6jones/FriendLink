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
/** @file
*   This file defines objects that allows you to work with skse 
*   and script dragon at the same time.
*/
/** @defgroup Skyrim Skyrim */
#pragma once
#include "stdafx.h"
#include "ScriptDragon\skyscript.h"
#include <skse\GameReferences.h>
#include "FriendLink-Common\game_structures.h"
namespace Game {
/**
*   Manages object references and easily converts between skse and script 
*   dragon. The default operators * and -> use the skse TESObjectREFR.
*   @ingroup Skyrim
*/
class ObjectReference {
 public:
  ObjectReference() {}
  ObjectReference(TESObjectREFR*);
  ObjectReference(ScriptDragon::TESObjectREFR*);
  ObjectReference(ObjectReference&);
  ObjectReference(ObjectReference&&);
  ~ObjectReference() { Remove(); };

  ObjectReference& operator=(ObjectReference&);
  ObjectReference& operator=(ObjectReference&&);
  TESObjectREFR operator*() { return *skse(); }
  TESObjectREFR* operator->() { return skse(); }
  operator bool();
  operator TESObjectREFR*() { return skse(); }
  operator ScriptDragon::TESObjectREFR*() { return dragon(); }
  
  bool IsValid();
  bool Compare(ObjectReference comparing_object_refr);
  void Reset(TESObjectREFR*);
  void Reset(ScriptDragon::TESObjectREFR*);
  TESObjectREFR* skse() { return object_refr_; }
  ScriptDragon::TESObjectREFR* dragon() { 
    return (ScriptDragon::TESObjectREFR*)object_refr_; 
  }

 private:
  void Remove();
  TESObjectREFR* object_refr_ = nullptr;
};

/**
*   Manages cell forms and will easily converts between skse and script
*   dragon. The default operators * and -> use the skse TESObjectCELL.
*   @ingroup Skyrim
*/
class Cell {
 public:
  Cell(){}
  explicit Cell(Location);
  Cell(TESObjectCELL* cell) : cell_(cell) {}
  Cell(ScriptDragon::TESObjectCELL* cell) : cell_((TESObjectCELL*)cell) {}
  Cell(Cell&) = default;
  Cell(Cell&&) = default;

  TESObjectCELL operator*() { return *skse(); }
  TESObjectCELL* operator->() { return skse(); }
  operator bool() { return cell_ != nullptr; }
  operator TESObjectCELL*() { return skse(); }
  operator ScriptDragon::TESObjectCELL*() { return dragon(); }
  
  std::string Name();
  TESObjectCELL* skse() { return cell_; }
  ScriptDragon::TESObjectCELL* dragon() {
    return (ScriptDragon::TESObjectCELL*)cell_;
  }
 private:
  TESObjectCELL* cell_ = nullptr;
};
/**
*   Manages world space forms and will easily converts between skse and script
*   dragon. The default operators * and -> use the skse TESWorldSpace.
*   @ingroup Skyrim
*/
class WorldSpace {
public:
  WorldSpace() {}
  explicit WorldSpace(Location);
  WorldSpace(TESWorldSpace* world_space) : world_space_(world_space) {}
  WorldSpace(ScriptDragon::TESWorldSpace* world_space) 
    : world_space_((TESWorldSpace*)world_space) {}
  WorldSpace(WorldSpace&) = default;
  WorldSpace(WorldSpace&&) = default;

  TESWorldSpace operator*() { return *skse(); }
  TESWorldSpace* operator->() { return skse(); }
  operator bool() { return world_space_ != nullptr; }
  operator TESWorldSpace*() { return skse(); }
  operator ScriptDragon::TESWorldSpace*() { return dragon(); }

  std::string Name();
  TESWorldSpace* skse() { return world_space_; }
  ScriptDragon::TESWorldSpace* dragon() {
    return (ScriptDragon::TESWorldSpace*)world_space_;
  }
private:
  TESWorldSpace* world_space_ = nullptr;
};

/**
 *  Places a new object at target with the form represented by form_id.
 *  @param target places the new object at this object reference. If the target
 *         is not valid then the new object does not get created.
 *  @param form_id the id of the form that the new object is based on.
 *  @param force_presist Whether the new object is presistent.
 *  @param initally_disabled Whether this object start off disabled.
 *  @return Object reference of the newly created object. If the target is not
 *          valid then an empty object reference is returned. 
 */
ObjectReference PlaceAtMe(ObjectReference target,
                            UInt32 form_id, 
                            bool force_presist, 
                            bool initially_disabled);
/**
 *  Teleports the object reference to the new location if valid.
 *  @remark This enables/disables and reconnects the object to the cell when
 *          used. So the object will flicker if moved close to it's original
 *          location.
 *  @remark If used in rapid succession then the game will have a chance to
 *          crash.
 */
void SetPosition(ObjectReference&, Location);
/**
 *  Translate the object reference to the new location if valid.
 *  @param speed The speed of which the object reference will translate. In
 *         units per second.
 *  @remark If used in rapid succession then the game will have a chance to
 *          crash.
 */
void TranslateTo(ObjectReference&, Location, float speed);
/**
 *  Teleports the object reference to the new location if valid.
 *  @param speed The speed of which the object reference will translate. In
 *         units per second.
 *  @remark This enables/disables and reconnects the object to the cell when
 *          used. So the object will flicker if moved close to it's original
 *          location.
 */
void ChangeCellTo(ObjectReference&, Location);
/**
 *  Checks to see if the game is currently loaded.
 */
bool IsGameOn();
}