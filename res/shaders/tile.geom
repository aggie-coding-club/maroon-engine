#version 330 core

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

in VS_OUT {
	uint id; 
} gs_in[];

out vec2 tex_coord;

void main()
{
	vec4 view;
	uint id;
	vec2 tile;

	float s;

	vec4 pos;

	/*region seen on screen in tiles*/
	view = vec4(0.0, 0.0, 20.0, 15.0);

	/*convert id into base text coord*/
	id = gs_in[0].id & 255u;
	tile.x = float(id & 15u) / 16.0;
	tile.y = float((id >> 4u) & 15u) / 16.0;

	s = 1.0 / 16.0; /*number of tiles in row/col*/

	/*transform position into screen coords*/ 
	pos = gl_in[0].gl_Position;
	pos.xy += view.xy;
	pos.y += 1.0; /*flip y-axis: first step*/ 
	pos.xy /= view.zw;
	pos.xy *= 2.0; /*move origin from center to corner*/
	pos.xy -= 1.0;
	pos.y = -pos.y; /*flip y-axis: second step*/ 

	/*bottom left*/
	gl_Position = pos;
	tex_coord = tile + vec2(0.0, s);
	EmitVertex();

	/*bottom right*/
	gl_Position = pos + vec4(2.0/view.z, 0.0, 0.0, 0.0);
	tex_coord = tile + vec2(s, s);
	EmitVertex();

	/*top left*/
	gl_Position = pos + vec4(0.0, 2.0/view.w, 0.0, 0.0);
	tex_coord = tile + vec2(0.0, 0.0);
	EmitVertex();

	/*top right*/
	gl_Position = pos + vec4(2.0/view.zw, 0.0, 0.0);
	tex_coord = tile + vec2(s, 0.0);
	EmitVertex();

	EndPrimitive();
}
