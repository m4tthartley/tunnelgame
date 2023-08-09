#version 330 core

#include "shader_shared.h"

//uniform sampler2D texture0;
//uniform float time;
//uniform vec2 camera;
//uniform vec2 scale;

struct light {
	vec3 pos;
	vec3 color;
};
//uniform vec3 lightPosition;
uniform vec3 lightPositions[MAX_LIGHTS];
uniform vec3 lightColors[MAX_LIGHTS];
uniform int numLights;
uniform vec3 cameraPosition;

in vec3 position;
in vec3 normal;
in vec2 tex_coord;
out vec4 color;

#include "math.gl.c"

struct attenuation_setting {
	float linear;
	float quadratic;
};
//attenuation_setting attenuationTerms[12] = {
#define ATTENUATION1 attenuation_setting(0.7, 1.8)
#define ATTENUATION2 attenuation_setting(0.35, 0.44)
#define ATTENUATION3 attenuation_setting(0.22, 0.20)
#define ATTENUATION4 attenuation_setting(0.14, 0.07)
#define ATTENUATION5 attenuation_setting(0.09, 0.032)
#define ATTENUATION6 attenuation_setting(0.07, 0.017)
#define ATTENUATION7 attenuation_setting(0.045, 0.0075)
#define ATTENUATION8 attenuation_setting(0.027, 0.0028)
#define ATTENUATION9 attenuation_setting(0.022, 0.0019)
#define ATTENUATION10 attenuation_setting(0.014, 0.0007)
#define ATTENUATION11 attenuation_setting(0.007, 0.0002)
#define ATTENUATION12 attenuation_setting(0.0014, 0.000007)
//};

void main() {
	//float light = max(10.0 - length(lightPosition - world_pos), 0.0) /10.0;
	
	vec3 texture = vec3(.15,.15,.15) + noise(position*200)*0.04*2.5;
	//texture = vec3(.15) + fbm(position*200)*0.04*2.5;
	float ambient = 0.2f;
	
	vec3 lightBlend = vec3(0);
	for(int i=0; i<numLights; ++i) {
		vec3 pos = lightPositions[i];
		vec3 lightColor = lightColors[i];
		
		vec3 n = normalize(normal);
		vec3 lightDir = normalize(pos - position);
		float angle = max(dot(n, lightDir), 0.0);
		
		vec3 diffuse = angle*lightColor*1.0;
		
#if 0
		// Phong specular
		float specularStrength = 0.5;
		//vec3 viewPos = vec3(0, 0, 0);
		vec3 viewPos = cameraPosition;
		vec3 viewDir = normalize(viewPos - position);
		vec3 reflectDir = reflect(-lightDir, n);
		float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
		vec3 specular = specularStrength * spec * lightColor;
#else
		// Blinn specular
		float shininess = 8.0;
		vec3 viewPos = cameraPosition;
		vec3 viewDir = normalize(viewPos - position);
		vec3 halfwayDir = normalize(lightDir + viewDir);
		float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
		vec3 specular = lightColor * spec * 1.0;
#endif
		
		attenuation_setting a = ATTENUATION3;
		float aConstant = 1.0;
		//float aLinear = 0.07;
		//float aQuadratic = 0.032;
		float dist = length(pos - position);
		float attenuation = 1.0 / (aConstant + a.linear*dist + a.quadratic*dist*dist);
		
		//ambient *= attenuation;
		diffuse *= attenuation;
		specular *= attenuation;
		
		lightBlend += diffuse+specular;
	}
	
	color = vec4(texture * lightBlend, 1);
}