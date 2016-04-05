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
#include "SkyPlayer.h"

#include <skse\PapyrusWornObject.h>
#include "FriendLink-Common\Error.h"

using namespace std;

namespace Game {
namespace Player {
Translator::Translator(ObjectReference translator_object_refr)
    : object_refr_(translator_object_refr)  {
  SetEndTime(kStartDelay);
  update_ = thread{&Translator::UpdateLoop, this};
  
}
Translator::~Translator() {
  end_loop = true;
  update_.join();
}
void Translator::To(Location new_location) {
  auto location_packed = new_location.ToProperty();
  future_.Push(location_packed.value);
}
void Translator::Stop() {
  stop_translating = true;
  Wait();
}
void Translator::Start() {
  stop_translating = false;
}
void Translator::UpdateLoop() {
  while(!end_loop) {
    if(!stop_translating) {
      auto location_packed = future_.Pop();
      if(future_) {
        auto future = Location({Property::Type::kLocation, location_packed});
        if(TimeSubtract(future, current_) > 1) {
          FutureToCurrent(future);
        }
      } else {
        this_thread::sleep_for(chrono::milliseconds(1));
      }
    } else {
      this_thread::sleep_for(chrono::milliseconds(1));
    }
  }
}
void Translator::FutureToCurrent(Location future) {
  if(!IsNewCell(future) && CellIsAttached(future)) {
    Translate(future);
  } else {
    ChangeLoadedArea(future);
  }
}
void Translator::ChangeLoadedArea(Location future) {
  int time = TimeSubtract(future, current_);
  Wait();
  current_ = future;
  SetEndTime(time);
  this_thread::sleep_for(chrono::milliseconds(time/5));
  ChangeCellTo({object_refr_}, future);
  this_thread::sleep_for(chrono::milliseconds(time/5));
}
void Translator::Translate(Location future) {
  int time = TimeSubtract(future, current_);
  float distance = DistanceBetween(future, current_);
  if(distance > kMovementThreshold) {
    Wait();
    TranslateTo(object_refr_, future, distance * 1050/ time); //time in seconds
    translating_complete = false;
  }
  current_ = future;
  SetEndTime(time);
}
void Translator::Wait() {
  auto now = chrono::steady_clock::now();
  while(current_end_double_ > now && (current_end_ > now || !translating_complete)) {
    now = chrono::steady_clock::now();
    this_thread::sleep_for(chrono::milliseconds(1));
  }
}
void Translator::SetEndTime(int ms_to_end) {
  using namespace chrono;
  current_end_ = steady_clock::now()
               + (milliseconds(ms_to_end) + milliseconds(kTranslationDelay));
  current_end_double_ = steady_clock::now()
               + (milliseconds(2*ms_to_end) + milliseconds(kTranslationDelay));
}
bool Translator::CellIsAttached(Location location) {
  Cell cell {location};
  return cell && ScriptDragon::Cell::IsAttached(cell);
}
//The idea of the new cell is to not claim a new cell if the cell is loaded and 
//visable. This seems to only happen when you are coming from interior to another
//interior. If this doesn't work try InSameSrea(location, location)
bool Translator::IsNewCell(Location future) {
  Cell future_cell {future};
  Cell current_cell {current_};
  bool is_new_cell = !InSameCell(future, current_) && 
                     ((future_cell && 
                       ScriptDragon::Cell::IsInterior(future_cell)) ||
                      (current_cell && 
                       ScriptDragon::Cell::IsInterior(current_cell)));
  bool is_new_world = !InSameWorldSpace(future, current_);
  return (is_new_world || is_new_cell);
}
Property::Stream Standard::GetProperties(std::vector<Property::Type> types) {
  Property::Stream property_out;
  for (auto type : types) {
    property_out << GetProperty(type);
  }
  return property_out;
}
Property::Property Standard::GetProperty(Property::Type type) {
  switch (type) {
    case Property::Type::ksName:
      return PackString(type, name());
    case Property::Type::kLocation:
      return location().ToProperty();
    case Property::Type::ksCellName:
      return PackString(type, cell().Name());
    case Property::Type::ksWorldSpaceName:
      return PackString(type, world_space().Name());
    case Property::Type::kLoadedState:
      return loaded_states().ToProperty();
    default:
      throw SkyPlayerException("Can't get property type");
  }
}
void Standard::SetProperties(Property::Stream properties_in) {
  for (Property::Property property; properties_in >> property;) {
    switch (property.type) {
      case Property::Type::ksName:
        SetName(UnpackString(property));
        break;
      case Property::Type::kLocation:
        SetLocation(Location{property});
        break;
      case Property::Type::ksCellName:
      case Property::Type::ksWorldSpaceName:
      default:
        break;
    }
  }
}
bool Standard::CompareObjectTo(ObjectReference comparing_object_refr) {
  return object_refr_.Compare(comparing_object_refr);
}
void Standard::SetName(std::string name) {
  if(object_refr_) {
    referenceUtils::SetDisplayName(&(object_refr_->extraData), 
                                   name.c_str(), true);
  }
}
void Standard::SetLocation(Location new_location) {
  ChangeCellTo(object_refr_, new_location);
}
LoadedState Standard::loaded_states() {
  LoadedState state {};
  state.unk00 = object_refr_->unk50;
  state.unk04 = 0;
  state.unk08 = 0;
  state.unk0C = 0;
  state.unk10 = 0;
  state.unk14 = 0;
  state.unk18 = 0;
  state.unk1C = 0;

  return state;
}
std::string Standard::name() {
  if(!object_refr_) return "No name";
  return string{CALL_MEMBER_FN(object_refr_.skse(), GetReferenceName)()};
}
WorldSpace Standard::world_space() {
  if(!object_refr_) return {};
  return CALL_MEMBER_FN(object_refr_.skse(), GetWorldspace)();
}
Cell Standard::cell() {
  if (!object_refr_)  return {};
  return object_refr_->parentCell;
}
Location Standard::location() {
  auto c = cell();
  auto ws = world_space();
  vector<float> pos = {object_refr_->pos.x, 
                       object_refr_->pos.y, 
                       object_refr_->pos.z};
  if (ws && c) {
    return Location{uint32_t{ws->formID}, uint32_t{c->formID}, pos};
  } else if (c) {
    return Location{false, uint32_t{c->formID}, pos};
  } else {
    return Location{false, false,{0, 0, 0}};
  }
}
Traversable::Traversable(UInt32 form_id) : Standard() {
  SetObjectRefr(PlaceAtMe({(TESObjectREFR*)*g_thePlayer}, form_id, true, false));
  translate.reset(new Translator(object_refr()));
}
} //namespace Player
} //namespace Game