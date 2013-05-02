========================================================================
    WIN32 APPLICATION : iHookIN5 Project Overview
========================================================================

Install a keyboard hook and send Function key presses directly to Internet Explorer Mobile
or IntermecHTML5 window.

history:
v3.4.1
	added code to allow read of keyX value from DWORD registry type
			[HKEY_LOCAL_MACHINE\SOFTWARE\Intermec\iHookIN5]
			"ForwardKey"=hex:0
			"arg0"=""
			"exe0"="\\windows\\iRotateCN2.exe"
			"key0"=72
			"forwardKey0"=0
	
v3.4.0
	added registry code and code to start process if F12 is pressed
			[HKEY_LOCAL_MACHINE\SOFTWARE\Intermec\iHookIN5]
			"ForwardKey"=hex:0
			"arg0"=""
			"exe0"="\\windows\\iRotateCN2.exe"
			"key0"=hex:\
				  72	
			"forwardKey0"=0
v3.3.0
	started with a inHTML5 only version
	added code to ignore F1 key completely
v3.1.1
	initial release
v3.2.1
	added remap function to catch the Period key and send a Comma instead
v3.2.2
	changed to use keybd_event instead of PostMessage for remapped key
	
/////////////////////////////////////////////////////////////////////////////s