;BEGIN FRAGMENT CODE - Do not edit anything between this and the end comment
;NEXT FRAGMENT INDEX 9
Scriptname QF_FLQ01_02001D8E Extends Quest Hidden

;BEGIN FRAGMENT Fragment_2
Function Fragment_2()
;BEGIN CODE
this_quest.ConnectStage()
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_8
Function Fragment_8()
;BEGIN CODE
SetObjectiveDisplayed(0)
Game.GetPlayer().AddSpell(FLQConnectSpell)
;Game.GetPlayer().AddSpell(FLQDisconnectSpell)
Game.GetPlayer().RemoveSpell(FLQDisconnectSpell)
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_4
Function Fragment_4()
;BEGIN CODE
this_quest.DisconnectStage()
;END CODE
EndFunction
;END FRAGMENT

;END FRAGMENT CODE - Do not edit anything between this and the begin comment

FLQ01Script Property this_quest  Auto  

SPELL Property FLQDisconnectSpell  Auto  

SPELL Property FLQConnectSpell  Auto  
