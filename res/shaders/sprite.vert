#version 330 core

layout (location = 0) in ivec2 pos;
layout (location = 1) in uint layer; 
layout (location = 2) in uint id; 
layout (location = 3) in uint flip;

out VS_OUT {
	uint id;
	uint flip;
} vs_out;

void main()
{
	vec2 upos;

	upos = vec2(pos) / 32.0F;
	gl_Position = vec4(upos, float(layer) / 256.0F, 1);
	vs_out.id = id;
	vs_out.flip = flip;
}

