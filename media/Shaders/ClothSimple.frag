#version 150 core

in Vertex	{
	vec3 colour;
} IN;

out vec4 gl_FragColor;

void main(void)	{
	gl_FragColor.rgb = IN.colour;
	gl_FragColor.a = 1.0;
}