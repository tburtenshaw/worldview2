#version 330

layout (location = 0) in vec2 vp;
layout (location = 1) in uint timestamp;

uniform vec4 nswe;
uniform vec2 resolution;
//uniform float seconds;

//uniform uint earliesttimetoshow;
//uniform uint latesttimetoshow;

out uint ts;


void main()
{
	float width; float height;
	float midx,midy;
	
	width = (nswe.w-nswe.z); 
	height = (nswe.x-nswe.y);
	midx = (nswe.w+nswe.z)/2; 
	midy = (nswe.x+nswe.y)/2; 
	
	ts=timestamp;	//pass on to the geometry shader

	float correctedLongitude=vp.x;

	if ((nswe.z<-180.0)&&(nswe.w<180.0)&&(vp.x>nswe.w))	{
		correctedLongitude-=360.0;
	}

	if ((nswe.w>=180.0)&&(nswe.z>-180.0)&&(vp.x<nswe.z))	{
		correctedLongitude+=360.0;
	}

	gl_Position = vec4(((correctedLongitude-midx)/width*2),(vp.y-midy)/height*2,0.0,1.0);
	

}