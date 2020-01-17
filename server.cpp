#include <windows.h>
#include <stdio.h>
#include <process.h>
#include "pipe.h"
#include "ZLPT.h"
#include <tchar.h>
#include <strsafe.h>


//mae

DWORD WINAPI PipeIO(LPVOID lpvParam);

int _tmain(VOID)
{
	HANDLE hPipe;
	SECURITY_ATTRIBUTES sa;
	SECURITY_DESCRIPTOR sd;

	if (InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION) == 0) {
		printf("InitializeSecurityDescriptor failed with error %d\n", GetLastError());
		return 1;
	}
	if (SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE) == 0) {
		printf("SetSecurityDescriptorDacl failed with error %d\n", GetLastError());
		return 1;
	}
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = &sd;
	sa.bInheritHandle = TRUE;

	BOOL   fConnected = FALSE;
	DWORD  dwThreadId = 0;
	HANDLE hPipe = INVALID_HANDLE_VALUE, hThread = NULL;
	LPCTSTR lpszPipename = TEXT("\\\\.\\pipe\\zls");

	for (;;)
	{
		_tprintf(TEXT("\nPipe Server: Main thread awaiting client connection on %s\n"), lpszPipename);
		hPipe = CreateNamedPipe(
			lpszPipename,            
			PIPE_ACCESS_DUPLEX,       
			PIPE_TYPE_MESSAGE |        
			PIPE_READMODE_MESSAGE |   
			PIPE_WAIT,                
			PIPE_UNLIMITED_INSTANCES, 
			PROTOCOL_SIZE,
			PROTOCOL_SIZE,
			0,                       
			NULL);                    

		if (hPipe == INVALID_HANDLE_VALUE)
		{
			_tprintf(TEXT("CreateNamedPipe failed, GLE=%d.\n"), GetLastError());
			return -1;
		}

		fConnected = ConnectNamedPipe(hPipe, NULL) ?
			TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

		if (fConnected)
		{
			printf("Client connected, creating a processing thread.\n");
			hThread = CreateThread(
				NULL,              
				0,                 
				PipeIO,    
				(LPVOID)hPipe,    
				0,                
				&dwThreadId);     

			if (hThread == NULL)
			{
				_tprintf(TEXT("CreateThread failed, GLE=%d.\n"), GetLastError());
				return -1;
			}
			else CloseHandle(hThread);
		}
		else
			CloseHandle(hPipe);
	}

	return 0;
}

DWORD WINAPI PipeIO(LPVOID lpvParam)
{
	HANDLE hPipe = (HANDLE)lpvParam;
	DWORD dwCountRead;
	DWORD dwSizeRead;
	ZLSPR mPr;
	INPUT pInput;
	bool fSuccess;
	while (TRUE) {
		if (ReadFile(hPipe, &mPr, sizeof(mPr), &dwCountRead, NULL)) {
			if (mPr.mEventType == ZKEYBOARD_EVENT) {
				/*pInput.type = INPUT_KEYBOARD;
				pInput.ki.wVk = mPr.ptData.lKeyboardStruct.vkCode;
				pInput.ki.time = mPr.ptData.lKeyboardStruct.time;
				pInput.ki.wScan = mPr.ptData.lKeyboardStruct.scanCode;
				pInput.ki.dwFlags = mPr.ptData.lKeyboardStruct.flags;
				SendInput(1, &pInput, sizeof(pInput));*/
			}
			/*else {
				pInput.type = INPUT_MOUSE;
				pInput.mi.mouseData = mPr.ptData.lKeyboardStruct.vkCode;
				printf("%d\n", mPr.ptData.wParam);
				switch (mPr.ptData.wParam) {

				case(ZL_LMOUSEBTDOWN): {
					pInput.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
					break;
				};
				case(ZL_LMOUSEBTUP): {
					pInput.mi.dwFlags = MOUSEEVENTF_LEFTUP;
					break;
				};
				case(ZL_RMOUSEBTDOWN): {
					pInput.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
					break;
				};
				case(ZL_RMOUSEBTUP): {
					pInput.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
					break;
				};
				default: {
					pInput.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
					break;
				}
				}
				pInput.mi.time = mPr.ptData.lKeyboardStruct.flags;
				pInput.mi.dx = mPr.ptData.pt.x * (65536.0f / GetSystemMetrics(SM_CXSCREEN));
				pInput.mi.dy = mPr.ptData.pt.y * (65536.0f / GetSystemMetrics(SM_CYSCREEN));
				SendInput(1, &pInput, sizeof(pInput));
			}
		}*/

			mPr.mDirection = ZTO_CLIENT;
		}
		WriteFile(hPipe, &mPr, sizeof(mPr), &dwCountRead, NULL);

	}
}
