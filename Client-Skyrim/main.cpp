// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "skse\PluginAPI.h"		// super
#include "skse\skse_version.h"	// What version of SKSE is running?
#include "Client-Skyrim.h"
#include "FriendLink-Common\Error.h"
#include "ScriptDragon\skyscript.h"
#include "ScriptDragon\obscript.h"
#include "ScriptDragon\types.h"
#include "ScriptDragon\enums.h"
#include "ScriptDragon\plugin.h"
#include <skse\GameMenus.h>

static PluginHandle					g_pluginHandle = kPluginHandle_Invalid;
static SKSEPapyrusInterface         * g_papyrus = NULL;


extern "C" {

	bool SKSEPlugin_Query(const SKSEInterface * skse, PluginInfo * info) {	// Called by SKSE to learn about this plugin and check that it's safe to load it
		Error::ClearLog();
		Error::LogToFile("FriendLink");

		// populate info structure
		info->infoVersion = PluginInfo::kInfoVersion;
		info->name = "FriendLink";
		info->version = 1;

		// store plugin handle so we can identify ourselves later
		g_pluginHandle = skse->GetPluginHandle();

		if (skse->isEditor)
		{
			Error::LogToFile("loaded in editor, marking as incompatible");
			return false;
		}
		else if (skse->runtimeVersion != RUNTIME_VERSION_1_9_32_0)
		{
			Error::LogToFile("unsupported runtime version " + std::to_string(skse->runtimeVersion));
			return false;
		}

		// ### do not do anything else in this callback
		// ### only fill out PluginInfo and return true/false

		// supported runtime version
		return true;
	}

	bool SKSEPlugin_Load(const SKSEInterface * skse) {	// Called by SKSE to load this plugin
		Error::LogToFile("FriendLink loaded");

		g_papyrus = (SKSEPapyrusInterface *)skse->QueryInterface(kInterface_Papyrus);

		//Check if the function registration was a success...
		bool btest = g_papyrus->Register(FriendLink::RegisterFuncs);

		if (btest) {
			Error::LogToFile("Register Succeeded");
		}

		return true;
	}
}

void main()
{
	ScriptDragon::PrintNote("Friend Link script launched");
	FriendLink::Start();
	while (TRUE)
	{
		ScriptDragon::Wait(0);
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
	{
		ScriptDragon::g_hModule = hModule;
		ScriptDragon::DragonPluginInit(hModule);
		break;
	}
	case DLL_PROCESS_DETACH:
	{
		break;
	}
	}
	return TRUE;
}
/*
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
*/

