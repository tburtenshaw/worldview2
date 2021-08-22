#version 330
layout (location = 0) in vec4 vp;

uniform vec4 nswe;
uniform vec2 resolution;

//in vec3  gcolour;
out vec3 vcolour;

void main()
{
	float width; float height; float midx,midy;
	width = (nswe.w-nswe.z); 
	height = (nswe.x-nswe.y);
	midx = (nswe.w+nswe.z)/2; 
	midy = (nswe.x+nswe.y)/2; 
	
	gl_Position = vec4(((vp.x-midx)/width*2),(vp.y-midy)/height*2,0,1.0);
	
	
	
	//gl_Position=vec4(vp,0.0,0.0)+vec4(0.0,0.0,0.0,0.0);//(vp/vec2(400.0)+vec2(0.0,0.0),0.0,0.0);
	//gl_Position=vp;
	vcolour = vec3(0.7,0.9,vp.x/2);
	//vcolour=gcolour;
}