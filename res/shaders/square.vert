#version 330 core

layout (location = 0) in uvec2 pos;
layout (location = 1) in uint layer; 
layout (location = 2) in uint id; 

out VS_OUT {
	uint id;
} vs_out;

void main()
{
	vec2 upos;

	upos = vec2(pos) / 32.0F;
	gl_Position = vec4(upos, float(layer) / 256.0F, 1);
	vs_out.id = id;
}

