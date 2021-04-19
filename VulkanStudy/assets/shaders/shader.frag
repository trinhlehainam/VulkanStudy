#version 450

layout (location = 0) in vec3 inColor;			// Input color from vertex shader

layout (location = 0) out vec4 outColor;		// Output to another pipeline

void main()
{
	outColor = vec4(inColor,1.0);
}
