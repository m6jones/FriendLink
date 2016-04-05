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
#include "Client-Skyrim.h"

#include <skse\PapyrusQuest.h>
#include <skse\Hooks_SaveLoad.h>

#include "FriendLink-Common\NetworkExceptions.h"
#include "FriendLink-Common\Error.h"
#include "SkyServerLink.h"

using namespace std;

namespace FriendLink {
namespace {
Network::SkyServerLink* g_server = nullptr;
bool g_reconnect = false;
} //namespace

void Start() {
  auto quest = (ScriptDragon::TESQuest*)papyrusQuest::GetQuest({}, kQuest);
	if (g_reconnect) {
		ScriptDragon::Quest::SetCurrentStageID(quest, 10);
		g_reconnect = false;
	} else {
		ScriptDragon::Quest::SetCurrentStageID(quest, 0);
	}
}
void PreLoadGame() {
  g_reconnect =  (g_server != nullptr);
	if (g_reconnect) {
    Disconnect({});
	}
}
bool Connect(StaticFunctionTag*, UInt32 formID) {
	try {
		auto ip__ports = Network::GetIpAndPorts(kAddressFile);
    g_server = new Network::SkyServerLink(ip__ports[0], //ip
                                          ip__ports[1], //port
                                          ip__ports[2], //udp port
                                          formID);
	} catch (Network::NetworkException e) {
		Error::LogToFile(e.what(), e.code(), e.message());
    g_server = nullptr;
		return false;
	} catch (runtime_error e) {
		Error::LogToFile(e.what());
    g_server = nullptr;
		return false;
	} catch (...) {
		Error::LogToFile("Unknown Error");
    g_server = nullptr;
		return false;
	}

	return true;
}
void StartDataTransfer(StaticFunctionTag *base) {
	if (g_server) {
    g_server->StartDataTransfer();
	}
}
UInt32 my_server_slot(StaticFunctionTag *base) {
	if (g_server) {
		return g_server->my_server_slot();
	}
	return 0;
}
UInt32 max_players(StaticFunctionTag *base) {
	if (g_server) {
		return g_server->max_players();
	}
	return 0;
}
void Disconnect(StaticFunctionTag *base) {
	if (g_server) {
    delete g_server;
    g_server = nullptr;
	}
}
bool IsConnected(StaticFunctionTag *base) {
	if (g_server) {
		return true;
	}
	return false;
}
bool ObjectInUse(TESObjectREFR* objRef) {
	if (!g_server || !objRef) {
		return false;
	}
	return g_server->IsObjectAPlayer(objRef);
}
void MarkTranslatingComplete(TESObjectREFR* objRef) {
  if (!g_server || !objRef) {
    return;
  }
  g_server->MarkPlayerTranslatingComplete(objRef);
}
bool RegisterFuncs(VMClassRegistry* registry) {
		
	registry->RegisterFunction(
		new NativeFunction1 <StaticFunctionTag, bool, UInt32>(
      "FLConnect", "FriendLinkScript", 
      FriendLink::Connect, registry));
	registry->RegisterFunction(
		new NativeFunction0 <StaticFunctionTag, void>(
       "FLStartDataTransfer", "FriendLinkScript", 
       FriendLink::StartDataTransfer, registry));
	registry->RegisterFunction(
		new NativeFunction0 <StaticFunctionTag, UInt32>(
        "FLMyServerSlot", "FriendLinkScript", 
        FriendLink::my_server_slot, registry));
	registry->RegisterFunction(
		new NativeFunction0 <StaticFunctionTag, void>(
        "FLDisconnect", "FriendLinkScript", 
        FriendLink::Disconnect, registry));
	registry->RegisterFunction(
		new NativeFunction0 <StaticFunctionTag, UInt32>(
        "FLMaxPlayers", "FriendLinkScript", 
        FriendLink::max_players, registry));
	registry->RegisterFunction(
		new NativeFunction0 <StaticFunctionTag, bool>(
        "FLIsConnected", "FriendLinkScript", 
        FriendLink::IsConnected, registry));
	registry->RegisterFunction(
		new NativeFunction0 <TESObjectREFR, bool>(
        "FLObjectInUse", "FLQDeleteOnNoConnection",
        FriendLink::ObjectInUse, registry));
  registry->RegisterFunction(
    new NativeFunction0 <TESObjectREFR, void>(
        "FLMarkTranslatingComplete", "FLQTranslatorObject", 
        FriendLink::MarkTranslatingComplete, registry));
  //adds the preload hook.
	Hooks_SaveLoad_Commit();
	return true;
}
}//namespace FriendLink
