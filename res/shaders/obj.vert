#version 330 core

layout (location = 0) in vec2 pos;
layout (location = 1) in uint id; 
layout (location = 2) in uint flags; 

out VS_OUT {
	uint id;
} vs_out;

void main()
{
	gl_Position = vec4(pos.x, pos.y, -1, 1);
	vs_out.id = id;
}

