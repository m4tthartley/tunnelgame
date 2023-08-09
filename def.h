
#pragma warning (disable: 4244)
#pragma warning (default: 4255) // missing prototypes
#pragma warning (disable: 4133) // incompatible types

typedef __int64  int64_t;
typedef unsigned __int64  uint64_t;
typedef __int32  int32_t;
typedef unsigned __int32  uint32_t;
typedef __int16  int16_t;
typedef unsigned __int16  uint16_t;
typedef unsigned char byte;

#define KILOBYTES(n) (n*1024)
#define MEGABYTES(n) (n*1024*1024)
#define GIGABYTES(n) (n*1024*1024*1024)
#define assert(exp) (exp ? (exp) : (*(int*)0 = 0))

#define array_size(arr) (assert(*((int*)arr-2) > 0) ? *((int*)arr-2) : 0)
/*(int)((byte*)&arr##_size - (byte*)&arr)*/
/*(sizeof(arr)/sizeof(arr[0]))*/
#define array_count(arr) *((int*)arr-1) // todo: is the struct packing ok with this in 64bit?
#define array_define(type, name, size) int name##_count; \
                                       type name[64]; \
                                       int name##_size
#define array_init(arr) arr##_size = (sizeof(arr)/sizeof(arr[0]))

typedef struct {
	void* address;
	int size; //note: 32bit only 
	struct memory_block* next;
	struct memory_block* prev;
} memory_block;
typedef struct {
	void* address;
	int size; //note: 32bit only 
	int stack;
	// todo: linked lists
} memory_arena;

#define _MATH_DECLARATIONS
#include "../math.c"

#define _GFX_HEADER
#include "../gfx.c"

#include "shader_shared.h"

// Platform
#define KEY_F1 0x70
#define KEY_F2 0x71
#define KEY_F3 0x72
#define KEY_F4 0x73
#define KEY_F5 0x74
#define KEY_F6 0x75
#define KEY_F7 0x76
#define KEY_F8 0x77
#define KEY_F9 0x78
#define KEY_F10 0x79
#define KEY_F11 0x7A
#define KEY_F12 0x7B
#define KEY_LEFT 0x25
#define KEY_UP 0x26
#define KEY_RIGHT 0x27
#define KEY_DOWN 0x28
#define KEY_SPACE 0x20
#define KEY_SHIFT 0xA0

typedef void* file_handle;
typedef struct {
	uint64_t created;
	uint64_t modified;
	int size; // 32bit only
} file_info;

#define OPENFILE_PROC(name) file_handle name(char* path)
#define READFILE_PROC(name) void name(file_handle file, int offset, int size, void* output)
#define WRITEFILE_PROC(name) void name(file_handle file, int offset, int size, void* data)
#define FILESTAT_PROC(name) file_info name(file_handle file)
#define CLOSEFILE_PROC(name) void name(file_handle file)
#define FILESTATBYPATH_PROC(name) file_info name(char* path)
typedef OPENFILE_PROC(openFile_proc);
typedef READFILE_PROC(readFile_proc);
typedef WRITEFILE_PROC(writeFile_proc);
typedef FILESTAT_PROC(fileStat_proc);
typedef CLOSEFILE_PROC(closeFile_proc);
typedef FILESTATBYPATH_PROC(fileStatByPath_proc);
typedef struct {
	int width;
	int height;
	
	byte keyboard[256];
	point mouse;
	point mouseDelta;
	int lbuttonDownEvent;
	int rbuttonDownEvent;
	int lbuttonUpEvent;
	int rbuttonUpEvent;
	int lbuttonIsDown;
	int rbuttonIsDown;
	
	struct {
		vec2 leftThumb;
		vec2 rightThumb;
		int leftBumper;
		int rightBumper;
	} gamepad;
	
	double runtimeInSeconds;
	
	openFile_proc* openFile;
	readFile_proc* readFile;
	writeFile_proc* writeFile;
	fileStat_proc* fileStat;
	closeFile_proc* closeFile;
	fileStatByPath_proc* fileStatByPath;
} win_state;

inline int keyIsDown(win_state* p, byte key) {
	return p->keyboard[key] & 1;
}

inline int keyUpEvent(win_state* p, byte key) {
	return (p->keyboard[key] & 3) == 2;
}

inline int keyDownEvent(win_state* p, byte key) {
	return (p->keyboard[key] & 3) == 1;
}

typedef struct {
	vec3 pos;
	//quaternion orientation;
	float pitch;
	float yaw;
	float roll;
} transform;

typedef struct {
	vec3 v0;
	vec3 v1;
	vec3 v2;
	vec3 v3;
	vec3 normal;
} tunnel_panel;
typedef struct {
	vec3 pos;
	quaternion ori;
	vec3 color;
	int id;
} tunnel_segment;
typedef struct {
	int panelsPerSegment;
	vec3 headPos;
	quaternion headOri;
	int latestId;
	int numSegments;
	memory_arena segmentMemory;
	int numLights;
	memory_arena lightMemory;
	int numGeometryPanels;
	memory_arena geometryMemory;
} tunnel_gen;
typedef struct {
	vec3 pos; // todo: these might need to be vec4 for uniform buffers
	vec3 color;
} light;

typedef enum {
	CAMERA_MODE_GAME = 0,
	CAMERA_MODE_FREE = 1,
	CAMERA_MODE_RAILS = 2,
	CAMERA_MODE_END
} camera_mode;
char* cameraModeNames[] = {
	"CAMERA_MODE_GAME",
	"CAMERA_MODE_RAILS",
	"CAMERA_MODE_FREE"
};

typedef struct {
	int showUI;
	transform freeCam;
} debug_state;

// Game
typedef struct {
	gfx_state gfx;
	gfx_program* shaderWall;
	
	debug_state debug;
	tunnel_gen tunnelGen;
	
	float rails;
	camera_mode cameraMode;
	vec3 playerPos;
	quat playerOri;
	float throttle;
	float pitchControl;
	float rollControl;
	float yawControl;
	
	memory_arena transientMemory;
} game_state;

#define GAMELOOP_PROC(name) void name(win_state* win, game_state* game, float dt)
#define GAMESTART_PROC(name) void name(win_state* win, game_state* game)
typedef GAMESTART_PROC(gamestart_proc);
typedef GAMELOOP_PROC(gameloop_proc);

//inline
inline void zeroMemory(byte* address, int size) {
	byte* end = address+size;
	while(address<end){
		*address++ = 0;
	}
}

inline void copyMemory(byte* dest, byte* src, int size) {
	byte* end = dest+size;
	while(dest<end){
		*dest++ = *src++;
	}
}

inline void* pushMemory(memory_arena* arena, int size) {
	assert(arena->stack + size <= arena->size);
	if(arena->stack + size <= arena->size) {
		void* result = (byte*)arena->address+arena->stack;
		arena->stack += size;
		return result;
	}
	return 0;
}
inline void* pushRollingMemory(memory_arena* arena, int size) {
	if(arena->stack+size > arena->size) {
		arena->stack = 0;
	}
	assert(arena->stack+size <= arena->size);
	if(arena->stack+size <= arena->size) {
		void* result = (byte*)arena->address+arena->stack;
		arena->stack += size;
		return result;
	}
	return 0;
}

inline void* pushAndCopyMemory(memory_arena* arena, byte* src, int size) {
	/*assert(arena->stack + size <= arena->size);
	if(arena->stack + size <= arena->size) {
		void* result = (byte*)arena->address+arena->stackIndex;
		arena->stackIndex += size;
		copy_memory(result, src, size);
		return result;
	}
	return 0;*/
	void* result = pushMemory(arena, size);
	if(result) {
		copyMemory(result, src, size);
	}
	return result;
}
inline void* pushAndCopyRollingMemory(memory_arena* arena, byte* src, int size) {
	void* result = pushRollingMemory(arena, size);
	if(result) {
		copyMemory(result, src, size);
	}
	return result;
}

inline void popMemory(memory_arena* arena, int size) {
	zeroMemory((byte*)arena->address + arena->stack - size, size);
	arena->stack -= size;
}

inline void clearMemoryArena(memory_arena* arena) {
	zeroMemory(arena->address, arena->stack);
	arena->stack = 0;
}