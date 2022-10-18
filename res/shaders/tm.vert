#version 330 core

layout (location = 0) in uint id;

uniform vec2 scroll;

out VS_OUT {
	uint id;
} vs_out;

void main()
{
	vec2 t;
	vec2 p;
	float layer;
	float z;
 
	/*partial 1D to 2D index transformation*/
	t = vec2(gl_VertexID, gl_VertexID >> 5); 

	/*adjust position by scroll and wrap around*/
	p = mod(t + scroll + 1.0, 32) - 1.0;

	/*layer zero is bottom*/
	layer = -float(gl_VertexID / 1024);
	z = layer / 16.0;

	gl_Position = vec4(p, z, 1.0);
	vs_out.id = id;
}
