#version 330 core

layout (location = 0) in vec3 pos;
layout (location = 1) in uint id; 

out VS_OUT {
	uint id;
} vs_out;

void main()
{
	gl_Position = vec4(pos.x, pos.y, pos.z, 1);
	vs_out.id = id;
}

