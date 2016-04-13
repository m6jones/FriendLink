Scriptname FLQTranslatorObject extends ObjectReference

Function FLMarkTranslatingComplete() native

Event OnTranslationAlmostComplete()
	;Debug.Notification("Translation Almost Done!")
	self.FLMarkTranslatingComplete()
EndEvent

Event OnTranslationComplete()
  ;Debug.Notification("Translation Done!")
  self.FLMarkTranslatingComplete()
EndEvent

Event OnTranslationFailed()
  ;Debug.Notification("Translation failed!")
  self.FLMarkTranslatingComplete()
EndEvent
  
