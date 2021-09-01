 #version 330

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

uniform vec2 resolution;
uniform float linewidth;
uniform vec4 nswe;

in VS_OUT {
    vec4 color;
	float dontdraw;
	vec2 origcoords;	//these are the map coords

} gs_in[]; 

out vec4 fcol;
out vec2 pointa;
out vec2 pointb;


void main()
{
    if (gs_in[1].dontdraw!=0.0) {
		EndPrimitive();
		return;
	}
	
    vec2 start = gl_in[0].gl_Position.xy;
    vec2 end = gl_in[1].gl_Position.xy;

	vec2 thickness;
	thickness=(linewidth+2)/resolution; //extra little bit to allow for aliasing

	//if both coordinates too far west, don't bother
	if ((gs_in[0].origcoords.x<nswe.z-thickness.x)&&(gs_in[1].origcoords.x<nswe.z-thickness.x))	{
		EndPrimitive();
		return;
	}
	//same with east
	if ((gs_in[0].origcoords.x>nswe.w+thickness.x)&&(gs_in[1].origcoords.x>nswe.w+thickness.x))	{
		EndPrimitive();
		return;
	}
	
	//culling the too far east and west positions is probably good enough.
	//nswe.x is north
	//if ((gs_in[0].origcoords.y>nswe.x+thickness.y)&&(gs_in[1].origcoords.y>nswe.x+thickness.y))	{
	//	EndPrimitive();
	//	return;
	//}


	float linelength;
	linelength = distance(start,end);

	pointa=start;
	pointb=end;


	//this gets a perpendicular vector of length one
	vec2 slope1;
	slope1=normalize(start-end);
	slope1.xy=slope1.yx;
	slope1.y=-slope1.y;

	vec2 thickness1 =thickness*slope1;

	vec2 extendlength = normalize(start-end)*thickness;


	fcol = gs_in[0].color;
	
	gl_Position = vec4(start,1.0,1.0) + vec4(-thickness1, 0.0, 0.0) + vec4(extendlength, 0.0, 0.0);
	EmitVertex();

	gl_Position = vec4(start,1.0,1.0) + vec4(thickness1, 0.0, 0.0)+ vec4(extendlength, 0.0, 0.0);
    EmitVertex();

	
	
	fcol = gs_in[1].color;

//	if (start.x-end.x > (0.0000000005/ ((nswe.w-nswe.z)/resolution.x)))	{
		////end+=vec2(1.0/ ((nswe.w-nswe.z)/resolution.x),100.0);
		//fcol=vec4(1.0,1.0,1.0,1.0);
	//}


	gl_Position = vec4(end,1.0,1.0) + vec4(-thickness1, 0.0, 0.0)- vec4(extendlength, 0.0, 0.0);
    EmitVertex();

	gl_Position = vec4(end,1.0,1.0) + vec4( thickness1, 0.0, 0.0) - vec4(extendlength, 0.0, 0.0);
    EmitVertex();


	EndPrimitive();
}
