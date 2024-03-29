/***
 *     ::::::::  :::::::::       ::::    ::::   ::::::::  :::::::::       :::            :::     :::    ::: ::::    :::  ::::::::  :::    ::: :::::::::: :::::::::
 *    :+:    :+: :+:    :+:      +:+:+: :+:+:+ :+:    :+: :+:    :+:      :+:          :+: :+:   :+:    :+: :+:+:   :+: :+:    :+: :+:    :+: :+:        :+:    :+:
 *          +:+  +:+    +:+      +:+ +:+:+ +:+ +:+    +:+ +:+    +:+      +:+         +:+   +:+  +:+    +:+ :+:+:+  +:+ +:+        +:+    +:+ +:+        +:+    +:+
 *        +#+    +#++:++#+       +#+  +:+  +#+ +#+    +:+ +#+    +:+      +#+        +#++:++#++: +#+    +:+ +#+ +:+ +#+ +#+        +#++:++#++ +#++:++#   +#++:++#:
 *      +#+      +#+    +#+      +#+       +#+ +#+    +#+ +#+    +#+      +#+        +#+     +#+ +#+    +#+ +#+  +#+#+# +#+        +#+    +#+ +#+        +#+    +#+
 *     #+#       #+#    #+#      #+#       #+# #+#    #+# #+#    #+#      #+#        #+#     #+# #+#    #+# #+#   #+#+# #+#    #+# #+#    #+# #+#        #+#    #+#
 *    ########## #########       ###       ###  ########  #########       ########## ###     ###  ########  ###    ####  ########  ###    ### ########## ###    ###
 */

#include <Windows.h>
#include <winternl.h>
#include <ntstatus.h>
#include <Tlhelp32.h>
#include <stdio.h>
#include <tchar.h>

#define MAKE_STRING_VERSION(rmaj, rmin) TEXT("v"TEXT(#rmaj)"."TEXT(#rmin))
#define TITLE TEXT("2B Mod Launcher "MAKE_STRING_VERSION(1, 4))
#define NIER_AUTOMATA TEXT("NieRAutomata.exe")
#define STEAMAPPS TEXT("steamapps")
#define STEAM_CONCAT_NIER_AUTOMATA TEXT("\\steamapps\\common\\NieRAutomata\\NieRAutomata.exe")
#define STEAM_CONCAT_LIBRARYFOLDERS_VDF TEXT("\\steamapps\\libraryfolders.vdf")
#define REGISTRY_KEY TEXT("Software\\2B Mod Launcher")
#define SEARCH_WILDCARD TEXT("\\*.dll")
#define INLINE inline
#define REGISTER register
#define STATIC static

#define RANDOMINT(_min, _max) ((rand() % ((_max) - (_min))) + (_min))

#ifdef UNICODE
#define LOADLIBRARY "LoadLibraryW"
#else
#define LOADLIBRARY "LoadLibraryA"
#endif

typedef CLIENT_ID* PCLIENT_ID;

typedef __kernel_entry NTSYSCALLAPI NTSTATUS(NTAPI* NtQuerySystemInformationFn)(
	IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
	OUT PVOID                   SystemInformation,
	IN ULONG                    SystemInformationLength,
	OUT PULONG                  ReturnLength
	);

typedef __kernel_entry NTSYSCALLAPI NTSTATUS(NTAPI* NtQueryInformationProcessFn)(
	IN HANDLE           ProcessHandle,
	IN PROCESSINFOCLASS ProcessInformationClass,
	OUT PVOID           ProcessInformation,
	IN ULONG            ProcessInformationLength,
	OUT PULONG          ReturnLength
	);

typedef __kernel_entry NTSYSCALLAPI NTSTATUS(NTAPI* NtDuplicateObjectFn)(
	IN HANDLE			 SourceProcessHandle,
	IN HANDLE			 SourceHandle,
	IN OPTIONAL HANDLE	 TargetProcessHandle,
	OUT OPTIONAL PHANDLE TargetHandle,
	IN ACCESS_MASK		 DesiredAccess,
	IN BOOLEAN			 InheritHandle,
	IN ULONG			 Options
	);

typedef __kernel_entry NTSYSCALLAPI NTSTATUS(NTAPI* NtOpenProcessFn)(
	OUT PHANDLE           ProcessHandle,
	IN ACCESS_MASK        DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PCLIENT_ID         ClientId
	);


// Fucking an actual useful peb definition not the opaque shit one in winternl.h
struct PEB {
	BYTE Reserved1[2];
	BYTE BeingDebugged;
	BYTE Reserved2[1];
	PVOID Mutant;
	PVOID ImageBaseAddress;
	PPEB_LDR_DATA Ldr;
	PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
	PVOID Reserved4[3];
	PVOID AtlThunkSListPtr;
	PVOID Reserved5;
	ULONG Reserved6;
	PVOID Reserved7;
	ULONG Reserved8;
	ULONG AtlThunkSListPtr32;
	PVOID Reserved9[45];
	BYTE Reserved10[96];
	PPS_POST_PROCESS_INIT_ROUTINE PostProcessInitRoutine;
	BYTE Reserved11[128];
	PVOID Reserved12[1];
	ULONG SessionId;
};

typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO
{
	USHORT UniqueProcessId;
	USHORT CreatorBackTraceIndex;
	UCHAR ObjectTypeIndex;
	UCHAR HandleAttributes;
	USHORT HandleValue;
	PVOID Object;
	ACCESS_MASK GrantedAccess;
} SYSTEM_HANDLE_TABLE_ENTRY_INFO, * PSYSTEM_HANDLE_TABLE_ENTRY_INFO;

typedef struct _SYSTEM_HANDLE_INFORMATION
{
	ULONG NumberOfHandles;
	struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO Handles[ANYSIZE_ARRAY];
} SYSTEM_HANDLE_INFORMATION, * PSYSTEM_HANDLE_INFORMATION;

typedef int EntityHandle;

struct EntityInfo
{
	void* m_pUnknown;							//0x0000 | i don't really know what this is (confirmed a struct pointer)
	char m_szEntityType[32];					//0x0008
	unsigned int m_ObjectId;					//0x0028
	BYTE m_Flags;								//0x002C  | An Entity cannot have this flag or'd with 3 game crashes (possibly a destroyed flag)
	char alignment[3];							//0x002D
	EntityHandle m_hParent;						//0x0030
	char _0x0038[4];							//0x0034
	char* m_pszData[2];							//0x0038
	void* m_pEntity;							//0x0048
	const char** m_pszDat;						//0x0050  | struct pointer
	DWORD* m_pUnk;								//0x0058  | dword array 2 members (0x1415F6B50)
	void* m_pParent;							//0x0060
	BOOL bSetInfo;								//0x0068
	DWORD _0x006C;								//0x006C
	DWORD _0x0070;								//0x0070
};

struct EntityInfoListEntry
{
	EntityHandle m_handle;
	struct EntityInfo* m_pInfo;
};

struct EntityInfoList
{
	DWORD m_dwItems;								//0x0000
	DWORD m_dwSize;									//0x0004
	DWORD m_dwBase;									//0x0008
	DWORD m_dwShift;								//0x000C
	struct EntityInfoListEntry* m_pItems;			//0x0010
	CRITICAL_SECTION m_CriticalSection;				//0x0018
};

typedef struct _USER_NOTIFY
{
	HANDLE hCallerEvent;
	CONST CHAR* szMsg;
	CONST CHAR* szCompletedMsg;
} USER_NOTIFY, * PUSER_NOTIFY;

enum ConsoleColors
{
	BLACK = 0x0,
	BLUE = 0x1,
	GREEN = 0x2,
	AQUA = 0x3,
	RED = 0x4,
	PURPLE = 0x5,
	YELLOW = 0x6,
	WHITE = 0x7,
	GRAY = 0x8,
	LIGHT_BLUE = 0x9,
	LIGHT_GREEN = 0xA,
	LIGHT_AQUA = 0xB,
	LIGHT_RED = 0xC,
	LIGHT_PURPLE = 0xD,
	LIGHT_YELLOW = 0xE,
	PURE_WHITE = 0xF
};

INLINE BOOL FileExists(LPCTSTR szPath);
INLINE BOOL DirectoryExists(LPCTSTR szPath);

void GotoXY(SHORT x, SHORT y)
{
	COORD CursorPosition = { x, y };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), CursorPosition);
}

void SetConsoleColors(enum ConsoleColors foreground, enum ConsoleColors background)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), foreground | (background << 4));
}

void AnimateWait(const char* szIn, HANDLE hWait, DWORD tickrate)
{
	const static char animations[4] = { '|', '/', '-', '\\' };
	INT iSequence = 0;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	COORD now;

	printf(szIn);

	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	now = csbi.dwCursorPosition;

	do {
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
		now = csbi.dwCursorPosition;
		printf("%c", animations[iSequence++]);
		iSequence %= ARRAYSIZE(animations);
		GotoXY(now.X, now.Y);
		Sleep(tickrate);
	} while (WaitForSingleObject(hWait, 0) != WAIT_OBJECT_0);
	printf("\n");
}

void AnimatePrint(const char* szIn, DWORD dwTickrate)
{
	const static char s_animations[4] = { '|', '/', '-', '\\' };
	INT iSequence = 0;
	SIZE_T uLength = strlen(szIn);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	COORD now;

	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	now = csbi.dwCursorPosition;

	for (SIZE_T i = 0; i < uLength; )
	{
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
		now = csbi.dwCursorPosition;
		printf("%c", s_animations[iSequence++]);
		iSequence %= ARRAYSIZE(s_animations);
		GotoXY(now.X, now.Y);

		if (!iSequence)
			printf("%c", szIn[i++]);

		Sleep(dwTickrate);
	}
}

void WINAPI UserNotify(PUSER_NOTIFY pParams)
{
	if (pParams->hCallerEvent == NULL)
		return;

	SetConsoleColors(LIGHT_PURPLE, BLACK);
	AnimateWait(pParams->szMsg, pParams->hCallerEvent, 200);
	SetConsoleColors(GREEN, BLACK);
	printf("%s\n", pParams->szCompletedMsg);
}

ULONG_PTR GetProcessBaseAddress(HANDLE hProcess)
{
	PROCESS_BASIC_INFORMATION ProcBasicInfo;
	struct PEB Peb;
	ULONG Length;

	HMODULE hNtdll = GetModuleHandle(TEXT("ntdll.dll"));

	if (!hNtdll)
		return 0;

	NtQueryInformationProcessFn pNtQueryInformationProcess = (NtQueryInformationProcessFn)GetProcAddress(hNtdll, "NtQueryInformationProcess");

	if (!NT_SUCCESS(pNtQueryInformationProcess(hProcess, ProcessBasicInformation, &ProcBasicInfo, sizeof(PROCESS_BASIC_INFORMATION), &Length)))
		return 0;

	ReadProcessMemory(hProcess, ProcBasicInfo.PebBaseAddress, &Peb, sizeof(struct PEB), NULL);

	return (ULONG_PTR)Peb.ImageBaseAddress;
}

BOOL StartProcess(IN LPCTSTR lpApplicationName, IN LPTSTR lpCommandLine, OPTIONAL OUT LPHANDLE lpProcHandle)
{
	STARTUPINFO StartInfo;
	PROCESS_INFORMATION ProcInfo;
	USER_NOTIFY UserNotifyProcess;

	UserNotifyProcess.hCallerEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
	UserNotifyProcess.szMsg = "Launching NieR:Automata...";
	UserNotifyProcess.szCompletedMsg = "DONE!";

	if (!UserNotifyProcess.hCallerEvent)
		return GetLastError();

	ZeroMemory(&StartInfo, sizeof(STARTUPINFO));

	HANDLE hNotify = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&UserNotify, &UserNotifyProcess, 0, NULL);

	if (!hNotify)
	{
		CloseHandle(UserNotifyProcess.hCallerEvent);
		return GetLastError();
	}

	if (!CreateProcess(lpApplicationName, lpCommandLine, NULL, NULL, FALSE, 0, NULL, NULL, &StartInfo, &ProcInfo))
	{
		CloseHandle(UserNotifyProcess.hCallerEvent);
		CloseHandle(hNotify);
		return GetLastError();
	}
	//WaitForInputIdle(ProcInfo.hProcess, INFINITE);

	while (!FindWindow(TEXT("NieR:Automata_MainWindow"), NULL))
		Sleep(100);

	SetEvent(UserNotifyProcess.hCallerEvent);

	struct EntityInfoList EntityList;
	struct EntityInfoListEntry Entry;
	struct EntityInfo Info;
	BOOL bFoundCelestialSphere = FALSE;

	while (!bFoundCelestialSphere)
	{
		// old denuvo (LPCVOID)0x14160DF88 RVA: 0x160DF88
		ReadProcessMemory(ProcInfo.hProcess, (LPCVOID)(GetProcessBaseAddress(ProcInfo.hProcess) + 0x1029510), 
			&EntityList, sizeof(struct EntityInfoList), NULL);

		for (DWORD i = 0; i < EntityList.m_dwSize; ++i)
		{
			ReadProcessMemory(ProcInfo.hProcess, (LPBYTE)EntityList.m_pItems + i * sizeof(struct EntityInfoListEntry), 
				&Entry, sizeof(struct EntityInfoListEntry), NULL);

			ReadProcessMemory(ProcInfo.hProcess, Entry.m_pInfo, &Info, sizeof(struct EntityInfo), NULL);

			if (Info.m_ObjectId == 0xCA000)
			{
				bFoundCelestialSphere = TRUE;
				break;
			}
		}
	}

	WaitForSingleObject(hNotify, INFINITE);
	CloseHandle(hNotify);
	CloseHandle(UserNotifyProcess.hCallerEvent);
	CloseHandle(ProcInfo.hThread);

	if (lpProcHandle)
		*lpProcHandle = ProcInfo.hProcess;

	return ERROR_SUCCESS;
}

/*

Tries to find NieR:Automata's exe path from the registry

Legit HKEY

Computer\HKEY_CURRENT_USER\Software\Valve\Steam

Illegitamate HKEYS

Computer\HKEY_CURRENT_USER\Software\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Compatibility Assistant\Store		Program Compatibility Assistant
Computer\HKEY_CURRENT_USER\System\GameConfigStore\Children\6c777a49-42c5-4839-b593-d47d6adfb0f4								Win 10 Game Bar/DVR
Computer\HKEY_LOCAL_MACHINE\SYSTEM\ControlSet001\Services\bam\UserSettings\S-1-5-21-2550820505-1424942723-4167368033-1001	Business Activity Monitoring User Settings
*/
LPTSTR QueryRegistryForExePath(IN BOOL bIgnoreOurKey)
{
	HANDLE hFile = INVALID_HANDLE_VALUE;
	HKEY hIllegitimatePath = INVALID_HANDLE_VALUE;
	HKEY hSteamPath = INVALID_HANDLE_VALUE;
	HKEY hOurKey = INVALID_HANDLE_VALUE;
	DWORD dwDisposition;
	DWORD dwType = REG_SZ;
	DWORD dwFileSize;
	DWORD cchData;
	LPTSTR lpData;
	SIZE_T cchNierPath;
	LPTSTR szNierPath;
	DWORD cchVDFPath;
	LPTSTR szVDFPath;
	LPTSTR lpBuffer;
	LPTSTR szNextToken = NULL;
	LPCTSTR szToken;

	if (!bIgnoreOurKey)
	{
		if (!RegCreateKeyEx(HKEY_CURRENT_USER, REGISTRY_KEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hOurKey, &dwDisposition))
		{
			if (dwDisposition == REG_CREATED_NEW_KEY)
			{
				RegSetValueEx(hOurKey, TEXT("ExePath"), 0, REG_SZ, NULL, 0);
				RegCloseKey(hOurKey);
				return QueryRegistryForExePath(TRUE);
			}
			else if (dwDisposition == REG_OPENED_EXISTING_KEY)
			{
				if (RegQueryValueEx(hOurKey, TEXT("ExePath"), NULL, &dwType, NULL, &cchData) == ERROR_NOT_FOUND)
					return NULL;

				lpData = LocalAlloc(LPTR, cchData * sizeof(TCHAR));

				RegQueryValueEx(hOurKey, TEXT("ExePath"), NULL, &dwType, lpData, &cchData);
				RegCloseKey(hOurKey);

				if (!FileExists(lpData))
					return NULL;

				return lpData;
			}
			return NULL;
		}
	}
	else if (!RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Valve\\Steam"), 0, KEY_QUERY_VALUE, &hSteamPath)) // steam 
	{
		/*
		if we land here we want to check the steam apps, and if it's not there read libraryfolders.vdf
		*/

		if (RegQueryValueEx(hSteamPath, TEXT("SteamPath"), NULL, &dwType, NULL, &cchData) == ERROR_NOT_FOUND)
			return NULL;

		/* is this really a warning to be worried about what can RegQueryValueEx return? */
		lpData = LocalAlloc(LMEM_ZEROINIT, cchData * sizeof(TCHAR));

		if (!lpData)
			return NULL;

		RegQueryValueEx(hSteamPath, TEXT("SteamPath"), NULL, &dwType, lpData, &cchData);

		RegCloseKey(hSteamPath);

		cchNierPath = (SIZE_T)(_tcslen(lpData) + _tcslen(STEAM_CONCAT_NIER_AUTOMATA) + sizeof(TCHAR));
		szNierPath = LocalAlloc(LMEM_ZEROINIT, cchNierPath * sizeof(TCHAR));

		if (!szNierPath)
		{
			LocalFree(lpData);
			return NULL;
		}

		_tcscpy_s(szNierPath, cchData, lpData);
		_tcscat_s(szNierPath, cchNierPath, STEAM_CONCAT_NIER_AUTOMATA);

		if (FileExists(lpData))
		{
			return szNierPath;
		}
		else
		{
			cchVDFPath = (DWORD)(_tcslen(lpData) + _tcslen(STEAM_CONCAT_LIBRARYFOLDERS_VDF) + sizeof(TCHAR));
			szVDFPath = LocalAlloc(LMEM_ZEROINIT, cchVDFPath * sizeof(TCHAR));

			if (!szVDFPath)
			{
				LocalFree(lpData);
				LocalFree(szNierPath);
				return NULL;
			}

			_tcscpy_s(szVDFPath, cchData, lpData);
			_tcscat_s(szVDFPath, cchVDFPath, STEAM_CONCAT_LIBRARYFOLDERS_VDF);

			hFile = CreateFile(szVDFPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

			if (hFile == INVALID_HANDLE_VALUE)
			{
				LocalFree(lpData);
				LocalFree(szNierPath);
				LocalFree(szVDFPath);
				return NULL;
			}

			dwFileSize = GetFileSize(hFile, NULL);

			lpBuffer = LocalAlloc(LMEM_ZEROINIT, dwFileSize);

			ReadFile(hFile, lpBuffer, dwFileSize, NULL, NULL);

			do
			{
				szToken = strtok_s(lpBuffer, "\"", &szNextToken);
			} while (!DirectoryExists(szToken)); // crude asf

#if UNICODE
			cchNierPath = (strlen(szToken) + sizeof(STEAM_CONCAT_NIER_AUTOMATA) + 1) * sizeof(WCHAR);
			szNierPath = LocalReAlloc(szNierPath, cchNierPath, LMEM_ZEROINIT);

			if (szNierPath)
			{
				mbstowcs_s(NULL, szNierPath, cchNierPath, szToken, strlen(szToken));
				wcscat_s(szNierPath, sizeof(STEAM_CONCAT_NIER_AUTOMATA), STEAM_CONCAT_NIER_AUTOMATA);
			}
#else 
			SIZE_T cbToken = strlen(szToken) + 1;

			szNierPath = LocalReAlloc(szNierPath, cbToken + sizeof(STEAM_CONCAT_NIER_AUTOMATA), LMEM_ZEROINIT);

			if (szNierPath)
			{
				strcpy_s(szNierPath, cbToken, szToken);
				strcat_s(szNierPath, sizeof(STEAM_CONCAT_NIER_AUTOMATA), STEAM_CONCAT_NIER_AUTOMATA);
			}
#endif // UNICODE
			LocalFree(lpBuffer);
			LocalFree(lpData);
			LocalFree(szVDFPath);
			return szNierPath;
		}
	}
	else if (!RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Compatibility Assistant\\Store"), 0, KEY_QUERY_VALUE, &hIllegitimatePath) ||
		!RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\ControlSet001\\Services\\bam\\UserSettings\\S-1-5-21-2550820505-1424942723-4167368033-1001"), 0, KEY_QUERY_VALUE, &hIllegitimatePath) ||
		!RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("System\\GameConfigStore\\Children\\6c777a49-42c5-4839-b593-d47d6adfb0f4"), 0, KEY_QUERY_VALUE, &hIllegitimatePath))	 // pirated (pretty much a shit show lmao)
	{
		/*
		if we land here we want to enumerate the values until we find the right one and not the debug build (check for steamapps in the path using tcsstr)
		*/

		DWORD dwIndex = 0;
		DWORD cchValueName;
		LPTSTR szValueName = LocalAlloc(LPTR, MAXSHORT);

		if (szValueName)
		{
			while (TRUE)
			{
				cchValueName = MAXSHORT / sizeof(TCHAR);

				if (RegEnumValue(hIllegitimatePath, dwIndex++, szValueName, &cchValueName, NULL, NULL, NULL, NULL))
				{
					LocalFree(szValueName);
					szValueName = NULL;
					break;
				}

				if (!_tcsstr(szValueName, STEAMAPPS) && _tcsstr(szValueName, NIER_AUTOMATA))
					break;
			}

			RegCloseKey(hIllegitimatePath);
			return szValueName;
		}
	}
	return NULL;
}

LSTATUS DeleteRegistryConfig(VOID)
{
	return RegDeleteKey(HKEY_CURRENT_USER, REGISTRY_KEY);
}

LSTATUS WriteRegistryConfig(LPCTSTR szExePath)
{
	LSTATUS Status;
	HKEY hOurKey;

	Status = RegCreateKeyEx(HKEY_CURRENT_USER, REGISTRY_KEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hOurKey, NULL);

	if (!Status)
	{
		RegSetValueEx(hOurKey, TEXT("ExePath"), 0, REG_SZ, szExePath, (DWORD)_tcslen(szExePath));
		RegCloseKey(hOurKey);
	}

	return Status;
}

HANDLE GetProcessHandle(LPCTSTR szProcess)
{
	PROCESSENTRY32 Entry;
	HANDLE hProcess;

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (hSnapshot == INVALID_HANDLE_VALUE)
		return INVALID_HANDLE_VALUE;

	Entry.dwSize = sizeof(PROCESSENTRY32);

	if (!Process32First(hSnapshot, &Entry))
		return INVALID_HANDLE_VALUE;

	do
	{
		if (!_tcsicmp(Entry.szExeFile, szProcess))
		{
#if 0
			NtOpenProcessFn pNtOpenProcess;
			InitializeObjectAttributes()
				CLIENT_ID cid = { Entry.th32ProcessID, 0 };
			pNtOpenProcess(&hProcess, PROCESS_ALL_ACCESS, NULL, &cid);
#endif
			hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, Entry.th32ProcessID);
			CloseHandle(hSnapshot);
			return hProcess;
	}

} while (Process32Next(hSnapshot, &Entry));

CloseHandle(hSnapshot);
return INVALID_HANDLE_VALUE;
}

NTSTATUS SetPrivilege(LPCTSTR szPrivilege, BOOLEAN bState)
{
	HANDLE hToken = INVALID_HANDLE_VALUE;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken))
		return 0;

	TOKEN_PRIVILEGES TokenPrivileges = { 0 };
	TokenPrivileges.PrivilegeCount = 1;
	TokenPrivileges.Privileges[0].Attributes = bState ? SE_PRIVILEGE_ENABLED : 0;

	if (!LookupPrivilegeValue(NULL, szPrivilege, &TokenPrivileges.Privileges[0].Luid))
	{
		CloseHandle(hToken);
		return 0;
	}

	if (!AdjustTokenPrivileges(hToken, FALSE, &TokenPrivileges, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
	{
		CloseHandle(hToken);
		return 0;
	}
	CloseHandle(hToken);
	return STATUS_SUCCESS;
}

HANDLE GetProcessHandleV2(LPCWSTR szProcess)
{
	//HANDLE hDuplicate;
	ULONG Length;

	SetPrivilege(SE_DEBUG_NAME, TRUE);

	HMODULE hNtdll = GetModuleHandle(TEXT("ntdll.dll"));

	if (!hNtdll)
		return 0;

	NtQuerySystemInformationFn pNtQuerySystemInformation = (NtQuerySystemInformationFn)GetProcAddress(hNtdll, "NtQuerySystemInformation");
	NtDuplicateObjectFn pNtDuplicateObject = (NtDuplicateObjectFn)GetProcAddress(hNtdll, "NtDuplicateObject");
	NtOpenProcessFn pNtOpenProcess = (NtOpenProcessFn)GetProcAddress(hNtdll, "NtOpenProcess");

	if (pNtQuerySystemInformation)
	{
		Length = 256 * sizeof(SYSTEM_PROCESS_INFORMATION);
		PSYSTEM_PROCESS_INFORMATION pSysProcInfo = LocalAlloc(LPTR, Length);

		while (pNtQuerySystemInformation(SystemProcessInformation, pSysProcInfo, Length, &Length) == STATUS_INFO_LENGTH_MISMATCH) //revise NT_SUCCESS() s == infomismatch
			if (!(pSysProcInfo = LocalReAlloc(pSysProcInfo, Length, LMEM_MOVEABLE)))
				return INVALID_HANDLE_VALUE;

		if (pSysProcInfo)
		{
			for (; pSysProcInfo->NextEntryOffset; pSysProcInfo = (PSYSTEM_PROCESS_INFORMATION)((LPBYTE)pSysProcInfo + pSysProcInfo->NextEntryOffset))
			{
				if (pSysProcInfo->ImageName.Buffer && !wcscmp(pSysProcInfo->ImageName.Buffer, szProcess))
				{
					HANDLE hProcess;
					OBJECT_ATTRIBUTES attr;
					InitializeObjectAttributes(&attr, NULL, 0, 0, NULL);
					CLIENT_ID cid = { pSysProcInfo->UniqueProcessId, 0 };
					NTSTATUS a = pNtOpenProcess(&hProcess, PROCESS_ALL_ACCESS, &attr, &cid);
					return hProcess;
					//Length = sizeof(ULONG) + 256 * sizeof(SYSTEM_HANDLE_TABLE_ENTRY_INFO);
					//PSYSTEM_HANDLE_INFORMATION pSysHandleInfo = LocalAlloc(LPTR, Length);
					////SystemHandleInformation = 16
					//while (pNtQuerySystemInformation(16, pSysHandleInfo, Length, &Length) == STATUS_INFO_LENGTH_MISMATCH)
					//{
					//	if (!(pSysHandleInfo = LocalReAlloc(pSysHandleInfo, Length, LMEM_MOVEABLE)))
					//	{
					//		LocalFree(pSysProcInfo);
					//		return INVALID_HANDLE_VALUE;
					//	}
					//}

					//if (pSysHandleInfo)
					//{
					//	for (ULONG Index = 0; Index < pSysHandleInfo->NumberOfHandles; ++Index)
					//	{
					//		PSYSTEM_HANDLE_TABLE_ENTRY_INFO pSysHandleEntry = &pSysHandleInfo->Handles[Index];
					//		if (pSysHandleEntry->UniqueProcessId == pSysProcInfo->UniqueProcessId)
					//		{
					//			
					//			
					//			if (PROCESS_TERMINATE & pSysHandleEntry->GrantedAccess)
					//				TerminateProcess(pSysHandleEntry->HandleValue, 22);


					//			return pSysHandleEntry->HandleValue; // won't work because we don't own the handle??
					//			//pNtDuplicateObject(, &pSysHandleEntry->HandleValue, GetCurrentProcess(), &hDuplicate, , , DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);
					//		}
					//	}
					//}
				}
			}
		}
	}
	return INVALID_HANDLE_VALUE;
}

HINSTANCE GetForeignModuleHandle(HANDLE hProcess, LPCTSTR szModule)
{
	MODULEENTRY32 Entry;

	if (hProcess == INVALID_HANDLE_VALUE)
		return NULL;

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetProcessId(hProcess));

	if (hSnapshot == INVALID_HANDLE_VALUE)
		return NULL;

	Entry.dwSize = sizeof(MODULEENTRY32);

	if (!Module32First(hSnapshot, &Entry))
		return NULL;

	do
	{
		if (!_tcsicmp(Entry.szExePath, szModule))
		{
			CloseHandle(hSnapshot);
			return Entry.hModule;
		}

	} while (Module32Next(hSnapshot, &Entry));

	CloseHandle(hSnapshot);
	return NULL;
}

HINSTANCE GetForeignModuleHandleV2(HANDLE hProcess, LPCWSTR szModule)
{
	PROCESS_BASIC_INFORMATION ProcInfo;
	ULONG Length;
	PEB Peb;
	PEB_LDR_DATA Ldr;
	LDR_DATA_TABLE_ENTRY Data;
	PLIST_ENTRY pNext;
	INT iCompare;

	HMODULE hNtdll = GetModuleHandle(TEXT("ntdll.dll"));

	if (!hNtdll)
		return NULL;

	NtQueryInformationProcessFn pNtQueryInformationProcess = (NtQueryInformationProcessFn)GetProcAddress(hNtdll, "NtQueryInformationProcess");

	if (hProcess != INVALID_HANDLE_VALUE && pNtQueryInformationProcess)
	{
		if (NT_SUCCESS(pNtQueryInformationProcess(hProcess, ProcessBasicInformation, &ProcInfo, sizeof(PROCESS_BASIC_INFORMATION), &Length)))
		{
			ReadProcessMemory(hProcess, ProcInfo.PebBaseAddress, &Peb, sizeof(PEB), NULL);
			ReadProcessMemory(hProcess, Peb.Ldr, &Ldr, sizeof(PEB_LDR_DATA), NULL);
			pNext = Ldr.InMemoryOrderModuleList.Flink;

			if (!pNext) return NULL;

			do
			{
				ReadProcessMemory(hProcess, CONTAINING_RECORD(pNext, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks), &Data, sizeof(LDR_DATA_TABLE_ENTRY), NULL);
				ReadProcessMemory(hProcess, &pNext->Flink, &pNext, sizeof(PLIST_ENTRY), NULL);
				iCompare = wcscmp(Data.FullDllName.Buffer, szModule);
			} while (iCompare && pNext);

			return (!iCompare) ? Data.DllBase : NULL;
		}
	}
	return NULL;
}

BOOL LoadDLL(IN HANDLE hProcess, IN LPCTSTR szDLL, OPTIONAL OUT HINSTANCE* phInstance)
{
	SIZE_T nBytesWritten;

	if (hProcess == INVALID_HANDLE_VALUE)
		return ERROR_INVALID_HANDLE;

	SIZE_T nAllocationBytes = (_tcslen(szDLL) + 1) * sizeof(TCHAR);
	LPWSTR szInternalDLL = VirtualAllocEx(hProcess, NULL, nAllocationBytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	if (!szInternalDLL)
		return GetLastError();

	_tprintf(TEXT("ALLOC OK!\n%lld bytes allocated!\n"), nAllocationBytes);

	if (WriteProcessMemory(hProcess, szInternalDLL, szDLL, nAllocationBytes, &nBytesWritten))
	{
		_tprintf(TEXT("MEMORY OK!\n%lld bytes written!\n"), nBytesWritten);
	}
	else
	{
		VirtualFreeEx(hProcess, szInternalDLL, 0, MEM_RELEASE);
		return GetLastError();
	}

	HMODULE hKernel32 = GetModuleHandle(TEXT("kernel32.dll"));

	if (!hKernel32)
	{
		VirtualFreeEx(hProcess, szInternalDLL, 0, MEM_RELEASE);
		return GetLastError();
	}

	HANDLE hThread = CreateRemoteThreadEx(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, LOADLIBRARY),
		szInternalDLL, 0, NULL, NULL);

	if (!hThread)
	{
		VirtualFreeEx(hProcess, szInternalDLL, 0, MEM_RELEASE);
		return GetLastError();
	}

	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	VirtualFreeEx(hProcess, szInternalDLL, 0, MEM_RELEASE);

	if (phInstance)
		*phInstance = GetForeignModuleHandle(hProcess, szDLL);

	return ERROR_SUCCESS;
}

BOOL LoadBinaries(IN HANDLE hProcess, IN LPCTSTR szDirectory, IN BOOL(*callback)(HANDLE, LPCTSTR, HINSTANCE*))
{
	WIN32_FIND_DATA Find;
	HINSTANCE hInstance;
	DWORD dwError;
	TCHAR szSearchDirectory[MAX_PATH];
	TCHAR szDLLPath[MAX_PATH];
	BOOL Status;
	BOOL bEmpty = TRUE;

	if (!szDirectory || !callback)
		return ERROR_INVALID_PARAMETER;

	if (hProcess == INVALID_HANDLE_VALUE)
		return ERROR_INVALID_HANDLE;

	if (_tcslen(szDirectory) > (MAX_PATH - ARRAYSIZE(SEARCH_WILDCARD)))
	{
		_tprintf(TEXT("Directory path is too long.\n"));
		return ERROR_DIRECTORY;
	}

	//prep the directory string for FindFirstFileW
	_tcscpy_s(szSearchDirectory, MAX_PATH, szDirectory);
	_tcscat_s(szSearchDirectory, MAX_PATH, SEARCH_WILDCARD);

	HANDLE hFind = FindFirstFile(szSearchDirectory, &Find);

	if (hFind == INVALID_HANDLE_VALUE)
		return GetLastError();

	do
	{
		if (bEmpty)
			bEmpty = FALSE;

		_tcsncpy_s(szDLLPath, MAX_PATH, szSearchDirectory, _tcslen(szSearchDirectory) - ARRAYSIZE(SEARCH_WILDCARD) + 2);
		_tcscat_s(szDLLPath, MAX_PATH, Find.cFileName);

		Status = callback(hProcess, szDLLPath, &hInstance);
		Status ? _tprintf(TEXT("%s failed to load with errorcode %x!\n"), Find.cFileName, Status) : _tprintf(TEXT("Loaded %s @ %llx\n"), Find.cFileName, (ULONGLONG)hInstance);
	} while (FindNextFile(hFind, &Find));

	FindClose(hFind);

	if (bEmpty)
	{
		_tprintf(TEXT("Directory empty! No mods to load!\n"));
		return ERROR_NO_MORE_FILES;
	}

	dwError = GetLastError();

	return (dwError == ERROR_NO_MORE_FILES) ? ERROR_SUCCESS : dwError;
}

INLINE BOOL FileExists(LPCTSTR szPath)
{
	return (GetFileAttributes(szPath) != INVALID_FILE_ATTRIBUTES);
}

INLINE BOOL DirectoryExists(LPCTSTR szPath)
{
	REGISTER DWORD dwAttributes = GetFileAttributes(szPath);
	return ((dwAttributes != INVALID_FILE_ATTRIBUTES) && (dwAttributes & FILE_ATTRIBUTE_DIRECTORY));
}

BOOL SanitizePath(IN LPCTSTR szDelimiter, IN LPTSTR szOriginalPath, IN SIZE_T cchOriginalPath, OUT LPTSTR szSanitizedPath, IN SIZE_T cchSanitizedPath)
{
	LPTSTR szToken;
	LPTSTR szNextToken = NULL;

	LPTSTR szTemporary = NULL;
	LPTSTR szDirPath = (LPTSTR)LocalAlloc(LPTR, (cchOriginalPath + 1) * sizeof(TCHAR));
	BOOL Status = ERROR_DIRECTORY;

	if (szDirPath)
	{
		_tcscpy_s(szDirPath, cchOriginalPath, szOriginalPath);
		szToken = _tcstok_s(szOriginalPath, szDelimiter, &szNextToken);

		while (szToken)
		{
			szTemporary = szToken;
			szToken = _tcstok_s(NULL, szDelimiter, &szNextToken);
		}

		if (szTemporary)
		{
			szDirPath[szTemporary - szOriginalPath] = (TCHAR)0;

			if (szSanitizedPath)
				_tcscpy_s(szSanitizedPath, cchSanitizedPath, szDirPath);

			Status = ERROR_SUCCESS;
		}
		LocalFree((HLOCAL)szDirPath);
	}
	else
	{
		Status = ERROR_OUTOFMEMORY;
	}

	return Status;
}

VOID Welcome(VOID)
{
	RECT Rect;
	HWND hConsole = GetConsoleWindow();

	GetWindowRect(hConsole, &Rect);
	SetWindowPos(hConsole, HWND_TOPMOST, Rect.left, Rect.top, (INT)(GetSystemMetrics(SM_CXSCREEN) * 0.55f), (INT)(GetSystemMetrics(SM_CYSCREEN) * 0.5f), 0);

	SetConsoleTitle(TITLE);

	srand(GetTickCount64() >> 32);

	switch (RANDOMINT(0, 2))
	{
	case 0:
		// no 1
		_tprintf(
			TEXT("   __  _ __    _ _ _            __                               \n")
			TEXT("     )( /  )  ( / ) )      /   ( /                      /        \n")
			TEXT(" .--'  /--<    / / / __ __/     /    __,  , , _ _   _, /_  _  _  \n")
			TEXT("(__   /___/   / / (_(_)(_/_   (/___/(_/(_(_/_/ / /_(__/ /_(/_/ (_\n")
			TEXT("                                                                 \n")
			TEXT("                                                                 \n")
			TEXT("\n\n")
		);
		break;
	case 1:
		//no 2		
		_tprintf(
			TEXT(" ::::::::  :::::::::       ::::    ::::   ::::::::  :::::::::       :::            :::     :::    ::: ::::    :::  ::::::::  :::    ::: :::::::::: ::::::::: \n")
			TEXT(":+:    :+: :+:    :+:      +:+:+: :+:+:+ :+:    :+: :+:    :+:      :+:          :+: :+:   :+:    :+: :+:+:   :+: :+:    :+: :+:    :+: :+:        :+:    :+:\n")
			TEXT("      +:+  +:+    +:+      +:+ +:+:+ +:+ +:+    +:+ +:+    +:+      +:+         +:+   +:+  +:+    +:+ :+:+:+  +:+ +:+        +:+    +:+ +:+        +:+    +:+\n")
			TEXT("    +#+    +#++:++#+       +#+  +:+  +#+ +#+    +:+ +#+    +:+      +#+        +#++:++#++: +#+    +:+ +#+ +:+ +#+ +#+        +#++:++#++ +#++:++#   +#++:++#: \n")
			TEXT("  +#+      +#+    +#+      +#+       +#+ +#+    +#+ +#+    +#+      +#+        +#+     +#+ +#+    +#+ +#+  +#+#+# +#+        +#+    +#+ +#+        +#+    +#+\n")
			TEXT(" #+#       #+#    #+#      #+#       #+# #+#    #+# #+#    #+#      #+#        #+#     #+# #+#    #+# #+#   #+#+# #+#    #+# #+#    #+# #+#        #+#    #+#\n")
			TEXT("########## #########       ###       ###  ########  #########       ########## ###     ###  ########  ###    ####  ########  ###    ### ########## ###    ###\n")
			TEXT("\n\n")
		);

		break;
	case 2:
		//no 3
		_tprintf(
			TEXT(" _______  _______    __   __  _______  ______     ___      _______  __   __  __    _  _______  __   __  _______  ______   \n")
			TEXT("|       ||  _    |  |  |_|  ||       ||      |   |   |    |   _   ||  | |  ||  |  | ||       ||  | |  ||       ||    _ |  \n")
			TEXT("|____   || |_|   |  |       ||   _   ||  _    |  |   |    |  |_|  ||  | |  ||   |_| ||       ||  |_|  ||    ___||   | ||  \n")
			TEXT(" ____|  ||       |  |       ||  | |  || | |   |  |   |    |       ||  |_|  ||       ||       ||       ||   |___ |   |_||_ \n")
			TEXT("| ______||  _   |   |       ||  |_|  || |_|   |  |   |___ |       ||       ||  _    ||      _||       ||    ___||    __  |\n")
			TEXT("| |_____ | |_|   |  | ||_|| ||       ||       |  |       ||   _   ||       || | |   ||     |_ |   _   ||   |___ |   |  | |\n")
			TEXT("|_______||_______|  |_|   |_||_______||______|   |_______||__| |__||_______||_|  |__||_______||__| |__||_______||___|  |_|\n")
			TEXT("\n\n")
		);
		break;
	}
}

INLINE VOID WaitOnUserInput(VOID)
{
	_tprintf(TEXT("Press enter to continue...\n"));
	getchar();
}


//  process handle, look for config file, registry, assumption, ask the user and save the result for the config
int main(int argc, char** argv)
{
	HANDLE hNierAutomata;
	TCHAR szLauncherPath[MAX_PATH];
	TCHAR szExePath[MAX_PATH];
	TCHAR szDirectoryPath[MAX_PATH];
	DWORD dwPathSize = MAX_PATH;
	BOOL Status = ERROR_SUCCESS;

	Welcome();

	if (argc > 1 && !_stricmp(argv[1], "-u"))
	{
		if (!DeleteRegistryConfig())
		{
			_tprintf(TEXT("Successfully uninstalled!"));
		}
		else
		{
			LPTSTR szError;
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, 0, 0, (LPTSTR)&szError, 0, NULL);
			_tprintf(TEXT("An error occured when uninstalling.\n %s"), szError);
			LocalFree(szError);
		}
		WaitOnUserInput();
		return Status;
	}

	hNierAutomata = GetProcessHandle(NIER_AUTOMATA);  //GetProcessHandleV2(L"NieRAutomata.exe"); 

	if (hNierAutomata == INVALID_HANDLE_VALUE)
	{
		LPTSTR szRegExePath = QueryRegistryForExePath(FALSE);

		//if (!szRegExePath)
		//	szRegExePath = QueryRegistryForExePath(TRUE); //search the steam key

		// Last attempt to find the exe path. Assume the launcher is in the same folder as the game
		if (!szRegExePath)
		{
			_tcscpy_s(szLauncherPath, MAX_PATH, argv[0]);
			SanitizePath(TEXT("\\"), szLauncherPath, MAX_PATH, szExePath, MAX_PATH);
			_tcscat_s(szExePath, MAX_PATH, NIER_AUTOMATA);

			// rifk ask the user
			if (!FileExists(szExePath))
			{
				OPENFILENAME ofn = { 0 };
				ofn.dwReserved = 0;
				ofn.Flags = OFN_FORCESHOWHIDDEN | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
				ofn.FlagsEx = 0;
				ofn.hInstance = GetModuleHandle(NULL);
				ofn.hwndOwner = GetConsoleWindow();
				ofn.lCustData = 0;
				ofn.lpfnHook = (LPOFNHOOKPROC)DefDlgProc;
				ofn.lpstrDefExt = "exe";
				ofn.lpstrFile = szExePath;
				ofn.lpstrFileTitle = NULL;
				ofn.lpstrFilter = TEXT("Portable Executable (.EXE)\0 *.EXE");
				ofn.nFilterIndex = 1;
				ofn.lpstrInitialDir = NULL;
				ofn.lpstrTitle = TEXT("2B Mod Launcher ~ Please provide the NieR:Automata executable to continue");
				ofn.lpTemplateName = NULL;
				ofn.lStructSize = sizeof(OPENFILENAME);
				ofn.nFileExtension = 0;
				ofn.nFileOffset = 0;
				ofn.nMaxCustFilter = 0x40;
				ofn.nMaxFile = sizeof(TCHAR[MAX_PATH]);
				ofn.nMaxFileTitle = 0;
				ofn.pvReserved = NULL;

				GetOpenFileName(&ofn);
			}
		}
		else
		{
			_tcscpy_s(szExePath, MAX_PATH, szRegExePath);
			LocalFree(szRegExePath);
		}

		if (StartProcess(szExePath, NULL, &hNierAutomata))
		{
			_tprintf(TEXT("Could not start NieR:Automata!\n"));
			WaitOnUserInput();
			return ERROR_INVALID_HANDLE;
		}
	}
	else
	{
		if (!QueryFullProcessImageName(hNierAutomata, 0, szExePath, &dwPathSize))
		{
			DWORD dwError = GetLastError();
			_tprintf(TEXT("Could not query path! %x\n"), dwError);
			return dwError;
		}
	}

	WriteRegistryConfig(szExePath);

	if (!SanitizePath(TEXT("\\"), szExePath, MAX_PATH, szDirectoryPath, MAX_PATH))
	{
		_tcscat_s(szDirectoryPath, MAX_PATH, TEXT("mods"));

		if (!DirectoryExists(szDirectoryPath))
		{
			CreateDirectory(szDirectoryPath, NULL);
			_tprintf(TEXT("Missing Mods Folder!\nCreated Mods Folder!\nTo ensure that this doesn't happen again, please verify that the mods folder exists before launching the launcher.\n"));
		}

		Status = LoadBinaries(hNierAutomata, szDirectoryPath, &LoadDLL);
	}
	else
	{
		_tprintf(TEXT("Mods folder path is malformed!\n"));
	}

	// Close the original ProcInfo.hProccess HANDLE now that we are done using it
	CloseHandle(hNierAutomata);
	WaitOnUserInput();

	return Status;
}