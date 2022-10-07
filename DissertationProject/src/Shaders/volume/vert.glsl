#version 330 core

layout(location = 0) in vec3 pos; //vertex position
layout(location = 1) in vec4 candleData; 
layout(location = 2) in float volume; //per instance candle data

uniform mat4 projection;

out vec4 vertColor; 

void main()
{
	float offsX = gl_InstanceID * 20.f;
	float offsY = (candleData.a + candleData.r) / 2.f; //OHLC midpoint

	float diff = candleData.r - candleData.a;
	float delta = abs(diff);
	float buy = 1.f - (diff / delta + 1.f) / 2.f; //0 = sell, 1 = buy

	float scaleY = delta; //diff between open and close 

	vec2 offs = vec2(offsX, offsY);

	vec4 p = vec4( offs + vec2(20.f * pos.x, scaleY * pos.y), 0.f, 1.0); //to 4d for homog matr 
	p.y = 0;

	scaleY = volume / 500000000.f * .2f;
	
	gl_Position = projection * p;
	gl_Position.y = (pos.y + .5f) * scaleY;
	gl_Position.xy += vec2(-1.f, -1.f);

	vertColor = buy * vec4(0, 1, 0, 1) + (1.f - buy) * vec4(1, 0, 0, 1);
}