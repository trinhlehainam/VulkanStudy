#version 450 		// GLSL 4.5

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;

layout (location = 0) out vec3 fragColor;		// Output color to fragment shader

vec3 positions[3] = vec3[](
	vec3(0.0, -0.4, 0.0),
	vec3(0.4, 0.4, 0.0),
	vec3(-0.4, 0.4, 0.0)
);

vec3 colors[3] = vec3[](
	vec3(1.0,0.0,0.0),
	vec3(0.0,1.0,0.0),
	vec3(0.0,0.0,1.0)
);

void main()
{
	gl_Position = vec4(positions[gl_VertexIndex],1.0);
	fragColor = colors[gl_VertexIndex];
}