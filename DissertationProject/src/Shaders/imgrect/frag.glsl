#version 330 core
#extension GL_ARB_shading_language_420pack: enable

in vec4 vertColor;
in vec2 _uv;

layout(binding = 0) uniform sampler2D atlas;

out vec4 FragColor;

void main()
{
	vec4 texf = texture(atlas, _uv).rgba;
	FragColor = texf.rgba;
	
}