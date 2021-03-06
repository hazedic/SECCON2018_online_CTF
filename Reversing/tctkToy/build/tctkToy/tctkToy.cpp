// tctkToy.cpp : Defines the entry point for the application.
//
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <wincrypt.h>

// C RunTime Header Files
#include <stdlib.h>
#include <stdio.h>
#include <string> 
#include <shellapi.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <tlhelp32.h>

//tcl tk Header Files
#include "tctkToy.h"
#include <tcl.h>
#include <tk.h>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
TCHAR crDir[MAX_PATH + 1];						// current directory
TCHAR strFilePath[MAX_PATH];					// Drag & Drop tcl file name
TCHAR flag[25];									// flag buffer
char sha256flag[65] = "";						// sha256 flag
HCRYPTPROV  hProv = NULL;						// Hash Provider
HCRYPTHASH  hHash = NULL;						// Hash handle
PBYTE       pbHash = NULL;						// Hash Value Pointer

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lp);
DWORD WINAPI Sub_Tcl(LPVOID *filename);
BOOL Sub_BuildingInstructions(LPTSTR filename);
LPCSTR Sub_MakeFlag(LPTSTR filename);
void freeHashResources();

//
//  FUNCTION: EnumWindowsProc(HWND hwnd, LPARAM lp)
//	
//  PURPOSE: Callback function
//
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lp) {
	TCHAR strWindowText[1024];
	TCHAR strClassText[1024];
	INT*    lpCount = (INT *)lp;
	GetWindowText(hwnd, strWindowText, sizeof(strWindowText));
	GetClassName(hwnd, strClassText, sizeof(strClassText));
	if (*lpCount == 0) {
		if (strcmp(strWindowText, "") == 0 && strcmp(strClassText,"TkChild") == 0) {
			*lpCount += 10;
		}
	}
	else {
		if (strcmp(strWindowText, "") == 0 && strcmp(strClassText, "Button") == 0) {
			*lpCount += 1;
		}
	}
	
	return TRUE;
}

//
//  FUNCTION: Sub_Tcl()
//
//  PURPOSE: Tcl/Tk Tcl_EvalFile
//
DWORD WINAPI Sub_Tcl(LPVOID *filename)
{
	Tcl_Interp *interp;
	interp = Tcl_CreateInterp();
	Tcl_Init(interp);
	Tk_Init(interp);

	if (Tcl_EvalFile(interp, (const char*)filename) != TCL_OK) {
		MessageBox(NULL, Tcl_GetStringResult(interp), "Error", MB_OK);
		MessageBox(NULL,"It cannot eat this completely." , "Oops!", MB_OK);
		exit(1);
	}
	Tk_MainLoop();

	Tcl_Finalize();
	return 0;
}

//
//  FUNCTION: Sub_Building_Instructions(LPTSTR filename)
//
//  PURPOSE: Check Tcl file if it follows my building instructions
//
BOOL Sub_BuildingInstructions(LPTSTR filename)
{
	typedef struct _procedure {
		BOOL first;
		BOOL second;
		BOOL third;
		BOOL fourth;
		struct _procedure* next;
	} procedure;

	procedure* construction = (procedure*)malloc(sizeof(procedure));
	construction->first = False;
	construction->second = False;
	construction->third = False;
	construction->fourth = False;

	// proc1. if current dir = C:\\tctkToy (.tcl 1st line: cd C:\\tctkToy )
	TCHAR bDir[MAX_PATH + 1];
	GetCurrentDirectory(MAX_PATH + 1, bDir);
	if (strcmp(bDir, crDir) != 0)
	{
		if (strncmp(bDir, "C:\\tctkToy", 11) == 0)
		{
			construction->first = True;
		}
	}
	// proc2. if taskmgr.exe is launched (.tcl 2nd line: exec $env(COMSPEC) /c start C:\\Windows\\System32\\taskmgr.exe; )
	PROCESSENTRY32 entry;
	entry.dwFlags = sizeof(PROCESSENTRY32);
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (Process32First(snapshot, &entry) == TRUE) {
		while (Process32Next(snapshot, &entry) == TRUE) {
			if (_stricmp(entry.szExeFile, "Taskmgr.exe") == 0) {
				HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE, FALSE, entry.th32ProcessID);
				construction->second = True;
				TerminateProcess(hProcess, 1);
				CloseHandle(hProcess);
			}
		}
	}
	CloseHandle(snapshot);

	Sleep(1000);
	// proc3. if window title = "tctkROBO" (.tcl 3rd line: tk::toplevel ".", 4th line: wm title . "tctkROBO" )
	HWND hWnd = FindWindow(NULL, "tctkROBO");
	if (hWnd != NULL)
	{
		construction->third = True;
		TCHAR title[256] = "";
		GetWindowText(hWnd, title, 256);
		char classBuf[256] = "";
		GetClassName(hWnd, classBuf, 256);

		// proc4. if 4 child window component is enough.
		INT  nCount = 0;
		EnumChildWindows(hWnd, EnumWindowsProc, (LPARAM)&nCount);
		if (nCount == 13) {
			construction->fourth = True;
		}
	}

	if (construction->first) {
		if (construction->second) {
			if (construction->third) {
				if (construction->fourth) {
					return True;
				}
				else {
					MessageBox(NULL, "4th stage: to adjust the components", "Failed",  MB_OK);
				}
			}
			else {
				MessageBox(NULL, "3rd stage: to create spesific window", "Failed",  MB_OK);
			}
		}
		else {
			MessageBox(NULL, "2nd stage: to launch the process", "Failed",  MB_OK);
		}
	}else{
		MessageBox(NULL, "1st stage: to move the workplace", "Failed",  MB_OK);
	}

	return False;
}


//
//  FUNCTION: Sub_ConvertSha256(LPTSTR rawflag)
//
//  PURPOSE: Connvert fraw string to SHA256 hash
//
LPCSTR Sub_ConvertSha256(LPCSTR rawflag)
{

	// Get handle
	if (!CryptAcquireContext(&hProv,NULL,NULL,PROV_RSA_AES,CRYPT_VERIFYCONTEXT))
	{
		freeHashResources();
		return "fail";
	}

	// Generate Hash Value
	if (!CryptCreateHash(hProv,CALG_SHA_256,0,0,&hHash))
	{
		freeHashResources();
		return "fail";
	}

	BYTE Buf[25] = "";  // Target string to convert sha256 hash
	memcpy(Buf, rawflag, sizeof(Buf));

	//add hash data
	if (!CryptHashData(hHash,Buf,sizeof(Buf) - 1,0))
	{
		freeHashResources();
		return "fail";
	}

	DWORD dwDataLen = 32; // SHA256 256bit=32byte

	// Allocate Buffer 
	pbHash = (BYTE *)malloc(dwDataLen);
	if (NULL == pbHash)
	{
		printf("unable to allocate memory\n");
		freeHashResources();
		return "fail";
	}

	// retrieve the sha256 hash
	if (!CryptGetHashParam(hHash,HP_HASHVAL,pbHash,&dwDataLen,0))
	{
		freeHashResources();
		return "fail";
	}

	for (DWORD i = 0; i < dwDataLen; i++)
	{
		char buffer[3] = "";
		snprintf(buffer, sizeof(buffer),"%02x", pbHash[i]);
		strcat_s(sha256flag, 65, buffer);
	}
	//MessageBox(NULL, sha256flag, "sha256", MB_OK);

	if(strncmp(sha256flag, "a68361", 6)==0) {
		return (LPCSTR)sha256flag;

	}
	return "fail";
	
}


//
//  FUNCTION: freeHashResources
//
//  PURPOSE:free Hash Resources
//
void freeHashResources()
{
	if (hHash)
	{
		CryptDestroyHash(hHash);
	}

	if (hProv)
	{
		CryptReleaseContext(hProv, 0);
	}

	if (pbHash)
	{
		free(pbHash);
	}
	return;
}

//
//  FUNCTION: Sub_MakeFlag(LSTSTR strFilePath)
//
//  PURPOSE: Make flag
//
LPCSTR Sub_MakeFlag(LPTSTR filename)
{
	HANDLE hFile;
	char *lpFileName = filename;
	char szBuff[1024];
	DWORD dwNumberOfReadBytes;
	hFile = CreateFile(lpFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	ReadFile(hFile, szBuff, sizeof(szBuff) / sizeof(szBuff[0]), &dwNumberOfReadBytes, NULL);
	strncpy_s(flag, 3,szBuff, _TRUNCATE);
	static char *cp = szBuff;
	while (cp = strchr(cp, '\n')) {
		static char tmp[2];
		strncpy_s(tmp, 3, cp+1, _TRUNCATE);
		if (strchr(tmp, '\n') != NULL) { break; }
		if (strchr(tmp, '.') == NULL) {
			strcat_s(flag, 25, tmp);
		}
		cp++;

	}
	CloseHandle(hFile);

	LPCSTR ans = Sub_ConvertSha256(flag);
	return ans;
}



int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.
	int Argc;
	LPWSTR *Argv;
	AllocConsole();
	HANDLE stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	TCHAR  szBuf[256];
	int n = 0;
	int option = 0;

	Argv = CommandLineToArgvW(GetCommandLineW(), &Argc);
	WriteConsole(stdoutHandle, Argv[1], 5, (LPDWORD)&n, NULL);

	option = atoi((LPCSTR)Argv[1]);
	switch (option) {
		case 1:
			lstrcpy(szBuf, TEXT("build & check mode\n"));
			WriteConsole(stdoutHandle, szBuf, lstrlen(szBuf), (LPDWORD)&n, NULL);
			break;

		case 2:
			lstrcpy(szBuf, TEXT("help message!!\n"));
			WriteConsole(stdoutHandle, szBuf, lstrlen(szBuf), (LPDWORD)&n, NULL);
			lstrcpy(szBuf, TEXT("This tctkToy was old fragile and small Windows application toy.. One day, my baby broke it and I forgot how to the repair.\n"));
			WriteConsole(stdoutHandle, szBuf, lstrlen(szBuf), (LPDWORD)&n, NULL);
			lstrcpy(szBuf, TEXT("Please reconstruct my tctkROBO with reverse engineering. It eats the procedure script and can be built.\n"));
			WriteConsole(stdoutHandle, szBuf, lstrlen(szBuf), (LPDWORD)&n, NULL);
			lstrcpy(szBuf, TEXT("This toy somehow cares about its task resources.\n"));
			WriteConsole(stdoutHandle, szBuf, lstrlen(szBuf), (LPDWORD)&n, NULL);
			Sleep(10000);
			exit(0);

		default: /* '?' */
			MessageBox(NULL, "Seek the start-up sequence.", "ERROR", MB_OK);
			exit(1);
	}

	LocalFree(Argv);
	


    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_TCTKTOY, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TCTKTOY));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
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
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TCTKTOY));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_TCTKTOY);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
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
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowExW(WS_EX_ACCEPTFILES,szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   // get init directory status
   GetCurrentDirectory(MAX_PATH + 1, crDir);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//	WM_DROPFILES - drag & drop
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HBITMAP hb;

    switch (message)
    {
	case WM_CREATE:
		DragAcceptFiles(hWnd, TRUE);
		LPCREATESTRUCT lpcs;
		lpcs = (LPCREATESTRUCT)lParam;
		hb = (HBITMAP)LoadBitmap(lpcs->hInstance, MAKEINTRESOURCE(IDB_BITMAP1));

		break;
	case WM_DROPFILES:
		SetForegroundWindow(hWnd);
		DragQueryFile((HDROP)wParam, 0, strFilePath, MAX_PATH);
		HANDLE hThread;
		DWORD dwThreadId;
		hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Sub_Tcl, (LPVOID)strFilePath, 0, &dwThreadId);
		Sleep(1000);
		DragFinish((HDROP)wParam);
		if (Sub_BuildingInstructions(strFilePath))
		{
			LPCSTR flagstr = Sub_MakeFlag(strFilePath);
			char congrats[110] = { '\0' };

			if (strncmp(flagstr, "fail", 4) != 0) {
				snprintf(congrats, sizeof(congrats), "congraturation!! flag is SECCON{%s}", flagstr);
				MessageBox(NULL, congrats, "Complete!", MB_OK);
			}
			else {
				MessageBox(hWnd, "review the order following FINISH view. 'pack' must be used once for each component", "If you cannot pass the flag??", MB_OK);
			}
		}

		break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
			BITMAP bp;
			HBITMAP holdb;
			int width, height;
			HDC mhdc;
            HDC hdc = BeginPaint(hWnd, &ps);
			GetObject(hb, sizeof(BITMAP), &bp);
			width = bp.bmWidth;
			height = bp.bmHeight;
			mhdc = CreateCompatibleDC(hdc);
			holdb = (HBITMAP)SelectObject(mhdc, hb);
			BitBlt(hdc, 0, 0, width, height, mhdc, 0, 0, SRCCOPY);
			DeleteDC(mhdc);
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
		DeleteObject(hb);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
