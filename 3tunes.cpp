#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <commctrl.h>

#pragma comment(lib, "comctl32")

#define IDI_ICON 500

#define TIMER_SECS 1

#define FIRST_SPLIT " by "
#define SECOND_SPLIT " - Pandora"

LRESULT CALLBACK WindowProcedure(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK EnumChildProc(HWND m_hWndChild, LPARAM m_lParam);

void SetupListView(void);
void FindTempFolder(void);
void GetTrackInfo(char* m_pszWindowText);
void CopyMP3File(char* m_pszArtist, char* m_pszTitle, int m_iAccess);
void AddTrackInfo(char* m_pszArtist, char* m_pszTitle, int m_iAccess);

char g_szClassName[] = "3tunes";

char g_pszTitlebarPrevious[1024] = {0};
char g_pszTitlebarCurrent[1024] = {0};
char g_pszArtist[256] = {0};
char g_pszTitle[256] = {0};
char g_pszCurrentlyPlaying[1024] = {0};
char g_pszSystemDrive[256] = {0};
char g_pszTempFolder[256] = {0};

int g_statwidths[] = {-1};
int i = 0;
int y = 0;

HWND g_hwnd;
HWND g_hwndMP3List;
HWND g_hwndStatusBar;
HWND g_hwndTitleBar;

// COLORREF rgbBackground = RGB(220, 227, 232); Ready for new UI
// CreateSolidBrush(rgbBackground); Ready for new UI

int WINAPI WinMain(HINSTANCE hThisInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int nFunsterStil)
{
	MSG messages;
	WNDCLASSEX wincl;

	wincl.hInstance	= hThisInstance;
	wincl.lpszClassName	= g_szClassName;
	wincl.lpfnWndProc = WindowProcedure;
	wincl.style = CS_DBLCLKS;
	wincl.cbSize = sizeof(WNDCLASSEX);
	wincl.hIcon = (HICON) LoadImage(hThisInstance, MAKEINTRESOURCE(IDI_ICON), IMAGE_ICON, 32, 32, 0);
	wincl.hIconSm = (HICON) LoadImage(hThisInstance, MAKEINTRESOURCE(IDI_ICON), IMAGE_ICON, 32, 32, 0);
	wincl.hCursor = LoadCursor(NULL, IDC_ARROW);
	wincl.lpszMenuName = NULL;
	wincl.cbClsExtra = 0;
	wincl.cbWndExtra = 0;
	wincl.hbrBackground =  (HBRUSH)(COLOR_BTNFACE+1); 

	if(!RegisterClassEx(&wincl)) 
	{
		return 0;
	}

	InitCommonControls();

	g_hwnd = CreateWindowEx(
		0, 
		g_szClassName, 
		"3tunes", 
		WS_OVERLAPPEDWINDOW, 
		CW_USEDEFAULT, 
		CW_USEDEFAULT, 
		400, 
		300, 
		HWND_DESKTOP, 
		NULL, 
		hThisInstance, 
		NULL);

	g_hwndMP3List = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		WC_LISTVIEW,
		NULL,
		WS_CHILD | WS_VISIBLE |  WS_BORDER | LVS_REPORT | LVS_NOLABELWRAP | WS_CLIPSIBLINGS | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
		5,
		5,
		380,
		245,
		g_hwnd,
		(HMENU)NULL,
		hThisInstance,
		NULL);

	SetupListView();

	g_hwndStatusBar = CreateWindowEx(
		0,
		STATUSCLASSNAME,
		NULL,
		WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
		0,
		0,
		0,
		0,
		g_hwnd,
		(HMENU)NULL,
		GetModuleHandle(NULL),
		NULL);

	EnumChildWindows(GetDesktopWindow(), EnumChildProc, (LPARAM) GetDesktopWindow());
	GetWindowText(g_hwndTitleBar, g_pszTitlebarPrevious, sizeof(g_pszTitlebarPrevious) - 1);
	ExpandEnvironmentStrings("%SYSTEMDRIVE%", g_pszSystemDrive, sizeof(g_pszSystemDrive) - 1);

	FindTempFolder();

	SendMessage(g_hwndStatusBar, SB_SETPARTS, sizeof(g_statwidths)/sizeof(int), (LPARAM)g_statwidths);
	SendMessage(g_hwndStatusBar, SB_SETTEXT, (WPARAM) 0, (LPARAM) "Finding current song...");

	ShowWindow(g_hwnd, SW_SHOW);
	UpdateWindow(g_hwnd);

	SetTimer(g_hwnd, TIMER_SECS, 10, NULL);

	while(GetMessage(&messages, NULL, 0, 0))
	{
		TranslateMessage(&messages);
		DispatchMessage(&messages);
	}

	return messages.wParam;
}

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			break;
		}

		case WM_SIZE:
		{
			int Width = LOWORD(lParam);
			int Height = HIWORD(lParam);

			MoveWindow(g_hwndMP3List, 5, 5, Width-10, Height-30, TRUE);
			MoveWindow(g_hwndStatusBar, 0, 0, 0, 0, TRUE);
			break;
		}

		case WM_COMMAND:
		{
			break;
		}

		case WM_TIMER:
		{
			switch (wParam)
			{
				case TIMER_SECS:
				{
					GetWindowText(g_hwndTitleBar, g_pszTitlebarCurrent, sizeof(g_pszTitlebarCurrent) - 1);

					if (strstr(g_pszTitlebarCurrent, "Pandora - Find New Music, Listen to Custom Internet Radio Stations") == NULL)
					{
						KillTimer(hwnd, TIMER_SECS);
						SendMessage(g_hwndStatusBar, SB_SETTEXT, (WPARAM) 0, (LPARAM) "Unable to find the Pandora window.");
					}
					else if (!IsWindowVisible(g_hwndTitleBar))
					{
						KillTimer(hwnd, TIMER_SECS);
						SendMessage(g_hwndStatusBar, SB_SETTEXT, (WPARAM) 0, (LPARAM) "Unable to find the Pandora window.");
					}
					else if (strstr(g_pszTitlebarCurrent, "(Paused)") != NULL)
					{
						SendMessage(g_hwndStatusBar, SB_SETTEXT, (WPARAM) 0, (LPARAM) "Current song is paused.");
					}
					else if (strcmp(g_pszTitlebarPrevious, g_pszTitlebarCurrent) == 0)
					{
						GetTrackInfo(g_pszTitlebarCurrent);
						sprintf(g_pszCurrentlyPlaying, "Currently Playing: %s by %s", g_pszTitle, g_pszArtist);
						SendMessage(g_hwndStatusBar, SB_SETTEXT, (WPARAM) 0, (LPARAM) g_pszCurrentlyPlaying);
						strcpy(g_pszTitlebarPrevious, g_pszTitlebarCurrent);
					}
					else
					{
						GetTrackInfo(g_pszTitlebarPrevious);
						CopyMP3File(g_pszArtist, g_pszTitle, i);
						AddTrackInfo(g_pszArtist, g_pszTitle, i);
						strcpy(g_pszTitlebarPrevious, g_pszTitlebarCurrent);
						i++;
					}

					memset(g_pszArtist, 0, sizeof(g_pszArtist));
					memset(g_pszTitle, 0, sizeof(g_pszTitle));
					memset(g_pszCurrentlyPlaying, 0, sizeof(g_pszCurrentlyPlaying));
				}

				break;
			}
		}

		default:
		{
			return DefWindowProc(hwnd, message, wParam, lParam);
		}
	}

	return 0;
}

BOOL CALLBACK EnumChildProc(HWND m_hWndChild, LPARAM m_lParam)
{
	char m_pszTitlebarText[1024];

	GetWindowText(m_hWndChild, m_pszTitlebarText, sizeof(m_pszTitlebarText) - 1);

	if (strstr(m_pszTitlebarText, "Pandora - Find New Music, Listen to Custom Internet Radio Stations") == NULL)
	{
		return true;
	}
	else
	{
		g_hwndTitleBar = m_hWndChild;
		return false;
	}
}

void SetupListView(void)
{
	LVCOLUMN LvCol;

	memset(&LvCol, 0, sizeof(LvCol));

	LvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	LvCol.pszText = "#";
	LvCol.cx = 20;
	SendMessage(g_hwndMP3List, LVM_INSERTCOLUMN, 0, (LPARAM) &LvCol);

	LvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	LvCol.pszText = "Artist";
	LvCol.cx = 178;
	SendMessage(g_hwndMP3List, LVM_INSERTCOLUMN, 1, (LPARAM) &LvCol);

	LvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	LvCol.pszText = "Title";
	LvCol.cx = 178;
	SendMessage(g_hwndMP3List, LVM_INSERTCOLUMN, 2, (LPARAM) &LvCol);

	SendMessage(g_hwndMP3List, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
}

void FindTempFolder(void)
{
	char m_pszBuffer[256] = {0};
	char m_pszFileBuff[256] = {0};
	char m_pszUsername[256] = {0};
	DWORD m_nUsername = sizeof(m_pszUsername);
	FILE* m_pFile;
	bool m_FoundFolder = false;

	GetUserName(m_pszUsername, &m_nUsername);

	while (m_FoundFolder == false)
	{
		if (y == 0)
		{
			sprintf(m_pszBuffer, "%s\\Documents and Settings\\%s\\Local Settings\\Temp\\plugtmp", g_pszSystemDrive, m_pszUsername);
		}
		else
		{
			sprintf(m_pszBuffer, "%s\\Documents and Settings\\%s\\Local Settings\\Temp\\plugtmp-%d", g_pszSystemDrive, m_pszUsername, y);
		}

		sprintf(m_pszFileBuff, "%s\\access", m_pszBuffer);

		m_pFile = fopen(m_pszFileBuff, "rb");

		if (m_pFile == NULL)
		{
			m_FoundFolder = false;
			y++;
		}
		else
		{
			fclose(m_pFile);
			strcpy(g_pszTempFolder, m_pszBuffer);
			m_FoundFolder = true;
		}
	}
}

void GetTrackInfo(char* m_pszWindowText)
{
	char* m_pszIndex = NULL;
	char m_pszTmpArtist[256] = {0};
	char m_pszTmpTitle[256] = {0};
	char m_pszBuff[256] = {0};
	int m_iChars = 0;

	if ((m_pszIndex = strstr(m_pszWindowText, FIRST_SPLIT)) != NULL)
	{
		m_iChars = (m_pszIndex - m_pszWindowText);
		strncpy(m_pszTmpTitle, m_pszWindowText, m_iChars);

		m_pszIndex += strlen(FIRST_SPLIT);
		strncpy(m_pszBuff, m_pszIndex, sizeof(m_pszBuff));
	}

	m_pszIndex = NULL;
	m_iChars = 0;

	if ((m_pszIndex = strstr(m_pszBuff, SECOND_SPLIT)) != NULL)
	{
		m_iChars = (m_pszIndex - m_pszBuff);
		strncpy(m_pszTmpArtist, m_pszBuff, m_iChars);
	}

	strcpy(g_pszArtist, &m_pszTmpArtist[1]);
	strncpy(g_pszTitle, m_pszTmpTitle, strlen(m_pszTmpTitle)-1);
}

void CopyMP3File(char* m_pszArtist, char* m_pszTitle, int m_iAccess)
{
	char m_pszNewPath[256] = {0};
	char m_pszAccess[256] = {0};
	
	if (m_iAccess == 0)
	{
		sprintf(m_pszAccess, "%s\\access", g_pszTempFolder);
	}
	else
	{
		sprintf(m_pszAccess, "%s\\access-%d", g_pszTempFolder, m_iAccess);
	}

	sprintf(m_pszNewPath, "%s\\%s - %s.mp3", g_pszSystemDrive, m_pszArtist, m_pszTitle);
	CopyFile(m_pszAccess, m_pszNewPath, FALSE);
}

void AddTrackInfo(char* m_pszArtist, char* m_pszTitle, int m_iAccess)
{
	LVITEM LvItem;
	char m_pszBuff[256] = {0};
	int y = m_iAccess + 1;

	itoa(y, m_pszBuff, 10);

	LvItem.mask=LVIF_TEXT;
	LvItem.iItem = m_iAccess;
	LvItem.iSubItem = 0;
	LvItem.pszText = m_pszBuff;

	SendMessage(g_hwndMP3List, LVM_INSERTITEM, 0, (LPARAM) &LvItem);

	LvItem.mask = LVIF_TEXT;
	LvItem.iSubItem = 1;
	LvItem.pszText = m_pszArtist;
	SendMessage(g_hwndMP3List, LVM_SETITEM, 0, (LPARAM) &LvItem);

	LvItem.mask = LVIF_TEXT;
	LvItem.iSubItem = 2;
	LvItem.pszText = m_pszTitle;
	SendMessage(g_hwndMP3List, LVM_SETITEM, 0, (LPARAM) &LvItem);
}
