// iHookIN5.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "iHookIN5.h"

#include "hooks.h"
#include "log2file.h"
#include "registry.h"

#pragma message( "Compiling " __FILE__ )
//TEST for IN HTML5 browser
#pragma message ( "############### compiling for INHTML5 ######################" )

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE			g_hInst;			// current instance
HWND				g_hWndMenuBar;		// menu bar handle

	TCHAR szAppName[] = L"iHookIN5 v3.4.4";
	BOOL g_bForwardKey=false;
	NOTIFYICONDATA nid;

// Forward declarations of functions included in this code module:
ATOM			MyRegisterClass(HINSTANCE, LPTSTR);
BOOL			InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

HICON g_hIcon[3];
void RemoveIcon(HWND hWnd);
void ChangeIcon(int idIcon);
HWND getTargetSubWindow(int* iResult);
HWND getTargetWindow();

int ReadReg();	//new with v3.4.0
//global to hold keycodes and commands assigned
typedef struct {
	byte keyCode;
	TCHAR keyCmd[MAX_PATH+1];
	TCHAR keyArg[MAX_PATH+1];
	DWORD KeyForward;
} hookmap;
static hookmap kMap[10];
int lastKey=-1;
BOOL launchExe4Key(DWORD vkCode, DWORD wmMsg, BOOL* bFound);

// Global variables can still be your...friend.
// CIHookDlg* g_This			= NULL;			// Needed for this kludgy test app, allows callback to update the UI
HINSTANCE	g_hHookApiDLL	= NULL;			// Handle to loaded library (system DLL where the API is located)

HWND		g_hWndIN5		= NULL;			// handle to browser component window
DWORD		g_dwTimer		= 1001;			//our timer id
UINT		g_uTimerId		= 0;			//timer id

DWORD g_lparamCodeDown[] = {	
							0x050001,	//f1
							0x060001,
							0x040001,
							0x0C0001,
							0x030001,
							0x0B0001,
							0x830001,
							0x0A0001,
							0x010001,
							0x090001,
							0x780001,
							0x070001,	//f12
							0x008000,	//f13
							0x010000,
							0x018000,
							0x020000,
							0x028000,
							0x030000,
							0x038000,
							0x040000,
							0x048000,
							0x050000,
							0x057000,
							0x05F000,	//f24
						};
DWORD g_lparamCodeUp[] = { 
							0xc0050001,	//f1
							0xc0060001,
							0xc0040001,
							0xc00C0001,
							0xc0030001,
							0xc00B0001,
							0xc0830001,
							0xc00A0001,
							0xc0010001,
							0xc0090001,
							0xc0780001,
							0xc0070001,	//f12
							0xc0008000,	//f13
							0xc0010000,
							0xc0018000,
							0xc0020000,
							0xc0028000,
							0xc0030000,
							0xc0038000,
							0xc0040000,
							0xc0048000,
							0xc0050000,
							0xc0057000,
							0xc005F000,	//f24
};

// Global functions: The original Open Source
BOOL g_HookDeactivate();
BOOL g_HookActivate(HINSTANCE hInstance);

//
#pragma data_seg(".HOOKDATA")									//	Shared data (memory) among all instances.
	HHOOK g_hInstalledLLKBDhook = NULL;						// Handle to low-level keyboard hook
	//HWND hWnd	= NULL;											// If in a DLL, handle to app window receiving WM_USER+n message
#pragma data_seg()

#pragma comment(linker, "/SECTION:.HOOKDATA,RWS")		//linker directive

// The command below tells the OS that this EXE has an export function so we can use the global hook without a DLL
__declspec(dllexport) LRESULT CALLBACK g_LLKeyboardHookCallback(
   int nCode,      // The hook code
   WPARAM wParam,  // The window message (WM_KEYUP, WM_KEYDOWN, etc.)
   LPARAM lParam   // A pointer to a struct with information about the pressed key
) 
{
	/*	typedef struct {
	    DWORD vkCode;
	    DWORD scanCode;
	    DWORD flags;
	    DWORD time;
	    ULONG_PTR dwExtraInfo;
	} KBDLLHOOKSTRUCT, *PKBDLLHOOKSTRUCT;*/
	
	// Get out of hooks ASAP; no modal dialogs or CPU-intensive processes!
	// UI code really should be elsewhere, but this is just a test/prototype app
	// In my limited testing, HC_ACTION is the only value nCode is ever set to in CE
	static int iActOn = HC_ACTION;
	bool processed_key=false;
	int iResult=0;
	BOOL bMatchedKey=FALSE;
	BOOL bForwardProcessKey=FALSE;

	if (nCode == iActOn) 
	{ 
		PKBDLLHOOKSTRUCT pkbhData = (PKBDLLHOOKSTRUCT)lParam;
		HWND hwndBrowserComponent=getTargetWindow();//FindWindow(L"KeyTest3AK",L"KeyTest3AK");
		if(hwndBrowserComponent==NULL){	// if IE is not loaded or not in foreground or browser window not found
			return CallNextHookEx(g_hInstalledLLKBDhook, nCode, wParam, lParam);
		}
		//we are only interested in FKey press/release
		if(pkbhData->vkCode >= VK_F1 && pkbhData->vkCode <=VK_F24){
			DEBUGMSG(1,(L"found function key 0x%08x ...\r\n", pkbhData->vkCode));
			Add2Log(L"found function key 0x%08x ...\r\n", pkbhData->vkCode);
			if( (pkbhData->vkCode==VK_F1) ) // || (pkbhData->vkCode==VK_F2) )
			{
				DEBUGMSG(1,(L"F1 is ignored\r\n"));
				Add2Log(L"F1 is ignored\r\n");
				return true;	//just ignore F1 key
			}
			if(processed_key==false){
				if (wParam == WM_KEYUP)
				{
					//synthesize a WM_KEYUP
					bForwardProcessKey=launchExe4Key(pkbhData->vkCode, WM_KEYUP, &bMatchedKey);
					if(bMatchedKey && bForwardProcessKey){//process a matched CreateProcess key?
						DEBUGMSG(1,(L"posting PROCESS WM_KEYUP 0x%08x to 0x%08x, lParam=0x%08x...\r\n", pkbhData->vkCode, hwndBrowserComponent, g_lparamCodeUp[pkbhData->vkCode - 0x70]));
						Add2Log(L"posting PROCESS WM_KEYUP 0x%08x to 0x%08x, lParam=0x%08x...\r\n", pkbhData->vkCode, hwndBrowserComponent, g_lparamCodeUp[pkbhData->vkCode - 0x70]);
						PostMessage(hwndBrowserComponent, WM_KEYUP, pkbhData->vkCode, g_lparamCodeUp[pkbhData->vkCode - 0x70]);						
					}
					else if(!bMatchedKey)
					{
						DEBUGMSG(1,(L"posting WM_KEYUP 0x%08x to 0x%08x, lParam=0x%08x...\r\n", pkbhData->vkCode, hwndBrowserComponent, g_lparamCodeUp[pkbhData->vkCode - 0x70]));
						Add2Log(L"posting WM_KEYUP 0x%08x to 0x%08x, lParam=0x%08x...\r\n", pkbhData->vkCode, hwndBrowserComponent, g_lparamCodeUp[pkbhData->vkCode - 0x70]);
						PostMessage(hwndBrowserComponent, WM_KEYUP, pkbhData->vkCode, g_lparamCodeUp[pkbhData->vkCode - 0x70]);						
					}
					processed_key=true;
				}
				else if (wParam == WM_KEYDOWN)
				{
					//synthesize a WM_KEYDOWN
					bForwardProcessKey=launchExe4Key(pkbhData->vkCode, WM_KEYDOWN, &bMatchedKey);
					if(bMatchedKey && bForwardProcessKey){
						DEBUGMSG(1,(L"posting PROCESS WM_KEYDOWN 0x%08x to 0x%08x, lParam=0x%08x...\r\n", pkbhData->vkCode, hwndBrowserComponent, g_lparamCodeDown[pkbhData->vkCode - 0x70]));
						Add2Log(L"posting PROCESS WM_KEYDOWN 0x%08x to 0x%08x, lParam=0x%08x...\r\n", pkbhData->vkCode, hwndBrowserComponent, g_lparamCodeDown[pkbhData->vkCode - 0x70]);
						PostMessage(hwndBrowserComponent, WM_KEYDOWN, pkbhData->vkCode, g_lparamCodeDown[pkbhData->vkCode - 0x70]);
					}
					else if(!bMatchedKey)
					{
						DEBUGMSG(1,(L"posting WM_KEYDOWN 0x%08x to 0x%08x, lParam=0x%08x...\r\n", pkbhData->vkCode, hwndBrowserComponent, g_lparamCodeDown[pkbhData->vkCode - 0x70]));
						Add2Log(L"posting WM_KEYDOWN 0x%08x to 0x%08x, lParam=0x%08x...\r\n", pkbhData->vkCode, hwndBrowserComponent, g_lparamCodeDown[pkbhData->vkCode - 0x70]);
						PostMessage(hwndBrowserComponent, WM_KEYDOWN, pkbhData->vkCode, g_lparamCodeDown[pkbhData->vkCode - 0x70]);
					}
					processed_key=true;
				}
				else if (wParam == WM_CHAR)	//this will never happen
				{
					DEBUGMSG(1, (L"WM_CHAR: 0x%x\r\n", pkbhData->vkCode));
					Add2Log(L"WM_CHAR: 0x%x\r\n", pkbhData->vkCode);
				}
			}
		}
		else if(pkbhData->vkCode==VK_TSTAR){
			DEBUGMSG(1, (L"VK_TSTAR: 0x%x\r\n", pkbhData->vkCode));
			Add2Log(L"VK_TSTAR: 0x%x\r\n", pkbhData->vkCode);
		}
		else if(pkbhData->vkCode==VK_TPOUND){
			DEBUGMSG(1, (L"VK_TPOUND: 0x%x\r\n", pkbhData->vkCode));
			Add2Log(L"VK_TPOUND: 0x%x\r\n", pkbhData->vkCode);
		}
		else if(pkbhData->vkCode==VK_HYPHEN){
			//got VK_PERIOD wParam=0x100 lParam=0x1f7cfc44 
			//got VK_PERIOD wParam=0x101 lParam=0x1f7cfc44
			/* down,char,up:
			VK_COMMA(0xbc)	1
			,(0x2c)	1
			VK_COMMA(0xbc)	c0000001

			VK_PERIOD(0xbe)	490001
			.(0x2e)	490001
			VK_PERIOD(0xbe)	c0490001
			*/
			if(processed_key==false){
				DEBUGMSG(1, (L"got VK_HYPHEN wParam=0x%02x lParam=0x%02x \r\n", wParam, lParam));
				Add2Log(L"got VK_HYPHEN wParam=0x%02x lParam=0x%02x \r\n", wParam, lParam);
				if(wParam==WM_KEYUP){
					keybd_event(VK_COMMA, 0x00, KEYEVENTF_KEYUP | 0, 0);
					//PostMessage(hwndBrowserComponent , WM_CHAR, 0x2c,   0x00000001);
					//PostMessage(hwndBrowserComponent , WM_KEYUP, VK_COMMA,   0xC0000001);
					
				}else if(wParam==WM_KEYDOWN){
					keybd_event(VK_COMMA, 0x00, 0, 0);
					//PostMessage(hwndBrowserComponent, WM_KEYDOWN, VK_COMMA, 0x00000001 );
				}
				processed_key=TRUE;
			}
		}
	}//nCode == iActOn
	else{
		DEBUGMSG(1, (L"Got unknwon action code: 0x%08x\r\n", nCode));
		Add2Log(L"Got unknwon action code: 0x%08x\r\n", nCode);
	}
	//shall we forward processed keys?
	if (processed_key)
	{
		processed_key=false; //reset flag
		if (g_bForwardKey){
			Add2Log(L"g_bForwardKey=%i\r\n", g_bForwardKey);
			return CallNextHookEx(g_hInstalledLLKBDhook, nCode, wParam, lParam);
		}
		else if(bMatchedKey && bForwardProcessKey) {
			Add2Log(L"bMatchedKey=%i bForwardProcessKey=%i\r\n", bMatchedKey, bForwardProcessKey);
			return CallNextHookEx(g_hInstalledLLKBDhook, nCode, wParam, lParam);
		}
		else
		{
			Add2Log(L"g_bForwardKey=%i\r\n", g_bForwardKey);
			return true;
		}
	}
	else{
		Add2Log(L"CallNextHookEx...\r\n");
		return CallNextHookEx(g_hInstalledLLKBDhook, nCode, wParam, lParam);
	}
}

BOOL g_HookActivate(HINSTANCE hInstance)
{
	// We manually load these standard Win32 API calls (Microsoft says "unsupported in CE")
	SetWindowsHookEx		= NULL;
	CallNextHookEx			= NULL;
	UnhookWindowsHookEx	= NULL;

	// Load the core library. If it's not found, you've got CErious issues :-O
	//TRACE(_T("LoadLibrary(coredll.dll)..."));
	g_hHookApiDLL = LoadLibrary(_T("coredll.dll"));
	if(g_hHookApiDLL == NULL) return false;
	else {
		// Load the SetWindowsHookEx API call (wide-char)
		//TRACE(_T("OK\nGetProcAddress(SetWindowsHookExW)..."));
		SetWindowsHookEx = (_SetWindowsHookExW)GetProcAddress(g_hHookApiDLL, _T("SetWindowsHookExW"));
		if(SetWindowsHookEx == NULL) return false;
		else
		{
			// Load the hook.  Save the handle to the hook for later destruction.
			//TRACE(_T("OK\nCalling SetWindowsHookEx..."));
			g_hInstalledLLKBDhook = SetWindowsHookEx(WH_KEYBOARD_LL, g_LLKeyboardHookCallback, hInstance, 0);
			if(g_hInstalledLLKBDhook == NULL) return false;
		}

		// Get pointer to CallNextHookEx()
		//TRACE(_T("OK\nGetProcAddress(CallNextHookEx)..."));
		CallNextHookEx = (_CallNextHookEx)GetProcAddress(g_hHookApiDLL, _T("CallNextHookEx"));
		if(CallNextHookEx == NULL) return false;

		// Get pointer to UnhookWindowsHookEx()
		//TRACE(_T("OK\nGetProcAddress(UnhookWindowsHookEx)..."));
		UnhookWindowsHookEx = (_UnhookWindowsHookEx)GetProcAddress(g_hHookApiDLL, _T("UnhookWindowsHookEx"));
		if(UnhookWindowsHookEx == NULL) return false;
	}
	//TRACE(_T("OK\nEverything loaded OK\r\n"));
	return true;
}


BOOL g_HookDeactivate()
{
	//TRACE(_T("Uninstalling hook..."));
	if(g_hInstalledLLKBDhook != NULL)
	{
		UnhookWindowsHookEx(g_hInstalledLLKBDhook);		// Note: May not unload immediately because other apps may have me loaded
		g_hInstalledLLKBDhook = NULL;
	}

	//TRACE(_T("OK\nUnloading coredll.dll..."));
	if(g_hHookApiDLL != NULL)
	{
		FreeLibrary(g_hHookApiDLL);
		g_hHookApiDLL = NULL;
	}
	//TRACE(_T("OK\nEverything unloaded OK\r\n"));
	return true;
}


int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPTSTR    lpCmdLine,
                   int       nCmdShow)
{
	if(wcsstr(lpCmdLine, L"uselogging")!=NULL){
		initFileNames();
		bUseLogging=TRUE;
		Add2LogWithTime(L"iHookIN5 started with logging\r\n");
	}

	MSG msg;

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow)) 
	{
		return FALSE;
	}


	HACCEL hAccelTable;
	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_IHOOKIE6));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
ATOM MyRegisterClass(HINSTANCE hInstance, LPTSTR szWindowClass)
{
	WNDCLASS wc;

	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_IHOOKIE6));
	wc.hCursor       = 0;
	wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName  = 0;
	wc.lpszClassName = szWindowClass;

	return RegisterClass(&wc);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND hWnd;
    TCHAR szTitle[MAX_LOADSTRING];		// title bar text
    TCHAR szWindowClass[MAX_LOADSTRING];	// main window class name

    g_hInst = hInstance; // Store instance handle in our global variable

    // SHInitExtraControls should be called once during your application's initialization to initialize any
    // of the device specific controls such as CAPEDIT and SIPPREF.
    //SHInitExtraControls();

	wsprintf(szTitle, L"%s", L"iHookIN5");
	wsprintf(szWindowClass, L"%s", L"iHookIN5");

	//LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING); 
    //LoadString(hInstance, IDC_IHOOKIE6, szWindowClass, MAX_LOADSTRING);

    //If it is already running, then focus on the window, and exit
    hWnd = FindWindow(szWindowClass, szTitle);	
    if (hWnd) 
    {
        // set focus to foremost child window
        // The "| 0x00000001" is used to bring any owned windows to the foreground and
        // activate them.
        SetForegroundWindow((HWND)((ULONG) hWnd | 0x00000001));
        return 0;
    } 

    if (!MyRegisterClass(hInstance, szWindowClass))
    {
		DEBUGMSG(1, (L"Already running? %i\r\n", GetLastError()));
    	return FALSE;
    }

		hWnd = CreateWindow(szWindowClass, szTitle, WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
		DEBUGMSG(1, (L"CreateWindow: %i\r\n", GetLastError()));

    if (!hWnd)
    {
        return FALSE;
    }

    // When the main window is created using CW_USEDEFAULT the height of the menubar (if one
    // is created is not taken into account). So we resize the window after creating it
    // if a menubar is present
    if (g_hWndMenuBar)
    {
        RECT rc;
        RECT rcMenuBar;

        GetWindowRect(hWnd, &rc);
        GetWindowRect(g_hWndMenuBar, &rcMenuBar);
        rc.bottom -= (rcMenuBar.bottom - rcMenuBar.top);
		
        MoveWindow(hWnd, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, FALSE);
    }

    ShowWindow   (hWnd , SW_HIDE); // nCmdShow);
    UpdateWindow(hWnd);

	//Create a Notification icon
	HICON hIcon;
	hIcon=(HICON) LoadImage (g_hInst, MAKEINTRESOURCE (IHOOK_STARTED), IMAGE_ICON, 16,16,0);
	nid.cbSize = sizeof (NOTIFYICONDATA);
	nid.hWnd = hWnd;
	nid.uID = 1;
	nid.uFlags = NIF_ICON | NIF_MESSAGE;
	// NIF_TIP not supported    
	nid.uCallbackMessage = MYMSG_TASKBARNOTIFY;
	nid.hIcon = hIcon;
	nid.szTip[0] = '\0';
	BOOL res = Shell_NotifyIcon (NIM_ADD, &nid);
	if(!res){
		DEBUGMSG(1 ,(L"Could not add taskbar icon. LastError=%i\r\n", GetLastError() ));
	}

	//load icon set
	g_hIcon[0] =(HICON) LoadImage (g_hInst, MAKEINTRESOURCE (IDI_ICON_BAD), IMAGE_ICON, 16,16,0);
	g_hIcon[1] =(HICON) LoadImage (g_hInst, MAKEINTRESOURCE (IDI_ICON_WARN), IMAGE_ICON, 16,16,0);
	g_hIcon[2] =(HICON) LoadImage (g_hInst, MAKEINTRESOURCE (IDI_ICON_OK), IMAGE_ICON, 16,16,0);

#ifdef DEBUG
	if (!res)
		DEBUGMSG(1, (L"InitInstance: %i\r\n", GetLastError()));
#endif
	
    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    //PAINTSTRUCT ps;
    //HDC hdc;

    static SHACTIVATEINFO s_sai;
	
    switch (message) 
    {
        case WM_COMMAND:
            wmId    = LOWORD(wParam); 
            wmEvent = HIWORD(wParam); 
            // Parse the menu selections:
            switch (wmId)
            {
                case IDM_HELP_ABOUT:
                    DialogBox(g_hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, About);
                    break;
                case IDM_OK:
                    SendMessage (hWnd, WM_CLOSE, 0, 0);				
                    break;
                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);
            }
            break;
		case WM_TIMER:	//used to update notification icon
			if(wParam==g_dwTimer){
				int iResult = 0;
				getTargetSubWindow(&iResult);
				DEBUGMSG(1,(L"Timer: getTargetSubWindow(), iResult=%i\r\n", iResult));
				ChangeIcon(iResult);
			}
			return 0;
			break;
        case WM_CREATE:
			ReadReg();
			if (g_HookActivate(g_hInst))
			{
				MessageBeep(MB_OK);
				//system bar icon
				//ShowIcon(hWnd, g_hInst);
				DEBUGMSG(1, (L"Hook loaded OK\r\n"));
				Add2Log("Hook loaded OK\r\n", TRUE);
			}
			else
			{
				MessageBeep(MB_ICONEXCLAMATION);
				MessageBox(hWnd, L"Could not hook. Already running a copy of iHook3? Will exit now.", L"iHook3", MB_OK | MB_ICONEXCLAMATION);
				Add2Log("Could not hook. Already running a copy of iHook3? Will exit now.\r\n", TRUE);
				PostQuitMessage(-1);
			}
			g_uTimerId = SetTimer(hWnd, g_dwTimer, 5000, NULL);
			//TRACE(_T("Hook did not success"));
			return 0;

			SHMENUBARINFO mbi;

            memset(&mbi, 0, sizeof(SHMENUBARINFO));
            mbi.cbSize     = sizeof(SHMENUBARINFO);
            mbi.hwndParent = hWnd;
            mbi.nToolBarId = IDR_MENU;
            mbi.hInstRes   = g_hInst;

            if (!SHCreateMenuBar(&mbi)) 
            {
                g_hWndMenuBar = NULL;
            }
            else
            {
                g_hWndMenuBar = mbi.hwndMB;
            }

            // Initialize the shell activate info structure
            memset(&s_sai, 0, sizeof (s_sai));
            s_sai.cbSize = sizeof (s_sai);
			if(true){
				int iResult = 0;
				getTargetSubWindow(&iResult);
				DEBUGMSG(1,(L"Timer: getTargetSubWindow(), iResult=%i\r\n", iResult));
				ChangeIcon(iResult);
			}
			break;
        case WM_PAINT:
			PAINTSTRUCT ps;    
			RECT rect;    
			HDC hdc;     // Adjust the size of the client rectangle to take into account    
			// the command bar height.    
			GetClientRect (hWnd, &rect);    
			hdc = BeginPaint (hWnd, &ps);     
			DrawText (hdc, TEXT ("iHookIN5 loaded"), -1, &rect,
				DT_CENTER | DT_VCENTER | DT_SINGLELINE);    
			EndPaint (hWnd, &ps);     
			return 0;
            break;
        case WM_DESTROY:
            CommandBar_Destroy(g_hWndMenuBar);
			//taskbar system icon
			RemoveIcon(hWnd);
			MessageBeep(MB_OK);
			g_HookDeactivate();
			Add2LogWithTime(L"Hook ending...\r\n");
			//Shell_NotifyIcon(NIM_DELETE, &nid);
            PostQuitMessage(0);
            break;

        case WM_ACTIVATE:
            // Notify shell of our activate message
            SHHandleWMActivate(hWnd, wParam, lParam, &s_sai, FALSE);
            break;
        case WM_SETTINGCHANGE:
            SHHandleWMSettingChange(hWnd, wParam, lParam, &s_sai);
            break;
		case MYMSG_TASKBARNOTIFY:
				switch (lParam) {
					case WM_LBUTTONUP:
						//ShowWindow(hWnd, SW_SHOWNORMAL);
						SetWindowPos(hWnd, HWND_TOPMOST, 0,0,0,0, SWP_NOSIZE | SWP_NOREPOSITION | SWP_SHOWWINDOW);
						if (MessageBox(hWnd, L"Hook is loaded. End hooking?", szAppName, 
							MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL | MB_SETFOREGROUND | MB_TOPMOST)==IDYES)
						{
							g_HookDeactivate();
							Shell_NotifyIcon(NIM_DELETE, &nid);
							PostQuitMessage (0) ; 
						}
						ShowWindow(hWnd, SW_HIDE);
					}
			return 0;
			break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
            {
                // Create a Done button and size it.  
                SHINITDLGINFO shidi;
                shidi.dwMask = SHIDIM_FLAGS;
                shidi.dwFlags = SHIDIF_DONEBUTTON | SHIDIF_SIPDOWN | SHIDIF_SIZEDLGFULLSCREEN | SHIDIF_EMPTYMENU;
                shidi.hDlg = hDlg;
                SHInitDialog(&shidi);
            }
            return (INT_PTR)TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                EndDialog(hDlg, LOWORD(wParam));
                return TRUE;
            }
            break;

        case WM_CLOSE:
            EndDialog(hDlg, message);
            return TRUE;

    }
    return (INT_PTR)FALSE;
}

void ChangeIcon(int idIcon)
{
//    NOTIFYICONDATA nid;
    int nIconID=1;
    nid.cbSize = sizeof (NOTIFYICONDATA);
    //nid.hWnd = hWnd;
    //nid.uID = idIcon;//	nIconID;
    //nid.uFlags = NIF_ICON | NIF_MESSAGE;   // NIF_TIP not supported
    //nid.uCallbackMessage = MYMSG_TASKBARNOTIFY;
    nid.hIcon = g_hIcon[idIcon];	// (HICON)LoadImage (g_hInst, MAKEINTRESOURCE (ID_ICON), IMAGE_ICON, 16,16,0);
    //nid.szTip[0] = '\0';

    BOOL r = Shell_NotifyIcon (NIM_MODIFY, &nid);
	if(!r){
		DEBUGMSG(1 ,(L"Could not change taskbar icon. LastError=%i\r\n", GetLastError() ));
		Add2Log(L"Could not change taskbar icon. LastError=%i\r\n", GetLastError());
	}
}

void RemoveIcon(HWND hWnd)
{
	NOTIFYICONDATA nid;

    memset (&nid, 0, sizeof nid);
    nid.cbSize = sizeof (NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = 1;

    Shell_NotifyIcon (NIM_DELETE, &nid);

}

//search for a child window with class name
static BOOL bStopScan=FALSE;
BOOL scanWindow(HWND hWndStart, TCHAR* szClass){
	HWND hWnd = NULL;
	HWND hWnd1 = NULL;	

	hWnd = hWndStart;

	TCHAR cszWindowString [MAX_PATH]; // = new TCHAR(MAX_PATH);
	TCHAR cszClassString [MAX_PATH]; //= new TCHAR(MAX_PATH);
	TCHAR cszTemp [MAX_PATH]; //= new TCHAR(MAX_PATH);
	
	BOOL bRet=FALSE;

	while (hWnd!=NULL && !bStopScan){
		//do some formatting

		GetClassName(hWnd, cszClassString, MAX_PATH);
		GetWindowText(hWnd, cszWindowString, MAX_PATH);

		wsprintf(cszTemp, L"\"%s\"  \"%s\"\t0x%08x\r\n", cszClassString, cszWindowString, hWnd);//buf);
		//DEBUGMSG(1, (cszTemp));

		if(wcsicmp(cszClassString, szClass)==0){
			DEBUGMSG(1 , (L"\n################### found target window, hwnd=0x%08x\n\r\n", hWnd));
			//set global hwnd
			g_hWndIN5=hWnd;	//store in global var
			hWnd=NULL;
			hWndStart=NULL;
			bRet=TRUE;
			bStopScan=TRUE;
			return TRUE;	//exit loop
		}
		// Find next child Window
		hWnd1 = GetWindow(hWnd, GW_CHILD);
		if( hWnd1 != NULL ){ 
			scanWindow(hWnd1, szClass);
		}
		//Find next neighbor window
		hWnd=GetWindow(hWnd,GW_HWNDNEXT);		// Get Next Window
	}
	return bRet;
}

/// look for IE window and test if in foreground
/// if found and in foreground, return window handle
/// else return NULL
///	result will be 0 or 1 or 2
HWND getTargetSubWindow(int* iResult){
	HWND hwnd_iem = FindWindow(L"Intermec HTML5 Browser", NULL);	// try INHTML5 window
	if((hwnd_iem!=NULL) && (hwnd_iem==GetForegroundWindow()))
		*iResult=2;
	else if(hwnd_iem!=NULL)
		*iResult=1;
	else
		*iResult=0;
	return hwnd_iem;
}

enum IETYPE{
	None = 0,
	IEMobile,
	IExplore
} myIETYPE;

HWND getTargetWindow(){
	//DEBUGMSG(1, (L"using hardcoded IEMobile window\r\n"));
	//g_hWndIN5 = (HWND)0x7c0d28A0;
	//return;

	//TEST FOR IN HTML5 browser
	g_hWndIN5=FindWindow(L"Intermec HTML5 Browser", NULL);
	Add2Log(L"Target window is 0x%08x\r\n", g_hWndIN5);
	return g_hWndIN5;	//return global var

}

int ReadReg()
{
	Add2Log(L"IN ReadReg()...\r\n");
	int i;
	TCHAR str[MAX_PATH+1];
	byte dw=0;
	DWORD dword=0;

	TCHAR name[MAX_PATH+1];
	lastKey=-1;
	LONG rc;
	int iRes = OpenKey(L"Software\\Intermec\\iHookIN5");
	Add2Log(L"\tOpenKey 'Software\\Intermec\\iHookIN5' returned %i (0x%x)\r\n", iRes, iRes);
	for (i=0; i<10; i++)
	{
		kMap[i].keyCode=0;
		wcscpy(kMap[i].keyCmd, L"");
		wcscpy(kMap[i].keyArg, L"");
		wsprintf(name, L"key%i", i);
		//look for keyX
		rc = RegReadByte(name, &dw);		
		if(rc!=0)	//try with a DWORD
		{
			DWORD dwValKey=0;
			rc=RegReadDword(name, &dwValKey);
			if(rc==0)
				dw=(BYTE)dwValKey;
		}
		if (rc==0)
		{
			//look for exeX
			Add2Log(L"\tlooking for entry 'key%i' (name='%s') return code=%i read value=(0x%x)...OK\r\n", i, name, rc, dw);
			kMap[i].keyCode=dw;
			wsprintf(name, L"exe%i", i);
			iRes=RegReadStr(name, str);
			Add2Log(L"\t\tlooking for exe%i (name='%s'), result=%i, value='%s'\r\n", i, name, iRes, str);
			if(iRes==0)
			{
				wcscpy(kMap[i].keyCmd, str);
				//look for argX
				wsprintf(name, L"arg%i", i);
				iRes=RegReadStr(name, str);
				lastKey=i;	// a valid combination is a keyX and a cmdX entry
				if(iRes==0)
				{
					Add2Log(L"\t\tlooking for arg%i (name='%s'), result=%i, value='%s' OK\r\n", i, name, iRes, str);
					wcscpy(kMap[i].keyArg, str);
				}
				else
				{
					Add2Log(L"\t\tlooking for arg%i (name='%s') result=%i FAILED. Using empty arg.\r\n", i, name, iRes);
				}
				wsprintf(name, L"forwardKey%i", i);
				DWORD dwVal=0;
				iRes = RegReadDword(name, &dwVal);
				if(iRes==0)
				{
					Add2Log(L"\t\tlooking for forwardKey%i (name='%s'), result=%i, value=%i OK\r\n", i, name, iRes, dwVal);
				}
				else
					Add2Log(L"\t\tlooking for forwardKey%i (name='%s'), result=%i FAILED, value=%i OK\r\n", i, name, iRes, dwVal);
				kMap[i].KeyForward=dwVal;
			}
			else {
				Add2Log(L"\t\tlooking for exe%i FAILED with result=%i\r\n", i, iRes);
				break; //no exe name
			}
		}
		else
		{
			#ifdef DEBUG
						ShowError(rc);
			#endif
			Add2Log(L"\tlooking for entry 'key%i' (name='%s') return code=%i...FAILED\r\n", i, name, rc);
			break; //no key
		}
	}
	Add2Log(L"\tread a total of %i (0x%x) valid entries\r\n", lastKey+1, lastKey+1);
	//Read if we have to forward the keys
	rc=RegReadDword(L"ForwardKey", &dword);
	if(rc==0)
	{
		Add2Log(L"\tlooking for 'ForwardKey' OK\r\n",lastKey,lastKey);
		if (dword>0){
			Add2Log(L"\tForwardKey is TRUE \r\n", FALSE);
			g_bForwardKey=true;
		}
		else{
			Add2Log(L"\tForwardKey is FALSE \r\n", FALSE);
			g_bForwardKey=false;
		}
	}
	else
	{
		#ifdef DEBUG
			ShowError(rc);
		#endif
		Add2Log(L"\tlooking for 'ForwardKey' FAILED. Using default=TRUE.\r\n", FALSE);
		g_bForwardKey=true;
	}
	CloseKey();
	Add2Log(L"OUT ReadReg()...\r\n", FALSE);
	return 0;
}

//check if exe is to launch for key
//return TRUE if key should be forwarded
//bFound is used to report if a key is a CreateProcess key
BOOL launchExe4Key(DWORD vkCode, DWORD wmMsg, BOOL* bFound){
	PROCESS_INFORMATION pi;
	BOOL bForwardTheKey=FALSE;
	int i=0;
	DEBUGMSG(1, (L"# hook got 0x%02x (%i). Looking for match...\r\n", vkCode, vkCode));
	BOOL bMatchFound=FALSE;
	for (i=0; i<=lastKey; i++) 
	{
		if (vkCode == kMap[i].keyCode)
		{
			bMatchFound=TRUE;
			DEBUGMSG(1, (L"# hook Catched key 0x%0x, launching '%s'\r\n", kMap[i].keyCode, kMap[i].keyCmd));
			Add2Log(L"# hook Matched key 0x%0x, launching '%s'\r\n", kMap[i].keyCode, kMap[i].keyCmd);
			if(wmMsg != WM_KEYUP)	//only process keyup msg
			{
				if (CreateProcess(kMap[i].keyCmd, kMap[i].keyArg, NULL, NULL, NULL, 0, NULL, NULL, NULL, &pi))
				{
					DEBUGMSG(1,(L"# hook CreateProcess OK\r\n", FALSE));
					CloseHandle(pi.hProcess);
					CloseHandle(pi.hThread);
				}
				else{
					DEBUGMSG(1,(L"# hook CreateProcess FAILED. LastError=%i (0x%x)\r\n", GetLastError(), GetLastError()));
				}
				DEBUGMSG(1,(L"# hook processed_key is TRUE\r\n", FALSE));
			}
			if(kMap[i].KeyForward==0)
				bForwardTheKey = FALSE;
			else
				bForwardTheKey = TRUE;
		}
	}
	*bFound=bMatchFound;
	return bForwardTheKey; //should the key be forwarded
}