#version 450

layout (set = 0, binding = 1) uniform sampler2D texSampler;

layout (location = 0) in vec3 inColor;			// Input color from vertex shader
layout (location = 1) in vec2 texCoord;

layout (location = 0) out vec4 outColor;		// Output to another pipeline

void main()
{
	outColor = texture(texSampler, texCoord);
}
