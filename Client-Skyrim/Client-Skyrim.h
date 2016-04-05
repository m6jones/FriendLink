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
 *  Directly connects the plugin into skyrim. 
 */
#pragma once
#include "stdafx.h"
#include "skse/PapyrusNativeFunctions.h"
#include "skse/GameReferences.h"

namespace FriendLink {
constexpr auto kQuest = "FLQ01";
constexpr auto kAddressFile = "FriendLinkIP.cfg";
/**
 *  This is called when the game loads for the first time. The time this is
 *  is the same time script dragon loads the plugin. Resets the state of the
 *  connection to disconnected state.
 */
void Start();
/**
 *  This is called before the game loads a new save.
 */
void PreLoadGame();
/**
 *  Connect to server.
 *  @param form_id The id of a form with the FLQTranslatorObject script 
 *                 attached.
 *  @return True if the connection was successful.
 */
bool Connect(StaticFunctionTag*, UInt32 form_id);
/**
 *  Starts data transfer between server and client.
 *  @todo This might not be necessary anymore. 
 */
void StartDataTransfer(StaticFunctionTag *base);
/**
 *  Gets server slot for the local client.
 *  @return Slot of local client. If not connected 0 is returned
 */
UInt32 my_server_slot(StaticFunctionTag *base);
/**
 *  Gets max number of players that can be on the server.
 *  @return Max number of players that can be on the server. If not connected 0 
 *          is returned
 */
UInt32 max_players(StaticFunctionTag *base);
void Disconnect(StaticFunctionTag *base);
bool IsConnected(StaticFunctionTag *base);
/**
 *  Checks to see if object is a representation of a player.
 *  @param object A skyrim object that will be compared against the player
 *                objects in players.
 *  @return True if this object is representing a current player.
 */
bool ObjectInUse(TESObjectREFR* actRef);
/**
 *  Marks translating has been complete for object reference.
 */
void MarkTranslatingComplete(TESObjectREFR* actRef);
/**
 *  Registers functions in papyrus as well as registering hooks.
 */
bool RegisterFuncs(VMClassRegistry* registry);
}//namespace FriendLink