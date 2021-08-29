#version 330
#define PI 3.1415926538

layout (location = 0) in vec2 vp;
layout (location = 1) in vec4 pointcolour;
layout (location = 2) in uint timestamp;

uniform vec4 nswe;
uniform vec2 resolution;
uniform float seconds;
uniform float secondsbetweenhighlights;
uniform float traveltimebetweenhighlights;

out vec3 vcolour;

void main()
{
	float width; float height;
	float midx,midy;
	
	width = (nswe.w-nswe.z); 
	height = (nswe.x-nswe.y);
	midx = (nswe.w+nswe.z)/2; 
	midy = (nswe.x+nswe.y)/2; 
	
	float correctedLongitude=vp.x;


	if ((nswe.z<-180.0)&&(nswe.w<180.0)&&(vp.x>nswe.w))	{
		correctedLongitude-=360.0;
	}

	if ((nswe.w>=180.0)&&(nswe.z>-180.0)&&(vp.x<nswe.z))	{
		correctedLongitude+=360.0;
	}

	gl_Position = vec4(((correctedLongitude-midx)/width*2),(vp.y-midy)/height*2,0,1.0);
	
	vcolour = pointcolour.rgb; //vec3(0.7,0.9,0.25);

	uint m;
	m = (timestamp -uint(1262304000));
	
	m = (m % uint(3600*24*7)); //i think the number has trouble as so large

	float ts;
	ts = float(m);
	
	float sinehighlight;
	sinehighlight=sin(ts*PI/(traveltimebetweenhighlights)-PI*seconds/(secondsbetweenhighlights));
	sinehighlight*=sinehighlight;

	sinehighlight=smoothstep(0.85,0.95,sinehighlight);

	vcolour+=vec3(sinehighlight);
}