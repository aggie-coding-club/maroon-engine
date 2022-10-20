#version 330 core

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

uniform vec2 view;

in VS_OUT {
	uint id; 
} gs_in[];

out vec2 tex_coord;

void main()
{
	uint id;
	vec2 tile;

	float b;
	float s;

	vec4 pos;

	/*convert id into base text coord*/
	id = gs_in[0].id & 255u;
	tile.x = float(id & 15u) / 16.0F;
	tile.y = float((id >> 4u) & 15u) / 16.0F;

	b = 1.0F / 4096.0F;
	s = 1.0F / 16.0F - b; /*number of tiles in row/col*/

	/*transform position into screen coords*/ 
	pos = gl_in[0].gl_Position;
	pos.y += 1.0F; /*flip y-axis: first step*/ 
	pos.xy /= view.xy;
	pos.xy *= 2.0F; /*move origin from center to corner*/
	pos.xy -= 1.0F;
	pos.y = -pos.y; /*flip y-axis: second step*/ 

	/*bottom left*/
	gl_Position = pos;
	tex_coord = tile + vec2(b, s);
	EmitVertex();

	/*bottom right*/
	gl_Position = pos + vec4(2.0F/view.x, 0.0F, 0.0F, 0.0F);
	tex_coord = tile + vec2(s, s);
	EmitVertex();

	/*top left*/
	gl_Position = pos + vec4(0.0F, 2.0F/view.y, 0.0F, 0.0F);
	tex_coord = tile + vec2(b, b);
	EmitVertex();

	/*top right*/
	gl_Position = pos + vec4(2.0F/view.xy, 0.0F, 0.0F);
	tex_coord = tile + vec2(s, b);
	EmitVertex();

	EndPrimitive();
}
