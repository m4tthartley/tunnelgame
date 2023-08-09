#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winuser.h>
#include <GL/gl.h>
#include <stdlib.h>
#include <stdio.h>

#include "def.h"

#define XUSER_MAX_COUNT 4
#define XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE  7849
#define XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE 8689

typedef struct _XINPUT_GAMEPAD {
	WORD  wButtons;
	BYTE  bLeftTrigger;
	BYTE  bRightTrigger;
	SHORT sThumbLX;
	SHORT sThumbLY;
	SHORT sThumbRX;
	SHORT sThumbRY;
} XINPUT_GAMEPAD, *PXINPUT_GAMEPAD;
typedef struct _XINPUT_STATE {
	DWORD          dwPacketNumber;
	XINPUT_GAMEPAD Gamepad;
} XINPUT_STATE, *PXINPUT_STATE;
typedef struct _XINPUT_VIBRATION {
	WORD wLeftMotorSpeed;
	WORD wRightMotorSpeed;
} XINPUT_VIBRATION, *PXINPUT_VIBRATION;
typedef DWORD WINAPI XInputGetStateProc(DWORD dwUserIndex, XINPUT_STATE *pState);
typedef DWORD WINAPI XInputSetStateProc(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration);
XInputGetStateProc* XInputGetState;
XInputSetStateProc* XInputSetState;

HWND window;
HDC hdc;
char* window_title = "Tunnel";
int quit;

win_state win = {0};

void debugShowWin32Error() {
	char errorString[64];
	int errorCode = GetLastError();
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, errorCode, 0, errorString, 64, 0);
	MessageBox(window, errorString, "Error", MB_OK);
}

int64_t freq;
int64_t startTime;
double getTime() {
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	//double time = (double)(counter.QuadPart-lastCounter)/double(freq);
	return (double)(counter.QuadPart-startTime) / (double)freq;
}
double getSecs() {
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	//double time = (double)(counter.QuadPart-lastCounter)/double(freq);
	return (double)(counter.QuadPart-startTime) / (double)freq / 1000.0;
}

typedef int64_t system_time;
system_time getSystemTime() {
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return counter.QuadPart;
}

double convertSystemTime(system_time time) {
	return (double)time / (double)freq;
}

LRESULT CALLBACK WindowCallback (HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
	LRESULT result = 0;
	switch (message) {
		case WM_CREATE:
		break;
		case WM_DESTROY: {
			quit = 1;
			OutputDebugString("WM_DESTROY");
		} break;
		case WM_INPUT: {
			RAWINPUT raw = {0};
			UINT cbsize = sizeof(raw);
			int x = GetRawInputData((HRAWINPUT)lparam, RID_INPUT, &raw, &cbsize, sizeof(RAWINPUTHEADER));
			int y = 0;
			if (raw.header.dwType == RIM_TYPEMOUSE && raw.data.mouse.usFlags == MOUSE_MOVE_RELATIVE) {
				win.mouseDelta.x = raw.data.mouse.lLastX;
				win.mouseDelta.y = raw.data.mouse.lLastY;
			}
			
			result = DefWindowProc(hwnd, message, wparam, lparam);
			break;
		}
		case WM_SIZE: {
			
			RECT rect;
			RECT windowRect;
			
			GetWindowRect(hwnd, &rect);
			//debug_print("rect %i %i %i %i\n", rect.left, rect.top, rect.right, rect.bottom);
			
			windowRect.left = 0;
			windowRect.right = rect.right - rect.left;
			windowRect.top = 0;
			windowRect.bottom = rect.bottom - rect.top;
			AdjustWindowRectEx(&windowRect, WS_OVERLAPPEDWINDOW | WS_VISIBLE, FALSE, 0);
			
			// 			int2 diff = {
			// 				(windowRect.right - windowRect.left) - (rect.right - rect.left),
			// 					(windowRect.bottom - windowRect.top) - (rect.bottom - rect.top),
			// 			};
			// 			
			// 			rain->window_width = (rect.right - rect.left) - diff.x;
			// 			rain->window_height = (rect.bottom - rect.top) - diff.y;
			
			result = DefWindowProc(hwnd, message, wparam, lparam);
			break;
		}
		case WM_MOUSEMOVE: {
			//win.mouse = MAKEPOINTS(lparam);
			// Now using GetCursorPos instead
			break;
		}
		default: {
			result = DefWindowProc(hwnd, message, wparam, lparam);
		} break;
	}
	return result;
}

int create_window() {
	WNDCLASS windowClass = {0};
	//HMODULE hInstance = GetModuleHandle(NULL);
	RECT windowRect;
//	RAWINPUTDEVICE mouse_raw_input;
	
	windowClass.lpfnWndProc = WindowCallback;
	// note: Apparently getting the hInstance this way can cause issues if used in a dll
	
	windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	//windowClass.hInstance = hInstance;
	windowClass.lpszClassName = "RainWindowClass";
	windowClass.hCursor = LoadCursor(0, IDC_ARROW);
	
	windowRect.left = 0;
	windowRect.right = win.width;
	windowRect.top = 0;
	windowRect.bottom = win.height;
	//AdjustWindowRectEx(&windowRect, WS_OVERLAPPEDWINDOW | WS_VISIBLE, FALSE, 0);
	// note: no window border anymore
	
	if (RegisterClassA(&windowClass)) {
		window = CreateWindowExA(0, windowClass.lpszClassName, window_title,
								 /*WS_OVERLAPPEDWINDOW*/WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN,
								 0, 0,
								 windowRect.right - windowRect.left,
								 windowRect.bottom - windowRect.top,
								 0, 0, NULL, 0);

		if (window) {
			quit = 0;
			UpdateWindow(window);
			/*BOOL posresult = SetWindowPos(window, 0,
						 0,
						 0,
						 windowRect.right - windowRect.left,
						 windowRect.bottom - windowRect.top,
						 0);*/
			
			hdc = GetDC(window);
		} else {
			printf("Error while creating window\n");
			//goto error;
			return 0;
		}
	} else {
		printf("Error while registering window class\n");
		//goto error;
		return 0;
	}
	
	RAWINPUTDEVICE mouse_raw_input;
	mouse_raw_input.usUsagePage = 1;
	mouse_raw_input.usUsage = 2;
	mouse_raw_input.dwFlags = 0;
	mouse_raw_input.hwndTarget = window;
	if (!RegisterRawInputDevices(&mouse_raw_input, 1, sizeof(mouse_raw_input))) {
		MessageBox(window, "Failed to register raw mouse input", NULL, MB_OK);
	}
	
//	SetWindowPos(window, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE);
	
	return 1;
}

#define WGL_DRAW_TO_WINDOW_ARB            0x2001
#define WGL_SUPPORT_OPENGL_ARB            0x2010
#define WGL_DOUBLE_BUFFER_ARB             0x2011
#define WGL_PIXEL_TYPE_ARB                0x2013
#define WGL_COLOR_BITS_ARB                0x2014
#define WGL_DEPTH_BITS_ARB                0x2022
#define WGL_STENCIL_BITS_ARB              0x2023
#define WGL_TYPE_RGBA_ARB                 0x202B
#define WGL_SAMPLE_BUFFERS_ARB            0x2041
#define WGL_SAMPLES_ARB                   0x2042

#define WGL_CONTEXT_DEBUG_BIT_ARB         0x00000001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002
#define WGL_CONTEXT_MAJOR_VERSION_ARB     0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB     0x2092
#define WGL_CONTEXT_FLAGS_ARB             0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB      0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB  0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002

typedef HGLRC WINAPI wglCreateContextAttribsARB_proc(HDC hDC, HGLRC hShareContext, const int *attribList);
typedef BOOL WINAPI wglChoosePixelFormatARB_proc(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
wglCreateContextAttribsARB_proc *wglCreateContextAttribsARB;
wglChoosePixelFormatARB_proc *wglChoosePixelFormatARB;

void create_opengl () {
	PIXELFORMATDESCRIPTOR pixelFormat = {0};
	int suggestedPixelFormatIndex;
	PIXELFORMATDESCRIPTOR suggestedPixelFormat;
	HGLRC glContext;

	// note: this sets theses files as the std output location
	/*freopen("stdout.txt", "a", stdout);
	freopen("stderr.txt", "a", stderr);*/

	pixelFormat.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pixelFormat.nVersion = 1;
	pixelFormat.iPixelType = PFD_TYPE_RGBA;
	pixelFormat.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
	pixelFormat.cColorBits = 32;
	pixelFormat.cAlphaBits = 8;
	pixelFormat.iLayerType = PFD_MAIN_PLANE;

	suggestedPixelFormatIndex = ChoosePixelFormat(hdc, &pixelFormat);
	if (!suggestedPixelFormatIndex) {
		printf("ChoosePixelFormat failed\n");
		goto error;
	}
	
	DescribePixelFormat(hdc, suggestedPixelFormatIndex, sizeof(PIXELFORMATDESCRIPTOR), &suggestedPixelFormat);
	if (!SetPixelFormat(hdc, suggestedPixelFormatIndex, &suggestedPixelFormat)) {
		printf("SetPixelFormat failed\n");
		goto error;
	}

	glContext = wglCreateContext(hdc);
	if (!glContext) {
		printf("wglCreateContext failed\n");
		goto error;
	}
	if (!wglMakeCurrent(hdc, glContext)) {
		printf("wglMakeCurrent failed\n");
		goto error;
	}

#if 1
	wglCreateContextAttribsARB = (wglCreateContextAttribsARB_proc*)wglGetProcAddress("wglCreateContextAttribsARB");
	wglChoosePixelFormatARB = (wglChoosePixelFormatARB_proc*)wglGetProcAddress("wglChoosePixelFormatARB");
	wglDeleteContext(glContext);

	{
		int format;
		unsigned int num_formats;
		int format_attribs[] = {
			WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
			WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
			WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
			WGL_COLOR_BITS_ARB, 32,
			WGL_DEPTH_BITS_ARB, 24,
			WGL_STENCIL_BITS_ARB, 8,

			/*WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
			WGL_SAMPLES_ARB, 8,*/
			0,
		};
		
		int attribs[] = {
			WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
				WGL_CONTEXT_MINOR_VERSION_ARB, 2,
				WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
				WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
				0
		};

		wglChoosePixelFormatARB(hdc, format_attribs, NULL, 1, &format, &num_formats);

		{
			HGLRC context = wglCreateContextAttribsARB(hdc, 0, attribs);
			wglMakeCurrent(hdc, context);
		}
	}
#endif

	return;
error:
	MessageBox(window, "There was an error initializing OpenGL video", NULL, MB_OK);
	exit(1);
}

file_handle openFile(char* path) {
	assert(sizeof(HANDLE)<=sizeof(file_handle));
	
	HANDLE handle = CreateFileA(path, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ,
								0, /*OPEN_EXISTING*/OPEN_ALWAYS, 0, 0);
	if(handle==INVALID_HANDLE_VALUE) {
		return 0;
	}
	return handle;
}

void readFile(file_handle file, int offset, int size, void* output) { // 32bit only
	DWORD bytesRead;
	OVERLAPPED overlapped = {0};
	overlapped.Offset = offset;
	int result = ReadFile(file, output, size, &bytesRead, &overlapped);
	if(!result || bytesRead!=size) {
		//char msg[64];
		//sprintf(msg, "Failed to load file: %s", 
		MessageBox(window, "Failed to read file", "Error", MB_OK);
	}
}

void writeFile(file_handle file, int offset, int size, void* data) { // 32bit only
	DWORD bytesWritten;
	OVERLAPPED overlapped = {0};
	overlapped.Offset = offset;
	int result = WriteFile(file, data, size, &bytesWritten, &overlapped);
	if(!result || bytesWritten!=size) {
		//char msg[64];
		//sprintf(msg, "Failed to load file: %s", 
		MessageBox(window, "Failed to write file", "Error", MB_OK);
	}
}

file_info fileStat(file_handle file) {
	file_info result = {0};
	BY_HANDLE_FILE_INFORMATION info = {0};
	if(GetFileInformationByHandle(file, &info)) {
		result.created = info.ftCreationTime.dwLowDateTime;
		result.created |= (uint64_t)info.ftCreationTime.dwHighDateTime<<32;
		result.modified = info.ftLastWriteTime.dwLowDateTime;
		result.modified |= (uint64_t)info.ftLastWriteTime.dwHighDateTime<<32;
		result.size = info.nFileSizeLow;
	} else {
		MessageBox(window, "Failed to stat file", "Error", MB_OK);
	}
	return result;
}

void closeFile(file_handle file) {
	if(file != INVALID_HANDLE_VALUE) {
		CloseHandle(file);
	}
}

FILESTATBYPATH_PROC(fileStatByPath) {
	file_info result = {0};
	WIN32_FIND_DATA info;
	HANDLE handle = FindFirstFileA(path, &info);
	if(handle!=INVALID_HANDLE_VALUE) {
		result.created = info.ftCreationTime.dwLowDateTime;
		result.created |= (uint64_t)info.ftCreationTime.dwHighDateTime<<32;
		result.modified = info.ftLastWriteTime.dwLowDateTime;
		result.modified |= (uint64_t)info.ftLastWriteTime.dwHighDateTime<<32;
		result.size = info.nFileSizeLow;
		FindClose(handle);
	} else {
		MessageBox(window, "Failed to stat file", "Error", MB_OK);
	}
	return result;
}

HANDLE gamelib = 0;
uint64_t gameCodeLastWriteTime;
gamestart_proc* gamestart;
gameloop_proc* gameloop;

char* gameCodeBuildPath = "build/_game.dll";
char* gameCodePath = "build/game.dll";

uint64_t getGameCodeWriteTime() {
	WIN32_FIND_DATAA findData;
	HANDLE findHandle = FindFirstFileA(gameCodeBuildPath, &findData);
	if(findHandle!=INVALID_HANDLE_VALUE) {
		FindClose(findHandle);
		return (findData.ftLastWriteTime.dwLowDateTime | 
				((uint64_t)findData.ftLastWriteTime.dwHighDateTime<<32));
	}
	return 0;
}

int loadGameCode() {
	FreeLibrary(gamelib);
	//uint64_t t1 = getGameCodeWriteTime();
	int copy = CopyFile(gameCodeBuildPath, gameCodePath, 0);
	//uint64_t t2 = getGameCodeWriteTime();
	gamelib = LoadLibraryA("game.dll");
	//uint64_t t3 = getGameCodeWriteTime();
	if(gamelib) {
		gamestart = (gamestart_proc*)GetProcAddress(gamelib, "gamestart");
		gameloop = (gameloop_proc*)GetProcAddress(gamelib, "gameloop");
		return gamestart && gameloop;
	}
	return 0;
}

float processThumbStickAxis(int axis, int deadzone) {
	float result = 0.0f;
	if (axis > deadzone) {
		result = (float)(axis-deadzone) / (32767.0f-deadzone);
	} else if(axis < -deadzone) {
		result = (float)(axis+deadzone) / (32768.0f-deadzone);
	}
	return result;
}

int main() {
	LARGE_INTEGER _freq;
	LARGE_INTEGER st;
	QueryPerformanceFrequency(&_freq);
	freq = _freq.QuadPart/1000;
	QueryPerformanceCounter(&st);
	startTime = st.QuadPart;
	
	win.openFile = openFile;
	win.readFile = readFile;
	win.writeFile = writeFile;
	win.fileStat = fileStat;
	win.closeFile = closeFile;
	win.fileStatByPath = fileStatByPath;
	
	{
		HANDLE xinputLib = LoadLibraryA("XInput1_4.dll");
		//uint64_t t3 = getGameCodeWriteTime();
		if(xinputLib) {
			XInputGetState = (XInputGetStateProc*)GetProcAddress(xinputLib, "XInputGetState");
			XInputSetState = (XInputSetStateProc*)GetProcAddress(xinputLib, "XInputSetState");
		}
	}
	
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	int pageSize = systemInfo.dwPageSize;
	
	DEVMODEA devmode;
	EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &devmode);
	int displayHz = devmode.dmDisplayFrequency;
	
	void* memory = VirtualAlloc(0, MEGABYTES(5), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

	memory_arena memoryDist = {0};
	memoryDist.address = memory;
	memoryDist.size = MEGABYTES(5);
	
	game_state* game = pushMemory(&memoryDist, align(sizeof(game_state), pageSize));
	
	game->gfx.memorySize = KILOBYTES(100);
	game->gfx.memory = pushMemory(&memoryDist, game->gfx.memorySize);
	
	game->gfx.transient.size = KILOBYTES(100);
	game->gfx.transient.address = pushMemory(&memoryDist, game->gfx.transient.size);
	
	game->gfx.programMemory.size = KILOBYTES(1);
	game->gfx.programMemory.address = pushMemory(&memoryDist, game->gfx.programMemory.size);
	
	game->gfx.shaderLinks.size = KILOBYTES(1);
	game->gfx.shaderLinks.address = pushMemory(&memoryDist, game->gfx.shaderLinks.size);
	
	game->gfx.freeBlocks = game->gfx.memory;
	game->gfx.freeBlocks->size = game->gfx.memorySize;
	
	game->transientMemory.size = KILOBYTES(10);
	game->transientMemory.address = pushMemory(&memoryDist, game->transientMemory.size);
	
	game->tunnelGen.segmentMemory.size = KILOBYTES(4);
	game->tunnelGen.segmentMemory.address = pushMemory(&memoryDist, game->tunnelGen.segmentMemory.size);
	game->tunnelGen.geometryMemory.size = KILOBYTES(400);
	game->tunnelGen.geometryMemory.address = pushMemory(&memoryDist, game->tunnelGen.geometryMemory.size);
	game->tunnelGen.lightMemory.size = sizeof(light)*MAX_LIGHTS;
	game->tunnelGen.lightMemory.address = pushMemory(&memoryDist, game->tunnelGen.lightMemory.size);
	
	/*{
		WIN32_FIND_DATAA findData;
		HANDLE findHandle = FindFirstFileA(gameCodePath, &findData);
		if(findHandle!=INVALID_HANDLE_VALUE) {
			gameCodeLastWriteTime = (findData.ftLastWriteTime.dwLowDateTime |
									 ((uint64_t)findData.ftLastWriteTime.dwHighDateTime<<32));
			FindClose(findHandle);
		} else {
			gameCodePath = "game.dll";
			gameCodeCopyPath = "game_copy.dll";
			HANDLE findHandle = FindFirstFileA(gameCodePath, &findData);
			if(findHandle!=INVALID_HANDLE_VALUE) {
				gameCodeLastWriteTime = (findData.ftLastWriteTime.dwLowDateTime |
										 ((uint64_t)findData.ftLastWriteTime.dwHighDateTime<<32));
				FindClose(findHandle);
			}
		}
	}*/
	
	gameCodeLastWriteTime = getGameCodeWriteTime();
	if(!gameCodeLastWriteTime) {
		gameCodeBuildPath = "_game.dll";
		gameCodePath = "game.dll";
		gameCodeLastWriteTime = getGameCodeWriteTime();
	}
	
	system_time lastFrameTime = getSystemTime();
	
	if(loadGameCode()) {
		win.width = 1920;
		win.height = 1080;
		create_window();
		create_opengl();
		gamestart(&win, game);
		while(!quit) {
#if 1
			uint64_t writeTime = getGameCodeWriteTime();
			if(writeTime != gameCodeLastWriteTime) {
				Sleep(400);
				//HANDLE findHandle = FindFirstFileA("build/game.dll", &findData);
				//uint64_t writeTime = findData.ftLastWriteTime.dwLowDateTime | 
				//((uint64_t)findData.ftLastWriteTime.dwHighDateTime<<32);
				//FindClose(findHandle);
				writeTime = getGameCodeWriteTime();
				printf("loading new game code \n");
				gameCodeLastWriteTime = writeTime;
				if(loadGameCode()) {
					SYSTEMTIME time;
					GetSystemTime(&time);
					printf("reloaded game code %i:%i:%i \n", time.wHour, time.wMinute, time.wSecond);
				} else {
					MessageBox(window, "Failed to load game code", "Error", MB_OK);
					exit(0);
				}
			}
#endif
			
			MSG msg;
			// BYTE keyboard[256] = {0};
			//int lclick = 0;
			win.lbuttonDownEvent=0;
			win.rbuttonDownEvent=0;
			win.lbuttonUpEvent=0;
			win.rbuttonUpEvent=0;
			win.mouseDelta.x = 0;
			win.mouseDelta.y = 0;
			//GetKeyboardState(win.keyboard);
			while(PeekMessage(&msg, window, 0, 0, PM_REMOVE)) {
				if(msg.message==WM_LBUTTONDOWN) {
					win.lbuttonDownEvent = 1;
					win.lbuttonIsDown = 1;
				}
				if(msg.message==WM_LBUTTONUP) {
					win.lbuttonUpEvent = 1;
					win.lbuttonIsDown = 0;
				}
				if(msg.message==WM_RBUTTONDOWN) {
					win.rbuttonDownEvent = 1;
					win.rbuttonIsDown = 1;
				}
				if(msg.message==WM_RBUTTONUP) {
					win.rbuttonUpEvent = 1;
					win.rbuttonIsDown = 0;
				}
				TranslateMessage(&msg);
				DispatchMessageA(&msg);
			}
			
			BYTE keyboard[256];
			GetKeyboardState(keyboard);
			for(int i=0; i<256; ++i) {
				win.keyboard[i] <<= 1;
				win.keyboard[i] |= keyboard[i] >> 7;
			}
			
			POINT mouse;
			GetCursorPos(&mouse);
			ScreenToClient(window, &mouse);
			win.mouse.x = mouse.x;
			win.mouse.y = mouse.y;
			
			float thumbMax = 32767.0f;
			for(int i=0; i< XUSER_MAX_COUNT; ++i) {
				XINPUT_STATE controller;
				if(XInputGetState(i, &controller) == ERROR_SUCCESS) {
					XINPUT_GAMEPAD* gamepad = &controller.Gamepad;
					win.gamepad.leftThumb.x = processThumbStickAxis(gamepad->sThumbLX,
																	XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
					win.gamepad.leftThumb.y = processThumbStickAxis(gamepad->sThumbLY,
																	XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
					win.gamepad.rightThumb.x = processThumbStickAxis(gamepad->sThumbRX,
																	 XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
					win.gamepad.rightThumb.y = processThumbStickAxis(gamepad->sThumbRY,
																	 XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
				}
			}
			
			win.runtimeInSeconds = getSecs();
			
			float targetFrameTime = 1000.0f / (float)displayHz;
			system_time newTime = getSystemTime();
			double frameTime = convertSystemTime(newTime-lastFrameTime);
			lastFrameTime = newTime;
			float timing = frameTime/targetFrameTime;
			float dt = 60.0f/(float)displayHz * timing;
			gameloop(&win, game, dt);
			
			SwapBuffers(hdc);
		}
	} else {
		MessageBox(window, "Failed to load game code", "Error", MB_OK);
	}
}