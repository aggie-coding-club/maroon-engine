#version 330 core

layout (location = 0) in uint id;

uniform vec2 scroll;
uniform int layer;

out VS_OUT {
	uint id;
} vs_out;

void main()
{
	vec2 t;
	vec2 p;
 
	/*partial 1D to 2D index transformation*/
	t = vec2(gl_VertexID, gl_VertexID >> 5); 

	/*adjust position by scroll and wrap around*/
	p = mod(t + scroll + 1.0, 32) - 1.0;

	gl_Position = vec4(p, -float(layer) / 16.0, 1.0);
	vs_out.id = id;
}
