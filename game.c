#define WIN32_LEAN_AND_MEAN
#include <WINDOWS.H>
#include <gl/GL.H>
#include <STDLIB.H>
#include <STDIO.H>

#include "def.h"

typedef __int64  int64_t;
typedef unsigned __int64  uint64_t;
typedef __int32  int32_t;
typedef unsigned __int32  uint32_t;
typedef __int16  int16_t;
typedef unsigned __int16  uint16_t;

//double getTime();
//double getSecs();
//int editorShowGrid;

openFile_proc* openFile;
readFile_proc* readFile;
writeFile_proc* writeFile;
fileStat_proc* fileStat;
closeFile_proc* closeFile;
fileStatByPath_proc* fileStatByPath;

#pragma pack(push, 1)
typedef struct {
	char header[2];
	uint32_t size;
	uint16_t reserved1;
	uint16_t reserved2;
	uint32_t offset;
	
	// Windows BITMAPINFOHEADER
	uint32_t headerSize;
	int32_t bitmapWidth;
	int32_t bitmapHeight;
	uint16_t colorPlanes;
	uint16_t colorDepth;
	uint32_t compression;
	uint32_t imageSize;
	int32_t hres;
	int32_t vres;
	uint32_t paletteSize;
	uint32_t importantColors;
} bmp_header;
#pragma pack(pop)

typedef struct {
	uint32_t* data;
	bmp_header* header;
} bmp;

bmp loadBmp(char* filename) {
	FILE* fontFile;
	long fileSize;
	void* fontData;
	bmp_header* header;
	uint32_t* palette;
	char* data;
	int rowSize;
	uint32_t* image;
	bmp result;

	fontFile = fopen(filename, "r"); // todo: this stuff crashes when file not found
	if(!fontFile) {
		printf("cant open file\n");
	}
	fseek(fontFile, 0, SEEK_END);
	fileSize = ftell(fontFile);
	fontData = malloc(fileSize);
	rewind(fontFile);
	fread(fontData, 1, fileSize, fontFile);
	fclose(fontFile);
	
	header = (bmp_header*)fontData;
	palette = (uint32_t*)((char*)fontData+14+header->headerSize);
	data = (char*)fontData+header->offset;
	rowSize = ((header->colorDepth*header->bitmapWidth+31) / 32) * 4;
	
	image = (uint32_t*)malloc(sizeof(uint32_t)*header->bitmapWidth*header->bitmapHeight);
	//{for(int w=0; w<header.bitmapHeight}
	{
		int row;
		int pixel;
		for(row=0; row<header->bitmapHeight; ++row) {
			int bitIndex=0;
			//printf("row %i \n", row);
// 			if(row==255) {
// 				DebugBreak();
// 			}
			for(pixel=0; pixel<header->bitmapWidth; ++pixel) {//while((bitIndex/8) < rowSize) {
				uint32_t* chunk = (uint32_t*)((char*)fontData+header->offset+(row*rowSize)+(bitIndex/8));
				uint32_t pi = *chunk;
				if(header->colorDepth<8) {
					pi >>= (header->colorDepth-(bitIndex%8));
				}
				pi &= (((int64_t)1<<header->colorDepth)-1);
				if(header->colorDepth>8) {
					image[row*header->bitmapWidth+pixel] = pi;
				} else {
					image[row*header->bitmapWidth+pixel] = palette[pi];
				}
				if(/*image[row*header->bitmapWidth+pixel]==0xFF000000 ||*/
					image[row*header->bitmapWidth+pixel]==0xFFFF00FF) {
					image[row*header->bitmapWidth+pixel] = 0;
				}
// 				if(pixel==120) {
// 					int asd = 0;
// 				}
				bitIndex += header->colorDepth;
			}
		}
	}
	
	result.data = image;
	result.header = header;
	return result;
}

GLuint createTexture(bmp image) {
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.header->bitmapWidth, image.header->bitmapHeight, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, image.data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	return tex;
}

gfx_file_info gfxFileStat(char* path) {
	file_info info = fileStatByPath(path);
	gfx_file_info result;
	result.writeTime = info.modified;
	result.size = info.size;
	return result;
}

int gfxLoadFile(char* path, int size, void* output) {
	file_handle file = openFile(path);
	if(file) {
		readFile(file, 0, size, output); // todo: if this fails it still returns success
		closeFile(file);
		return 1;
	}
	return 0;
}

generateTunnelSegments(tunnel_gen* gen, int numSegments, vec3 axis, float angle, vec3 color) {
	float tunnelRadius = 2.0f;
	//quaternion rotation = qidentity();
	//vec3 currentCurve = _vec3(0.0f, 0.1f, 0.0f);
	//vec3 pos = _vec3(0, 0, 0);
	quat targetOri = gen->headOri;
	qrotate(&targetOri, axis, angle);
	tunnel_segment* segments = gen->segmentMemory.address;
	tunnel_panel* panels = gen->geometryMemory.address;
	for(int segi=0; segi<numSegments; ++segi) {
		float t = (float)segi/(float)numSegments;
		//float e = 2.71828183f;
		//t = (1 / (1 + powf(e, -t)));
		quat rotation = qnlerp(gen->headOri, targetOri, t);
		
		for(int i=0; i<gen->panelsPerSegment; ++i) {
			float rad = (i/(float)gen->panelsPerSegment) * PI2;
			float rad2 = ((i+1)/(float)gen->panelsPerSegment) * PI2;
			tunnel_panel p;
			p.v0 = _vec3(sinf(rad)*tunnelRadius, cosf(rad)*tunnelRadius, 0.5f);
			p.v1 = _vec3(sinf(rad2)*tunnelRadius, cosf(rad2)*tunnelRadius, 0.5f);
			p.v2 = _vec3(sinf(rad2)*tunnelRadius, cosf(rad2)*tunnelRadius,-0.5f);
			p.v3 = _vec3(sinf(rad)*tunnelRadius, cosf(rad)*tunnelRadius, -0.5f);
	
			qrotate_vec3(&p.v0, rotation);
			qrotate_vec3(&p.v1, rotation);
			qrotate_vec3(&p.v2, rotation);
			qrotate_vec3(&p.v3, rotation);
			p.v0 = add3(p.v0, gen->headPos);
			p.v1 = add3(p.v1, gen->headPos);
			p.v2 = add3(p.v2, gen->headPos);
			p.v3 = add3(p.v3, gen->headPos);
			
			if(gen->latestId > 0) {
				//p.v0 = panels[(seg-1)*gen->panelsPerSegment+i].v3;
				//p.v1 = panels[(seg-1)*gen->panelsPerSegment+i].v2;
				//tunnel_panel* lastPanel = (byte*)gen->geometryMemory.address;
				int index = gen->geometryMemory.stack/sizeof(tunnel_panel) - gen->panelsPerSegment;
				if(index < 0) {
					index += gen->numGeometryPanels;
				}
				tunnel_panel lastPanel = panels[index];
				p.v0 = lastPanel.v3;
				p.v1 = lastPanel.v2;
			}
			p.normal = cross3(sub3(p.v0, p.v3), sub3(p.v0, p.v1));
			//p.normal = _vec3(-sinf(rad)*tunnelRadius, -cosf(rad)*tunnelRadius, 0.0f);
			//qrotate_vec3(&p.normal, rotation);
			//p.normal = normalize3(p.normal);
			pushAndCopyRollingMemory(&gen->geometryMemory, &p, sizeof(tunnel_panel));
			if(sizeof(tunnel_panel)*(gen->numGeometryPanels+1) <= gen->geometryMemory.size) {
				++gen->numGeometryPanels;
			}
			
			if(i==gen->panelsPerSegment/2 && gen->latestId%4==0) {
				light l;
				l.pos = add3(p.v0, mul3f(p.normal, 0.1f));
				l.color = /*normalize3(_vec3(randf(),randf(),randf()))*/color;
				pushAndCopyRollingMemory(&gen->lightMemory, &l, sizeof(light));
				if(gen->numLights < MAX_LIGHTS) {
					++gen->numLights;
				}
			}
		}
		
		tunnel_segment seg;
		seg.pos = gen->headPos;
		seg.ori = rotation;
		seg.id = ++gen->latestId;
		pushAndCopyRollingMemory(&gen->segmentMemory, &seg, sizeof(tunnel_segment));
		if(sizeof(tunnel_segment)*(gen->numSegments+1) <= gen->segmentMemory.size) {
			++gen->numSegments;
		} else {
			int x = 0;
		}
								 
		//qrotate(&gen->headOri, normalize3(axis), angle);
		vec3 dir = _vec3(0.0f, 0.0f, -1.0f);
		qrotate_vec3(&dir, seg.ori);
		gen->headPos = add3(gen->headPos, dir);
	}
	//gen->numSegments += numSegments;
	gen->headOri = targetOri;
}
generateTunnelStart(tunnel_gen* tunnelGen) {
	//generateTunnelSegments(tunnelGen, 10, _vec3(1.0f, 1.0f, 0.0f), 0.2f);
	//generateTunnelSegments(tunnelGen, 10, _vec3(-1.0f, -1.0f, 0.0f), 0.2f);
	//generateTunnelSegments(tunnelGen, 10, _vec3(1.0f, 1.0f, 0.0f), 0.2f);
	//generateTunnelSegments(tunnelGen, 10, _vec3(1.0f, 1.0f, 0.0f), 0.0f);
	//generateTunnelSegments(tunnelGen, 10, _vec3(1.0f, 0.0f, 0.0f), -0.2f);
}

void drawLightCube(vec3 pos, vec3 color) {
	glPushMatrix();
	glTranslatef(pos.x, pos.y, pos.z);
	glColor3f(color.x, color.y, color.z);
	glBegin(GL_QUADS);
	float size = 0.1f;
	glVertex3f(-0.5f*size, -0.5f*size, 0.5f*size);
	glVertex3f(+0.5f*size, -0.5f*size, 0.5f*size);
	glVertex3f(+0.5f*size, +0.5f*size, 0.5f*size);
	glVertex3f(-0.5f*size, +0.5f*size, 0.5f*size);
	
	glVertex3f(+0.5f*size, -0.5f*size, -0.5f*size);
	glVertex3f(-0.5f*size, -0.5f*size, -0.5f*size);
	glVertex3f(-0.5f*size, +0.5f*size, -0.5f*size);
	glVertex3f(+0.5f*size, +0.5f*size, -0.5f*size);
	
	glVertex3f(-0.5f*size, -0.5f*size, -0.5f*size);
	glVertex3f(-0.5f*size, -0.5f*size, +0.5f*size);
	glVertex3f(-0.5f*size, +0.5f*size, +0.5f*size);
	glVertex3f(-0.5f*size, +0.5f*size, -0.5f*size);
	
	glVertex3f(0.5f*size, -0.5f*size, +0.5f*size);
	glVertex3f(0.5f*size, -0.5f*size, -0.5f*size);
	glVertex3f(0.5f*size, +0.5f*size, -0.5f*size);
	glVertex3f(0.5f*size, +0.5f*size, +0.5f*size);
	
	glVertex3f(-0.5f*size, +0.5f*size, +0.5f*size);
	glVertex3f(+0.5f*size, +0.5f*size, +0.5f*size);
	glVertex3f(+0.5f*size, +0.5f*size, -0.5f*size);
	glVertex3f(-0.5f*size, +0.5f*size, -0.5f*size);
	
	glVertex3f(+0.5f*size, -0.5f*size, +0.5f*size);
	glVertex3f(-0.5f*size, -0.5f*size, +0.5f*size);
	glVertex3f(-0.5f*size, -0.5f*size, -0.5f*size);
	glVertex3f(+0.5f*size, -0.5f*size, -0.5f*size);
	glEnd();
	glPopMatrix();
}

void drawCube(float size) {
	glBegin(GL_QUADS);
	glNormal3f(0.0f, 0.0f, 1.0f);
	glVertex3f(-0.5f*size, -0.5f*size, 0.5f*size);
	glVertex3f(+0.5f*size, -0.5f*size, 0.5f*size);
	glVertex3f(+0.5f*size, +0.5f*size, 0.5f*size);
	glVertex3f(-0.5f*size, +0.5f*size, 0.5f*size);
	
	glNormal3f(0.0f, 0.0f, -1.0f);
	glVertex3f(+0.5f*size, -0.5f*size, -0.5f*size);
	glVertex3f(-0.5f*size, -0.5f*size, -0.5f*size);
	glVertex3f(-0.5f*size, +0.5f*size, -0.5f*size);
	glVertex3f(+0.5f*size, +0.5f*size, -0.5f*size);
	
	glNormal3f(-1.0f, 0.0f, 0.0f);
	glVertex3f(-0.5f*size, -0.5f*size, -0.5f*size);
	glVertex3f(-0.5f*size, -0.5f*size, +0.5f*size);
	glVertex3f(-0.5f*size, +0.5f*size, +0.5f*size);
	glVertex3f(-0.5f*size, +0.5f*size, -0.5f*size);
	
	glNormal3f(1.0f, 0.0f, 0.0f);
	glVertex3f(0.5f*size, -0.5f*size, +0.5f*size);
	glVertex3f(0.5f*size, -0.5f*size, -0.5f*size);
	glVertex3f(0.5f*size, +0.5f*size, -0.5f*size);
	glVertex3f(0.5f*size, +0.5f*size, +0.5f*size);
	
	glNormal3f(0.0f, 1.0f, 0.0f);
	glVertex3f(-0.5f*size, +0.5f*size, +0.5f*size);
	glVertex3f(+0.5f*size, +0.5f*size, +0.5f*size);
	glVertex3f(+0.5f*size, +0.5f*size, -0.5f*size);
	glVertex3f(-0.5f*size, +0.5f*size, -0.5f*size);
	
	glNormal3f(0.0f, -1.0f, 0.0f);
	glVertex3f(+0.5f*size, -0.5f*size, +0.5f*size);
	glVertex3f(-0.5f*size, -0.5f*size, +0.5f*size);
	glVertex3f(-0.5f*size, -0.5f*size, -0.5f*size);
	glVertex3f(+0.5f*size, -0.5f*size, -0.5f*size);
	glEnd();
}

void processFlyingControl(float* control, float input, float acceleration) {
	if(input > 0.01f) {
		if(*control < input)
			*control += input*acceleration;
	} else if(input < -0.01f) {
		if(*control > input)
			*control += input*acceleration;
	} else {
		if(*control > acceleration) {
			*control -= acceleration;
		} else if(*control < -acceleration) {
			*control += acceleration;
		} else {
			*control = 0.0f;
		}
	}
}

GAMESTART_PROC(gamestart) {
	openFile = win->openFile; // todo: duplicate
	readFile = win->readFile;
	writeFile = win->writeFile;
	fileStat = win->fileStat;
	closeFile = win->closeFile;
	fileStatByPath = win->fileStatByPath;
	
	//bmp charactersBmp = loadBmp("characters.bmp");
	//game->charactersTexture = createTexture(charactersBmp);
	//game->snowShader = create_shader_vf("basic.vert", "snow.frag");
	
	load_opengl_extensions(&game->gfx);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	//game->shaderWall = create_shader_vf("common.vert", "wall.frag");
	
	//void* vertCommon = gfx_create_shaderv("common.vert");
	//void* fragWall = gfx_create_shaderf("tunnel_Panel.frag");
	//void* shaderWall = gfx_link_program
	
	gfx_set_file_stat_callback(gfxFileStat);
	gfx_set_load_file_callback(gfxLoadFile);
	game->shaderWall = gfx_create_shader("vertex.gl.c", "wall.gl.c");
	
	//game->camera = mat4_camera(_vec3(0.0f, 0.0f, 0.0f),
							   //_vec3(0.0f, 0.0f, -1.0f),
	//_vec3(0.0f, 1.0f, 0.0));
	game->debug.freeCam.pos = _vec3(0.0f, 0.0f, 4.0f);
	game->playerOri = qidentity();
	//mat4_rotate_y(&game->camera, rotation*0.02f);
	//mat4_translate(&game->camera, _vec3(0.0f, 0.0f, 4.0f));
	
	game->tunnelGen.panelsPerSegment = 10;
	game->tunnelGen.headOri = qidentity();
	//generateTunnelStart(&game->tunnelGen);
}

float rotation = 0;

GAMELOOP_PROC(gameloop) {
	openFile = win->openFile;
	readFile = win->readFile;
	writeFile = win->writeFile;
	fileStat = win->fileStat;
	closeFile = win->closeFile;
	fileStatByPath = win->fileStatByPath;
	
	set_gfx_globals(&game->gfx);
	gfx_set_file_stat_callback(gfxFileStat);
	gfx_set_load_file_callback(gfxLoadFile);
	
	tunnel_gen* gen = &game->tunnelGen;
	/*clearMemoryArena(&gen->segmentMemory);
	clearMemoryArena(&gen->lightMemory);
	clearMemoryArena(&gen->geometryMemory);
	gen->panelsPerSegment = 10;
	gen->headOri = qidentity();
	gen->headPos = _vec3(0,0,0);
	gen->numSegments = 0;
	gen->numLights = 0;
	generateTunnelStart(&game->tunnelGen);*/
	gen->panelsPerSegment = 50;
	
	tunnel_segment* segments = gen->segmentMemory.address;
	tunnel_segment* closestSegment = segments;
	float closestDist = len3(sub3(closestSegment->pos, game->playerPos));
	for(int i=1; i<gen->numSegments; ++i) {
		float d = len3(sub3(segments[i].pos, game->playerPos));
		if(d < closestDist) {
			closestDist = d;
			closestSegment = segments+i;
		}
	}
	
	if(closestSegment->id > gen->latestId-50) {
		generateTunnelSegments(gen, 40,
							   _vec3(randfr(-1.0f,1.0f), randfr(-1.0f,1.0f), randfr(-1.0f,1.0f)),
							   randf()*1.0f,
							   normalize3(_vec3(randf(),randf(),randf())));
	}
	
	if(keyUpEvent(win, KEY_F1)) {
		game->debug.showUI = 1-game->debug.showUI;
	}
	if(keyUpEvent(win, KEY_F2)) {
		game->debug.freeCam.pos = game->playerPos;
		
		++game->cameraMode;
		if(game->cameraMode==CAMERA_MODE_END) {
			game->cameraMode = 0;
		}
	}
	if(keyUpEvent(win, KEY_F3)) {
		game->playerPos = closestSegment->pos;
	}
	
	gfx_update_shaders();
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	rotation += 0.5f;
	//if(game->rails > gen->numSegments) {
		//game->rails -= gen->numSegments;
	//}
	
	//game->debug.cameraMode = CAMERA_MODE_FREE;
	
	mat4 perspective = perspective_matrix(70, 1920.0f/1080.0f, 0.1f, 100.0f);
	mat4 cameraMatrix = mat4_identity();
	
	//game->cameraMode = CAMERA_MODE_GAME;
	if(game->cameraMode == CAMERA_MODE_FREE) {
		float speed = 0.07f * dt;
		transform* cam = &game->debug.freeCam;
		if(keyIsDown(win, 'W')) {
			//mat4_translate(&game->camera, _vec3(0.0f, 0.0f, -0.1f));
			vec3 v = _vec3(sinf(cam->yaw)*-speed, 0.0f, cosf(cam->yaw)*-speed);
			cam->pos = add3(cam->pos, v);
		}
		if(keyIsDown(win, 'S')) {
			vec3 v = _vec3(sinf(cam->yaw)*-speed, 0.0f, cosf(cam->yaw)*-speed);
			cam->pos = sub3(cam->pos, v);
		}
		if(keyIsDown(win, 'A')) {
			vec3 v = _vec3(cosf(cam->yaw)*-speed, 0.0f, sinf(cam->yaw)*speed);
			cam->pos = add3(cam->pos, v);
		}
		if(keyIsDown(win, 'D')) {
			vec3 v = _vec3(cosf(cam->yaw)*-speed, 0.0f, sinf(cam->yaw)*speed);
			cam->pos = sub3(cam->pos, v);
		}
		if(keyIsDown(win, KEY_SPACE)) {
			cam->pos.y += speed;
		}
		if(keyIsDown(win, KEY_SHIFT)) {
			cam->pos.y += -speed;
		}
		vec2 mouseDelta = _vec2(win->mouseDelta.x, win->mouseDelta.y);
		//qrotate(&game->camera.orientation, _vec3(0.0f, 1.0f, 0.0f), mouseDelta.x*-0.01f);
		//qrotate(&game->camera.orientation, _vec3(1.0f, 0.0f, 0.0f), mouseDelta.y*-0.01f);
		cam->yaw += mouseDelta.x*-0.01f;
		cam->pitch += mouseDelta.y*-0.01f;
		
		cameraMatrix = mat4_translation(mul3f(cam->pos, -1.0f));
		mat4_rotate_y(&cameraMatrix, cam->yaw);
		mat4_rotate_x(&cameraMatrix, cam->pitch);
		//cameraMatrix = mat4_mul(cameraMatrix, qmat4(cam->ori));
	}
	
	if(game->cameraMode == CAMERA_MODE_RAILS) {
		game->rails += 0.2f * dt;
		int rail = game->rails;
		int rail2 = game->rails+1.0f;
		rail %= gen->numSegments;
		rail2 %= gen->numSegments;
		if(rail2 < rail) {
			int x = 0;
		}
		
		vec3 lerpPos =  lerp3(segments[rail].pos, segments[rail2].pos, fract(game->rails));
		cameraMatrix = mat4_translation(mul3f(lerpPos, -1.0f));
		game->playerPos = lerpPos;
		
		quaternion q = segments[rail].ori;
		quaternion q2 = segments[rail2].ori;
		quaternion qn = qnlerp(q, q2, fract(game->rails));
		cameraMatrix = mat4_mul(cameraMatrix, qmat4(qinverse(qn)));
	}
	
	if(game->cameraMode == CAMERA_MODE_GAME) {
		vec3 dir = _vec3(0.0f, 0.0f, -1.0f);
		vec3 xaxis = _vec3(1,0,0);
		vec3 yaxis = _vec3(0,1,0);
		vec3 zaxis = _vec3(0,0,-1);
		qrotate_vec3(&xaxis, game->playerOri);
		qrotate_vec3(&yaxis, game->playerOri);
		qrotate_vec3(&zaxis, game->playerOri);
		float pitchSpeed = 0.02f * dt;
		float rollSpeed = 0.06f * dt;
		float yawSpeed = 0.005f * dt;
		float flyingSpeed = 0.2f * dt;
		if(keyIsDown(win, KEY_SPACE)) flyingSpeed *= 1.5f;
		if(keyIsDown(win, KEY_UP)) {
			qrotate(&game->playerOri, xaxis, -pitchSpeed);
		}
		if(keyIsDown(win, KEY_DOWN)) {
			qrotate(&game->playerOri, xaxis, pitchSpeed);
		}
		if(keyIsDown(win, KEY_LEFT)) {
			qrotate(&game->playerOri, zaxis, -rollSpeed);
		}
		if(keyIsDown(win, KEY_RIGHT)) {
			qrotate(&game->playerOri, zaxis, rollSpeed);
		}
		if(keyIsDown(win, 'A')) {
			qrotate(&game->playerOri, yaxis, yawSpeed);
		}
		if(keyIsDown(win, 'D')) {
			qrotate(&game->playerOri, yaxis, -yawSpeed);
		}
		
		/*if(win->gamepad.rightThumb.y > 0.01f) {
			if(game->pitchControl < win->gamepad.rightThumb.y)
				game->pitchControl += win->gamepad.rightThumb.y*controlAcceleration;
			//game->pitchControl = min(game->pitchControl, win->gamepad.rightThumb.y);
		} else if(win->gamepad.rightThumb.y < -0.01f) {
			if(game->pitchControl > win->gamepad.rightThumb.y)
				game->pitchControl += win->gamepad.rightThumb.y*controlAcceleration;
			//game->pitchControl = max(game->pitchControl, win->gamepad.rightThumb.y);
		} else {
			if(game->pitchControl > 0.01f) {
				game->pitchControl -= controlAcceleration;
			} else if(game->pitchControl < -0.01f) {
				game->pitchControl += controlAcceleration;
			} else {
				game->pitchControl = 0.0f;
			}
		}*/
		float acceleration = 0.2f * dt;
		processFlyingControl(&game->pitchControl, win->gamepad.rightThumb.y, acceleration);
		processFlyingControl(&game->rollControl, win->gamepad.rightThumb.x, acceleration);
		processFlyingControl(&game->yawControl, win->gamepad.leftThumb.x, acceleration);
		//printf("roll %f %f \n", game->rollControl, win->gamepad.rightThumb.x);
		//game->pitchControl = clamp(game->pitchControl, -fabs(win->gamepad.rightThumb.y), fabs(win->gamepad.rightThumb.y));
		
		/*if(win->gamepad.rightThumb.x != 0.0f) {
			game->rollControl += win->gamepad.rightThumb.x*controlAcceleration;
		} else {
			game->rollControl -= (game->rollControl/fabs(game->rollControl))*controlAcceleration;
		}
		game->rollControl = clamp(game->rollControl,
								  -fabs(win->gamepad.rightThumb.x), fabs(win->gamepad.rightThumb.x));*/

		qrotate(&game->playerOri, xaxis, -pitchSpeed * game->pitchControl);
		qrotate(&game->playerOri, zaxis, rollSpeed * game->rollControl);
		qrotate(&game->playerOri, yaxis, -yawSpeed * game->yawControl);
		
		if(win->gamepad.leftThumb.y > 0.01f) {
			game->throttle += win->gamepad.leftThumb.y*0.01f*dt;
		} else if(win->gamepad.leftThumb.y < -0.01f) {
			if(game->throttle > 1.1f) {
				game->throttle += (1.0f-game->throttle)*0.05f*dt;
			} else if(game->throttle > 0.5f) {
				game->throttle += win->gamepad.leftThumb.y*0.01f*dt;
			}
		} else {
			if(game->throttle > 1.0f) {
				game->throttle += (1.0f-game->throttle)*0.001f*dt;
			} else {
				game->throttle += (1.0f-game->throttle)*0.1f*dt;
			}
		}

		//game->throttle = max(min(game->throttle, 2.0f), 0.5f);
		
		qrotate_vec3(&dir, game->playerOri);
		game->playerPos = add3(game->playerPos, mul3f(dir, flyingSpeed*game->throttle));
		
		cameraMatrix = mat4_translation(mul3f(game->playerPos, -1.0f));
		cameraMatrix = mat4_mul(cameraMatrix, qmat4(qinverse(game->playerOri)));
	}
	
	gfx_sh(game->shaderWall);
	gfx_um4("projection", perspective.f);
	gfx_um4("camera", cameraMatrix);
	gfx_uf3("cameraPosition", game->playerPos.x, game->playerPos.y, game->playerPos.z);
	mat4 modelview = mat4_identity();
	gfx_um4("modelview", modelview.f);
	
	// LIGHTS
	tunnel_panel* panels = gen->geometryMemory.address;
	//vec3* lightPositions = pushMemory(&game->transientMemory, sizeof(vec3)*12);
	//vec3* lightColors = pushMemory(&game->transientMemory, sizeof(vec3)*12);
	/*for(int i=0; i<10; i+=1) {
		lightPositions[i] = panels[i*gen->panelsPerSegment+5].v0;
		lightPositions[i].y += 0.05f;
		lightColors[i] = _vec3(1.0f, 1.0f, 1.0f);
		lightPositions[i+1] = panels[i*gen->panelsPerSegment].v0;
		lightPositions[i+1].y -= 0.05f;
		lightColors[i+1] = _vec3(1.0f, 1.0f, 1.0f);
	}
	lightPositions[10] = _vec3(0.0f + sinf(rotation*0.1f)*0.5f, 0.5, 0.0f + (sinf(rotation*0.02f)-1.0f)*5);
	lightColors[10] = _vec3(1.0f, 0.0f, 0.0f);
	lightPositions[11] = _vec3(0.0f + cosf(rotation*0.1f)*0.5f, -0.5, 0.0f + (cosf(rotation*0.02f)-1.0f)*5);
	lightColors[11] = _vec3(0.0f, 0.0f, 1.0f);*/
	
	vec3* lightPositions = pushMemory(&game->transientMemory, sizeof(vec3)*gen->numLights);
	vec3* lightColors = pushMemory(&game->transientMemory, sizeof(vec3)*gen->numLights);
	light* lights = gen->lightMemory.address;
	for(int i=0; i<gen->numLights; ++i) {
		lightPositions[i] = lights[i].pos;
		lightColors[i] = lights[i].color;
	}
	gfx_ui1("numLights", gen->numLights);
	gfx_uf3v("lightPositions", gen->numLights, lightPositions);
	gfx_uf3v("lightColors", gen->numLights, lightColors);
	
	
	// TUNNEL
	gfx_sh(game->shaderWall);
	glBegin(GL_QUADS);
	//for(int seg=0; seg<gen->numGeometryPanels; ++seg) {
	for(int i=0; i<gen->numGeometryPanels; ++i) {
		tunnel_panel p = panels[i];
		glNormal3f(p.normal.x, p.normal.y, p.normal.z);
		glVertex3f(p.v0.x, p.v0.y, p.v0.z);
		glVertex3f(p.v1.x, p.v1.y, p.v1.z);
		glVertex3f(p.v2.x, p.v2.y, p.v2.z);
		glVertex3f(p.v3.x, p.v3.y, p.v3.z);
	}
	//}
	glEnd();

	
	// CUBE
	mat4_translate(&modelview, _vec3(1, -1.5, -2.0f));
	gfx_um4("modelview", modelview.f);
	drawCube(1.0f);


	// DEBUG
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(perspective.f);
	glMatrixMode(GL_MODELVIEW);
	
	glPushMatrix();
	glLoadMatrixf(cameraMatrix.f);
	gfx_sh(0);
	
	// LIGHT CUBES
	for(int i=0; i<gen->numLights; ++i) {
		drawLightCube(lightPositions[i], lightColors[i]);
	}
	
	// SEGMENT CUBES
#if 0
	for(int i=0; i<gen->numSegments; ++i) {
		if(segments+i == closestSegment) {
			drawLightCube(segments[i].pos, _vec3(1,1,0));
		} else {
			drawLightCube(segments[i].pos, _vec3(0,1,0));
		}
	}
#endif
	
#if 0
	// NORMALS
	glBegin(GL_LINES);
	for(int seg=0; seg<10; ++seg) {
		for(int i=0; i<game->tunnelGeometryNumPanels; ++i) {
			tunnel_panel p = panels[seg*game->tunnelGeometryNumPanels+i];
			vec3 normal = p.normal;
			glVertex3f(p.v0.x, p.v0.y, p.v0.z);
			glVertex3f(p.v0.x+normal.x*0.1f, p.v0.y+normal.y*0.1f, p.v0.z+normal.z*0.1f);
		}
	}
	glEnd();
#endif
	
	glPopMatrix();
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
	glColor3f(1,0,0);
	glBegin(GL_LINES);
	glVertex2f(-0.9f, -0.5f);
	glVertex2f(-0.9f, 0.5f);
	glEnd();
	glColor3f(0,1,0);
	glBegin(GL_LINES);
	glVertex2f(-0.9f, -0.5f);
	glVertex2f(-0.9f, -0.5f+((game->throttle-0.5f)*(1.0f/1.5f)));
	glEnd();
	
	if(game->debug.showUI) {
		// Memory debug
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glDisable(GL_LIGHTING);
		glDisable(GL_DEPTH_TEST);
		glUseProgram(0);
		
		glPushMatrix();
		glTranslatef(-0.9f, 0.5f, 0);
		float m = 10.0f / (float)gfx->memorySize;
		glScalef(m, m, 1);
		glColor3f(1, 0, 0);
		glBegin(GL_QUADS);
		glVertex2f(0, 0);
		glVertex2f((float)gfx->memorySize*m, 0);
		glVertex2f((float)gfx->memorySize*m, 0.1f);
		glVertex2f(0, 0.1f);
		glEnd();
		
		gfx_shader* shader = gfx->shaders;
		while(shader) {
			int offset = ((byte*)shader-gfx->memory);
			int size = align(sizeof(gfx_shader) + shader->path_size + shader->code_size, 64);
			int w = size;
			//glColor3f((float)((uint64_t)shader % 10) / 10.0f, 1, 0);
			glColor3f(0, (float)((uint64_t)shader->code_size % 255) / 255.0f, 0);
			for(int i=offset/64; i<((offset+w)/64); ++i) {
				int x = (((i)%64)*64);
				int y = (((i)/64)*128);
				glBegin(GL_QUADS);
				glVertex2f(x, y);
				glVertex2f(x+64, y);
				glVertex2f(x+64, y+128);
				glVertex2f(x, y+128);
				glEnd();
			}
			
			glBegin(GL_LINES);
			glVertex2f(offset, 0.1f);
			glVertex2f(offset, 0.15f);
			glEnd();
			
			shader = shader->next;
		}
		
		free_block* free = gfx->freeBlocks;
		while(free) {
			int offset = ((byte*)free-gfx->memory);
			int w = free->size;
			glColor3f(0.2f, 0.2f, 0.2f);
			for(int i=offset/64; i<((offset+w)/64); ++i) {
				int x = (((i)%64)*64);
				int y = (((i)/64)*128);
				glBegin(GL_QUADS);
				glVertex2f(x, y);
				glVertex2f(x+64, y);
				glVertex2f(x+64, y+128);
				glVertex2f(x, y+128);
				glEnd();
			}
			
			glBegin(GL_LINES);
			glVertex2f(offset, 0.1f);
			glVertex2f(offset, 0.15f);
			glEnd();
			
			free = free->next;
		}
		
		glPopMatrix();
	}
	
	// quaternion test
	/*vec3 test = _vec3(1.0, 0.0, 0.0);
	quaternion q = qidentity();
	qrotate(&q, _vec3(0.0f, 0.0f, 1.0f), rotation*0.1f);
	qrotate_vec3(&test, q);
	glColor3f(0, 1, 0);
	glBegin(GL_QUADS);
	glVertex3f(test.x-0.1, test.y-0.1, test.z);
	glVertex3f(test.x+0.1, test.y-0.1, test.z);
	glVertex3f(test.x+0.1, test.y+0.1, test.z);
	glVertex3f(test.x-0.1, test.y+0.1, test.z);
	glEnd();*/
	
	clearMemoryArena(&game->transientMemory);
}