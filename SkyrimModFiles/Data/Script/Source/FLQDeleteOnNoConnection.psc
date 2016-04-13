Scriptname FLQDeleteOnNoConnection extends ObjectReference  

import FriendLinkScript

bool Function FLObjectInUse() native

EVENT onCellAttach()
	;Debug.Notification("cell")
	if (!self.FLObjectInUse())
		;Debug.Notification("Deleting Object cell")
		disable(false)
		delete()
	endif
endEVENT

Event OnLoad()
	;Debug.Notification("load")
	if(!self.FLObjectInUse()) 
		;Debug.Notification("Deleting Object")
		disable(false)
		delete()
	endIf
	
endEvent