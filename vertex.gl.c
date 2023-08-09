#version 120

uniform mat4 projection;
uniform mat4 camera;
uniform mat4 modelview;
varying out vec2 screen_pos;
varying out vec3 position;
varying out vec3 normal;
varying out vec2 tex_coord;
varying out vec3 cameraPosition;

void main() {
	//screen_pos = (gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex).xy;
	position = (modelview * gl_Vertex).xyz;
	normal = gl_Normal;
	
	gl_Position = projection * camera * modelview * gl_Vertex;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	tex_coord = gl_MultiTexCoord0.xy;
	cameraPosition = vec3(camera[3][0],camera[3][1],camera[3][2]);
}