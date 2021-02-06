#version 330
layout (location = 0) in vec2 vp;
//layout (location = 1) in uint ts;
layout (location = 1) in vec4 colour;
layout (location = 2) in float detail;

uniform float seconds;
uniform float cycle;
uniform vec4 nswe;
uniform vec2 resolution;


out VS_OUT {
    vec3 color;
	float dontdraw;		//whether or not we draw the (?next) line
	vec2 origcoords;	//these are the map coords
} vs_out;

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 rainbow(float f)
{
    //thanks jodie, https://www.shadertoy.com/view/4l2cDm
	
	return sqrt(sin( (f+vec3(0,2,1)/3.)*2*3.14159265359 ) * .5 + .5);
}



void main() {
	vs_out.origcoords=vp;

	float width; float height; float midx,midy;
	width = (nswe.w-nswe.z); 
	height = (nswe.x-nswe.y);
	midx = (nswe.w+nswe.z)/2; 
	midy = (nswe.x+nswe.y)/2; 
	
	gl_Position = vec4(((vp.x-midx)/width*2),(vp.y-midy)/height*2,0,1.0);
	
	//uint m;
	//float f;
	
	//m = (ts - uint(seconds)) % uint(cycle); 


	//f = float(m)/cycle;
	

	//vs_out.color=rainbow(f);
	//vs_out.color=hsv2rgb(vec3(f,1.0,1.0));
	
	vs_out.color=colour.rgb;

	float res;
	vs_out.dontdraw=1.0;
	
	//workout the resolution, then if the detail is needed, flag this as a line to draw
	res = (nswe.w-nswe.z)/resolution.x;

	if (detail>res)	{
		vs_out.dontdraw=0.0;
	}

}