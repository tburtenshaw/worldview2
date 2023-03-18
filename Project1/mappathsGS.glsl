 #version 330

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

uniform vec2 resolution;
uniform float linewidth;
uniform vec4 nswe;
uniform float dpphoriz;

in VS_OUT {
    vec4 color;
	float detaillevel;
	vec2 origcoords;	//these are the map coords
	float timefrom2010;
} gs_in[]; 

out vec4 fcol;
out vec2 pointa;
out vec2 pointb;


void main()
{
    if (gs_in[1].detaillevel<dpphoriz) {	//if the detail level isn't needed to bother showing
		EndPrimitive();
		return;
	}
	
    vec2 start = gl_in[0].gl_Position.xy;
    vec2 end = gl_in[1].gl_Position.xy;


	if ((start.x-end.x)*(nswe.w-nswe.z) >360.0)	{
		end=end+vec2(720.0/(nswe.w-nswe.z),0.0);
	}

	if ((end.x-start.x)*(nswe.w-nswe.z) >360.0)	{
		start=start+vec2(720.0/(nswe.w-nswe.z),0.0);
	}

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




	//For the frag shader if required.
	pointa=start;
	pointb=end;
	float linelength = distance(start,end);
	

	//this gets a perpendicular vector of length one
	vec2 slope1;
	slope1=normalize(start-end);
	slope1.xy=slope1.yx;
	slope1.y=-slope1.y;

	vec2 thickness1 =thickness*slope1;

	vec2 extendlength = normalize(start-end)*thickness;



	//fcol = gs_in[0].color;
	fcol=vec4(linelength/(gs_in[1].timefrom2010 - gs_in[0].timefrom2010)*360.0);

	fcol.a=min(1.0/linelength,1.0);	//fades away long lines (i.e. crossing the screen)
	fcol.a=pow(fcol.a,1.5);
	
	gl_Position = vec4(start,1.0,1.0) + vec4(-thickness1, 0.0, 0.0) + vec4(extendlength, 0.0, 0.0);
	EmitVertex();

	gl_Position = vec4(start,1.0,1.0) + vec4(thickness1, 0.0, 0.0)+ vec4(extendlength, 0.0, 0.0);
    EmitVertex();

	//fcol = gs_in[1].color;
//	fcol.a=min(1.0/linelength,1.0);

	gl_Position = vec4(end,1.0,1.0) + vec4(-thickness1, 0.0, 0.0)- vec4(extendlength, 0.0, 0.0);
    EmitVertex();

	gl_Position = vec4(end,1.0,1.0) + vec4( thickness1, 0.0, 0.0) - vec4(extendlength, 0.0, 0.0);
    EmitVertex();


	EndPrimitive();
}
