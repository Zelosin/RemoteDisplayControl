#pragma once

#define ZL_LMOUSEBTDOWN 0x0201
#define ZL_LMOUSEBTUP   0x0202
#define ZL_RMOUSEBTDOWN 0x0204
#define ZL_RMOUSEBTUP   0x0205
#define ZL_MMOUSEBTDOWN 0x0207
#define ZL_MMOUSEBTUP   0x0208

typedef enum { ZKEYBOARD_EVENT, ZMOUSE_EVENT } EVENT_TYPE;
typedef enum { ZTO_SERVER, ZTO_CLIENT } DIRECTION;

struct ZLS_IMAGE{
	int stride;
	int width;
	int height;
	BYTE data[960000]; 
};

struct ZKBDLLHOOKSTRUCT {
	DWORD     vkCode;
	DWORD     scanCode;
	DWORD     flags;
	DWORD     time;
	ULONG_PTR dwExtraInfo;
};

struct ZLSPR_CONTROL {
	POINT pt;
	int nCode;
	WPARAM wParam;
	ZKBDLLHOOKSTRUCT lKeyboardStruct;
};

struct ZLSPR {
	EVENT_TYPE mEventType;
	DIRECTION mDirection;
	ZLSPR_CONTROL ptData;
	ZLS_IMAGE mScreenShot;
};

const int PROTOCOL_SIZE = sizeof(ZLSPR);
const int PROTOCOL_SIZE_WITHOUT_IMAGE = sizeof(EVENT_TYPE) + sizeof(DIRECTION) + sizeof(ZLSPR_CONTROL);