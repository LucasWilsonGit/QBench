#version 330 core
#extension GL_ARB_shading_language_420pack: enable

in vec4 vertColor;
in vec2 _uv;

layout(binding = 0) uniform sampler2D atlas;

out vec4 FragColor;

void main()
{
	vec4 texf = vec4(1.f) * texture(atlas, _uv.xy).r;
	FragColor = vec4(vec3(texf.rgb * vertColor.rgb), texf.a); //data in GL_RED format
	if (FragColor.a < .25f) {
		discard;
	}
}