#version 330 core
layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 rgba;

uniform vec2 screenDim;

out vec4 vertColor;
out vec2 _uv;

void main()
{
	gl_Position = vec4((vec2(2.f * pos.x, -2.f * pos.y) / screenDim) - vec2(1.f, -1.f), 1.f - pos.z / 1000.f, 1.0); //to 4d for homog matr 
	vertColor = rgba;
	_uv = uv;
}