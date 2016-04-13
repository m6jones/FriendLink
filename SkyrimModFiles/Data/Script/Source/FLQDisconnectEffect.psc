Scriptname FLQDisconnectEffect extends activemagiceffect  

Quest Property FLQ01  Auto  

Event OnEffectFinish(Actor Target,Actor Caster)
	FLQ01.SetStage(20)
endEvent