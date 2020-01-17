#pragma warning(disable : 4996)
#include <Windows.h>
#include <stdio.h>
#include "ZLPT.h"
#include "pipe.h"

#define ZLS_IDBT_START 3001
#define ZLS_IDBT_TEXT  3002

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
ATOM RegMyWindowClass(HINSTANCE, LPCTSTR);
int connectToTheServer();
void registrateMSG();

HHOOK mKeyboardhookHANDLE, mMousehookHANDLE;
ZLSPR mPr;

HANDLE hPipe;
POINT pt;
HWND hwndConnectBT, hWnd, hwndMessageTX, hwndAddressED;

ATOM RegMyWindowClass(HINSTANCE hInst, LPCTSTR lpzClassName)
{
	WNDCLASS wcWindowClass = { 0 };
	wcWindowClass.lpfnWndProc = (WNDPROC)WndProc;
	wcWindowClass.style = CS_HREDRAW | CS_VREDRAW;
	wcWindowClass.hInstance = hInst;
	wcWindowClass.lpszClassName = lpzClassName;
	wcWindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcWindowClass.hbrBackground = (HBRUSH)COLOR_APPWORKSPACE;
	return RegisterClass(&wcWindowClass);
}

LRESULT CALLBACK WndProc(
	HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case(WM_CREATE): {
		hwndConnectBT = CreateWindow(
			"BUTTON",
			"CONNECT",
			WS_VISIBLE | WS_CHILD,
			225,
			300,
			100,
			30,
			hWnd,
			(HMENU)ZLS_IDBT_START,
			NULL,
			NULL);
		hwndMessageTX = CreateWindow(
			"STATIC",
			"Ready to connect",
			WS_VISIBLE | WS_CHILD,
			225,
			335,
			200,
			30,
			hWnd,
			(HMENU)ZLS_IDBT_TEXT,
			NULL,
			NULL);
		break;
	}
	case(WM_COMMAND): {
		switch (LOWORD(wParam)) {
		case(ZLS_IDBT_START): {
			connectToTheServer();
			registrateMSG();
		}
		}
	}
	case(WM_DESTROY): {
		exit(1);
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void updateLabel(const char* msg) {
	SetWindowText(hwndMessageTX, msg);
	ShowWindow(hwndMessageTX, SW_HIDE);
	ShowWindow(hwndMessageTX, SW_SHOW);
}

void sendData() {
	if (!WriteFile(hPipe, &mPr, PROTOCOL_SIZE, NULL, 0)) {
		updateLabel("Error: Cannot send a message");
		CloseHandle(hPipe);
		exit(1);
	}
}

void updateData(EVENT_TYPE zEvent, int nCode, WPARAM wParam, LPARAM lParam) {
	ZLSPR_CONTROL mDataToSend;
	if (zEvent == ZKEYBOARD_EVENT) {
		KBDLLHOOKSTRUCT kbdStruct = *((KBDLLHOOKSTRUCT*)lParam);
		ZKBDLLHOOKSTRUCT zkbdStruct = {
				kbdStruct.vkCode,
				kbdStruct.scanCode,
				kbdStruct.flags,
				kbdStruct.time,
				kbdStruct.dwExtraInfo
		};

		mDataToSend.pt = pt;
		mDataToSend.nCode = nCode;
		mDataToSend.wParam = wParam;
		mDataToSend.lKeyboardStruct = zkbdStruct;
	}
	else {
		MSLLHOOKSTRUCT kbdStruct = *((MSLLHOOKSTRUCT*)lParam);
		ZKBDLLHOOKSTRUCT zkbdStruct = {
				kbdStruct.mouseData,
				kbdStruct.flags,
				kbdStruct.time,
				kbdStruct.dwExtraInfo
		};
		mDataToSend.pt = kbdStruct.pt;
		mDataToSend.nCode = nCode;
		mDataToSend.wParam = wParam;
		mDataToSend.lKeyboardStruct = zkbdStruct;
	}
	mPr.mEventType = zEvent;
	mPr.ptData = mDataToSend;
	sendData();
}

LRESULT __stdcall Keyboardproc(int nCode, WPARAM wParam, LPARAM lParam) {
	updateData(ZKEYBOARD_EVENT, nCode, wParam, lParam);
	return CallNextHookEx(mKeyboardhookHANDLE, nCode, wParam, lParam);
}

LRESULT __stdcall Mouseproc(int nCode, WPARAM wParam, LPARAM lParam) {
	updateData(ZMOUSE_EVENT, nCode, wParam, lParam);
	return CallNextHookEx(mMousehookHANDLE, nCode, wParam, lParam);
}

void SetHook() {
	mPr.mDirection = ZTO_SERVER;
	if (!(mKeyboardhookHANDLE = SetWindowsHookEx(WH_KEYBOARD_LL, Keyboardproc, NULL, 0))) {
		updateLabel("Failed to install hook!");
	}
	if (!(mMousehookHANDLE = SetWindowsHookEx(WH_MOUSE_LL, Mouseproc, NULL, 0))) {
		updateLabel("Failed to install hook!");
	}
}

void ReleaseHook() {
	UnhookWindowsHookEx(mKeyboardhookHANDLE);
	UnhookWindowsHookEx(mMousehookHANDLE);
}

void registrateMSG() {

	int iGetOk = 0;
	MSG msg;

	while ((iGetOk = GetMessage(&msg, NULL, 0, 0)) != 0)
	{
		if (iGetOk == -1) return;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

}

int connectToTheServer() {
	SECURITY_ATTRIBUTES sa;
	SECURITY_DESCRIPTOR sd;

	if (InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION) == 0) {
		updateLabel("InitializeSecurityDescriptor failed with error");
		return 1;
	}
	if (SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE) == 0) {
		updateLabel("SetSecurityDescriptorDacl failed with error");
		return 1;
	}
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = &sd;
	sa.bInheritHandle = TRUE;

	while (1)
	{
		hPipe = CreateFile(szPIPE_NAME, GENERIC_WRITE,
			0, NULL, OPEN_EXISTING, 0, &sa);
		if (hPipe != INVALID_HANDLE_VALUE) {

			break;
		}
		if (GetLastError() != ERROR_PIPE_BUSY) {
			updateLabel("Error: Pipe busy");
			return 1;
		}

		if (!WaitNamedPipe(szPIPE_NAME, NMPWAIT_USE_DEFAULT_WAIT))
		{
			updateLabel("Error: timeout expired");
			return 1;
		}
	}
	DWORD dwMode = PIPE_READMODE_MESSAGE;
	if (!SetNamedPipeHandleState(hPipe, &dwMode, NULL, NULL))
	{
		updateLabel("Error: Cannot set pipe handle state");
		CloseHandle(hPipe);
		return 1;
	}
	SetHook();
	updateLabel("Connected to server");
}

int main(int argc, char* argv[]) {

	HWND Console;
	AllocConsole();
	Console = FindWindowA("ConsoleWindowClass", NULL);
	ShowWindow(Console, 0);


	LPCTSTR lpzClass = TEXT("ZLS_WN_CLASS");

	if (!RegMyWindowClass(GetModuleHandle(NULL), lpzClass))
		return 1;

	HWND hWnd = CreateWindow(lpzClass, TEXT("Virtual helper client"),
		WS_OVERLAPPEDWINDOW | WS_VISIBLE, 200, 200, 600, 400, NULL, NULL,
		GetModuleHandle(NULL), NULL);
	if (!hWnd) return 2;
	registrateMSG();
	return 0;
}
