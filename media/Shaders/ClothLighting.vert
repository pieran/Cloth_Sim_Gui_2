#version 150 core

uniform mat4 viewProjMatrix;
uniform float normal_mult;

in  vec3 position;
in  vec3 colour;
in  vec3 normal;

out Vertex {
	vec3 colour;
	vec3 worldPos;
	vec3 normal;
} OUT;

void main(void)	{
	gl_Position	  = viewProjMatrix * vec4(position, 1.0);
	OUT.colour    = colour;
	OUT.worldPos  = position;
	OUT.normal    = normal * normal_mult;
}