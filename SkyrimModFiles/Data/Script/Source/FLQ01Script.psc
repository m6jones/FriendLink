Scriptname FLQ01Script extends Quest  

import FriendLinkScript
 
SPELL Property FLQConnectSpell  Auto  
SPELL Property FLQDisconnectSpell  Auto
MiscObject Property FLQBucket  Auto

Event OnInit()
	;SetStage(0)
	;Game.GetPlayer().AddSpell(FLQConnectSpell)
endEvent

function ConnectStage() 
	SetObjectiveCompleted(0,false)
	SetObjectiveDisplayed(10)
	if(FLConnect(FLQBucket.GetFormID()))
		FLStartDataTransfer()
		Game.GetPlayer().AddSpell(FLQDisconnectSpell)
		Game.GetPlayer().RemoveSpell(FLQConnectSpell)
	else 
		reset()
		SetStage(0)
	endif
EndFunction


function DisconnectStage()
	FLDisconnect()
	SetObjectiveCompleted(10,false)
	SetObjectiveDisplayed(20)
	SetObjectiveCompleted(20, false)
	;reset()
	SetStage(0)
EndFunction

function onStage0Load()
	Debug.Notification("Stage 0 started")
	SetObjectiveDisplayed(0)
	Game.GetPlayer().AddSpell(FLQConnectSpell)
	Game.GetPlayer().AddSpell(FLQDisconnectSpell)
	Game.GetPlayer().RemoveSpell(FLQDisconnectSpell)
EndFunction

function PlayerLoadGame()
	;if(FLIsConnected())
		;SetStage(10)
	;else
		;SetStage(0)
	;endif
EndFunction


