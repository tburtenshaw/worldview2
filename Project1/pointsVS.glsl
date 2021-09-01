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

uniform uint earliesttimetoshow;
uniform uint lastesttimetoshow;

uniform int colourby;
uniform vec4 palette[24];

out vec4 vcolour;

vec4 ColourByWeekday(uint t)
{

	int day=int(((t / uint(86400)) + uint(4)) % uint(7));

	return palette[day];
}

vec4 ColourByHour(uint t)
{
	uint secondsthroughday = t % uint(3600 * 24);
	int hour = 24 * int(secondsthroughday) / (3600 * 24);

	return palette[hour];
}

vec4 ColourByYear(uint t)
{
    t+=uint(31536000);
	const uint fouryears = uint(31536000 * 3 + 31622400);
	uint olympiad = t / fouryears;
	uint remainder = min(uint(3), ((t - (olympiad * fouryears)) / uint(31536000)));
	
    uint yearcalc=olympiad * uint(4) + remainder + uint(1969);
    return palette[yearcalc % uint(7)];
}

float expImpulse( float n, float k )
{
    float h = k*n;
    return h*exp(1.0-h);
}


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

	gl_Position = vec4(((correctedLongitude-midx)/width*2),(vp.y-midy)/height*2,0.0,1.0);
	
	vcolour = pointcolour.rgba; //vec3(0.7,0.9,0.25);
	
	
	if (colourby==1)	{
		vcolour = ColourByHour(timestamp);
	}
	else if (colourby==4)	{
		vcolour = ColourByYear(timestamp);
	}
	else vcolour = ColourByWeekday(timestamp);
	

	uint m;
	m = (timestamp -uint(1262304000));
	
	m = (m % uint(3600*24*7)); //i think the number has trouble as so large

	float ts;
	ts = float(m);
	
	float sinehighlight;
	sinehighlight=expImpulse(mod(-ts/traveltimebetweenhighlights + seconds/(secondsbetweenhighlights),1),20.0);
	
	vcolour+=vec4(sinehighlight, sinehighlight, sinehighlight,sinehighlight);
}