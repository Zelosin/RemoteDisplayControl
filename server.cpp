#include <windows.h>
#include <gdiplus.h>
#include <stdio.h>
#include <process.h>
#include "pipe.h"
#include "ZLPT.h"
#include <tchar.h>
#include <strsafe.h>

#pragma comment(linker, "/STACK:20000000")
using namespace Gdiplus;

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
	HANDLE hThread = NULL;

	for (;;)
	{
		LPCTSTR lpszPipename = TEXT("\\\\.\\pipe\\zls");
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
			&sa);                    

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

int BitmapToBytes(Gdiplus::Bitmap* bmp, BYTE*	ppImageData)
{
	Gdiplus::Rect rect(0, 0, bmp->GetWidth(), bmp->GetHeight());
	//get the bitmap data
	Gdiplus::BitmapData bitmapData;
	if (Gdiplus::Ok == bmp->LockBits(
		&rect, //A rectangle structure that specifies the portion of the Bitmap to lock.
		Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite, //ImageLockMode values that specifies the access level (read/write) for the Bitmap.            
		PixelFormat32bppARGB,// PixelFormat values that specifies the data format of the Bitmap.
		&bitmapData //BitmapData that will contain the information about the lock operation.
		))
	{
		//get the lenght of the bitmap data in bytes
		int len = bitmapData.Height * abs(bitmapData.Stride);

		memcpy(ppImageData, bitmapData.Scan0, len);//copy it to an array of BYTEs

		bmp->UnlockBits(&bitmapData);	
	}
	return bitmapData.Stride;
}


DWORD WINAPI PipeIO(LPVOID lpvParam)
{

	Gdiplus::GdiplusStartupInput startupInfo;
	ULONG_PTR gdiToken;
	Gdiplus::GdiplusStartup(&gdiToken, &startupInfo, nullptr);

	HANDLE hPipe = (HANDLE)lpvParam;
	DWORD dwCountRead;
	DWORD dwSizeRead;
	ZLSPR mPr;
	INPUT pInput;
	bool fSuccess;
	while (TRUE) {

		if (ReadFile(hPipe, &mPr, PROTOCOL_SIZE_WITHOUT_IMAGE, &dwCountRead, NULL)) {
			if (mPr.mEventType == ZKEYBOARD_EVENT) {
				pInput.type = INPUT_KEYBOARD;
				pInput.ki.wVk = mPr.ptData.lKeyboardStruct.vkCode;
				pInput.ki.time = mPr.ptData.lKeyboardStruct.time;
				pInput.ki.wScan = mPr.ptData.lKeyboardStruct.scanCode;
				pInput.ki.dwFlags = mPr.ptData.lKeyboardStruct.flags;
				SendInput(1, &pInput, sizeof(pInput));
			}
			else {
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
			}


		mPr.mDirection = ZTO_CLIENT;
		HDC hScreenDC = GetDC(NULL);
		HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

		int x = GetDeviceCaps(hScreenDC, HORZRES);

		int y = GetDeviceCaps(hScreenDC, VERTRES);
		HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, x, y);
		HGDIOBJ hOldBitmap = SelectObject(hMemoryDC, hBitmap);

		BitBlt(hMemoryDC, 0, 0, x, y, hScreenDC, 0, 0, SRCCOPY);
		hBitmap = (HBITMAP)SelectObject(hMemoryDC, hOldBitmap);
		BYTE *pt = mPr.mScreenShot.data;
		Bitmap mSceenShot(hBitmap, NULL);

		float horizontalScalingFactor = (float) 600 / (float) mSceenShot.GetWidth();
		float verticalScalingFactor = (float) 400 / (float)  mSceenShot.GetHeight();
		Image* mImage = new Bitmap((int) 600, (int) 400);
		Graphics mBitmapToImageGraphics(mImage);
		mBitmapToImageGraphics.ScaleTransform(horizontalScalingFactor, verticalScalingFactor);
		mBitmapToImageGraphics.DrawImage(&mSceenShot, 0, 0);

		Bitmap mRescaledBitmap(mImage->GetWidth(), mImage->GetHeight() , PixelFormat8bppIndexed);
		Graphics mImageToBitmapGraphics(&mRescaledBitmap);
 	
		mImageToBitmapGraphics.DrawImage(mImage,0,0,mImage->GetWidth(),mImage->GetHeight());

		mPr.mScreenShot.stride = BitmapToBytes(&mRescaledBitmap, pt);
		mPr.mScreenShot.height = mRescaledBitmap.GetHeight();
		mPr.mScreenShot.width = mRescaledBitmap.GetWidth();
		
		if(!WriteFile(hPipe, &mPr, PROTOCOL_SIZE, &dwCountRead, NULL))
			printf("%d", GetLastError());
		printf("sent\n");
	}
}
