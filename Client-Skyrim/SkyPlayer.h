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
*   Includes interfaces to add and manipulate object references in skyrim. This 
*   files contains a direct link to game information and connects the rest of 
*   the solution to the game.
*/
/** @defgroup Skyrim Skyrim */
#pragma once
#include "stdafx.h"
#include <common\IMemPool.h>
#include <skse\GameReferences.h>
#include <skse\Hooks_Threads.h>
#include <skse\GameThreads.h>
#include "ScriptDragon\skyscript.h"
#include "FriendLink-Common\game_structures.h"
#include "FriendLink-Common\Sharing.h"
#include "reference_handler.h"

namespace Game {
namespace Player {
/**
*   Translator allows you to safely translate an object in Skyrim.
*   The skyrim object required that the script FLQTranslatorObject be 
*   attached and be an active client.
*   @ingroup Skyrim
*/
//Look into NiAVObject
class Translator {
  ///Determines how far the object reference has to move before it is considered
  ///moving,
  static constexpr float kMovementThreshold = 5; //From experiments
  ///Delays all translations by this amount of time in milliseconds.
  static constexpr int kTranslationDelay = 0; //milliseconds
  ///Delays all movement at the start by,
  static constexpr int kStartDelay = 100; //milliseconds
 public:
  /**
   *  @param translator_object_refr Has to have the script FLQTranslatorObject
   *         attached and be an active client.
   */
  Translator(ObjectReference translator_object_refr);
  ~Translator();
  void To(Location new_location);
  /**
   *  Marks when the translation is completed. Currently called from papyrus.
   */
  void MarkComplete() { translating_complete = true; }
  /**
   *  Waits for the current translation to finish then halts all new translation.
   */
  void Stop();
  /**
   *  Starts the translations after calling stop.
   */
  void Start();
 private:
  void UpdateLoop();
  void FutureToCurrent(Location future);
  void ChangeLoadedArea(Location future);
  void Translate(Location future);
  void Wait();
  void SetEndTime(int ms_to_end);
  bool CellIsAttached(Location);
  bool IsNewCell(Location);
   

  std::chrono::time_point<std::chrono::steady_clock> current_end_;
  std::chrono::time_point<std::chrono::steady_clock> current_end_double_;
  std::thread update_;
  ObjectReference object_refr_;
  Location current_{false,false,{0,0,0}};
  Data::Sharing::FixedQueue future_{};
  std::atomic<bool> end_loop = false;
  std::atomic<bool> stop_translating = false;
  std::atomic<bool> translating_complete = true;
};

/**
*   SkyPlayer is an interface to manage object references that represents
*   players in Skyrim.
*   SkyPlayer either creates a new object by a form id or
*   gets the current local player reference. SkyPlayer then allows easy access
*   to get and set important properties of the object reference.
*   @ingroup Skyrim
*/
class Standard {
 public:
  Standard(ObjectReference object_refr) : object_refr_(object_refr) {}
  Property::Stream GetProperties(std::vector<Property::Type>);
  Property::Property GetProperty(Property::Type);
  void SetProperties(Property::Stream properties_in);
  bool CompareObjectTo(ObjectReference comparing_object_refr);
 protected:
   /** Give SkyPlayer object control over the local player character.
   *   @remark Deleting SkyPlayer after using this constructor will not delete
   *           the local player.
   */
  Standard() : object_refr_() {}
  ObjectReference object_refr() { return object_refr_; }
  void SetObjectRefr(ObjectReference object_refr) 
      { object_refr_ = object_refr; }
  /**
   *  Sets the name of the object. Called in set properties.
   */
  virtual void SetName(std::string);
  /**
   *  Sets the location of the object. Called in set properties.
   */ 
  virtual void SetLocation(Location);
  
 private:
  LoadedState loaded_states();
  std::string name();
  WorldSpace world_space();
  Cell cell();
  Location location();

  ObjectReference object_refr_;
};
/**
 *   Extends Player.Standard for the local player in the game.
 *   @ingroup Skyrim
 */
class Local : public Standard {
  public:
    Local() : Standard() { SetObjectRefr({(TESObjectREFR*)*g_thePlayer}); }
};
/**
 *  Extends Player.Standard for traversable objects in the game. A traversable
 *  is an object that has the script FLQTranslatorObject attached.
 *  @ingroup Skyrim
 */
class Traversable : public Standard {
 public:
  /**
   *  @param translator_object_refr Has to have the script FLQTranslatorObject
   *         attached and be an active client.
   */
  Traversable(UInt32 translator_object_refr);
  ~Traversable();
  void MarkTranslationComplete() { translate->MarkComplete(); }

 private:
  void SetLocation(Location location) { translate->To(location); }
  std::unique_ptr<Translator> translate;
};
class SkyPlayerException : public std::runtime_error {
 public:
  SkyPlayerException(std::string message) : std::runtime_error(message) {}
};
}//namespace Player
}//namespace Game