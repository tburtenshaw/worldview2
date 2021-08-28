#version 330
layout (location = 0) in vec2 vp;
layout (location = 1) in vec4 pointcolour;

uniform vec4 nswe;
uniform vec2 resolution;

out vec3 vcolour;

void main()
{
	float width; float height; float midx,midy;
	width = (nswe.w-nswe.z); 
	height = (nswe.x-nswe.y);
	midx = (nswe.w+nswe.z)/2; 
	midy = (nswe.x+nswe.y)/2; 
	
	gl_Position = vec4(((vp.x-midx)/width*2),(vp.y-midy)/height*2,0,1.0);
	
	vcolour = pointcolour.rgb; //vec3(0.7,0.9,0.25);
}