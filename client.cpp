#pragma warning(disable : 4996)
#include <Windows.h>
#include <stdio.h>
#include <gdiplus.h>
#include "ZLPT.h"
#include "pipe.h"
#include <stdlib.h>
#pragma comment( lib, "gdiplus.lib" ) 

#define ZLS_IDBT_START 3001
#define ZLS_IDBT_TEXT  3002

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
ATOM RegMyWindowClass(HINSTANCE, LPCTSTR);
int connectToTheServer();
void registrateMSG();
void SetHook();
DWORD WINAPI readData(LPVOID handle);

HHOOK mKeyboardhookHANDLE, mMousehookHANDLE;
ZLSPR mPr, mPd;

HANDLE hPipe;
POINT pt;
HWND hwndConnectBT, hWnd, hwndMessageTX, hwndAddressED;

using namespace Gdiplus;

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
			525,
			720,
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
			500,
			750,
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
			SetHook();
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
	DWORD dd;
	if (!WriteFile(hPipe, &mPr, PROTOCOL_SIZE_WITHOUT_IMAGE, &dd, 0)) {
		updateLabel("Error: Cannot send a message");
		CloseHandle(hPipe);
		exit(1);
	}
}



void BitmapToBytes(Gdiplus::Bitmap* bmp, BYTE* ppImageData)
{
	Gdiplus::Rect rect(0, 0, bmp->GetWidth(), bmp->GetHeight());
	Gdiplus::BitmapData bitmapData;
	if (Gdiplus::Ok == bmp->LockBits(
		&rect,
		Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite,             
		PixelFormat32bppARGB,
		&bitmapData
	))
	{
		
		int len = bitmapData.Height * abs(bitmapData.Stride);

		memcpy(ppImageData, bitmapData.Scan0, len);

		bmp->UnlockBits(&bitmapData);
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


	BYTE* pq = mPd.mScreenShot.data;
	Graphics g(GetDC(hWnd));
	DWORD dr;
	if (!ReadFile(hPipe, &mPd, PROTOCOL_SIZE, &dr, NULL)) {
		updateLabel("Error: Cannot read a message");
		CloseHandle(hPipe);
		exit(1);
	}
	Bitmap zaeb(mPd.mScreenShot.width, mPd.mScreenShot.height, mPd.mScreenShot.stride, PixelFormat32bppARGB, pq);
	float horizontalScalingFactor = (float)1280 / (float)zaeb.GetWidth();
	float verticalScalingFactor = (float)720 / (float)zaeb.GetHeight();
	Image* mImage = new Bitmap((int)1280, (int)720);
	Graphics mBitmapToImageGraphics(mImage);

	mBitmapToImageGraphics.ScaleTransform(horizontalScalingFactor, verticalScalingFactor);
	mBitmapToImageGraphics.DrawImage(&zaeb, 0, 0);

	g.DrawImage(mImage, 0, 0);
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
		hPipe = CreateFile(szPIPE_NAME, GENERIC_READ | GENERIC_WRITE,
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
		updateLabel("Error: Cannot set	pipe handle state");
		CloseHandle(hPipe);
		return 1;
	}
	updateLabel("Connected to server");
}



int main(int argc, char* argv[]) {

	HWND Console;
	AllocConsole();
	Console = FindWindowA("ConsoleWindowClass", NULL);
	ShowWindow(Console, 0);

	Gdiplus::GdiplusStartupInput startupInfo;
	ULONG_PTR gdiToken;
	Gdiplus::GdiplusStartup(&gdiToken, &startupInfo, nullptr);

	LPCTSTR lpzClass = TEXT("ZLS_WN_CLASS");

	if (!RegMyWindowClass(GetModuleHandle(NULL), lpzClass))
		return 1;

	hWnd = CreateWindow(lpzClass, TEXT("Virtual helper client"),
		WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, 1280, 820, NULL, NULL,
		GetModuleHandle(NULL), NULL);
	if (!hWnd) return 2;
	registrateMSG();
	return 0;
}
