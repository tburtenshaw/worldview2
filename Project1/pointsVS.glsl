#version 330
#define PI 3.1415926538

layout (location = 0) in vec2 vp;
layout (location = 1) in vec4 pointcolour;
layout (location = 2) in uint timestamp;

uniform vec4 nswe;
uniform vec2 resolution;
uniform float seconds;

uniform bool showhighlights;
uniform float secondsbetweenhighlights;
uniform float traveltimebetweenhighlights;

uniform uint earliesttimetoshow;
uniform uint latesttimetoshow;

uniform float cycleSeconds;	//for the rainbow display, the time between the colours of the rainbow.

uniform int colourby;
uniform vec4 palette[24];

out vec4 vcolour;
out uint ts;

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
    t+=uint(31536000);	//increase the date, so we start on a non-leap, after a leap year
	const uint fouryears = uint(31536000 * 3 + 31622400);	//365,365,365,366 days
	uint olympiad = t / fouryears;	//which group of four years
	uint remainder = min(uint(3), ((t - (olympiad * fouryears)) / uint(31536000)));
	
    uint yearcalc=olympiad * uint(4) + remainder + uint(1969);	//from 1969 as we went forward a year previously
    return palette[yearcalc % uint(24)];	//24 is the size of the palette for year, maybe pass as uniform
}

float expImpulse( float n, float k )
{
    float h = k*n;
    return h*exp(1.0-h);
}


vec3 hpluv(float t) {

    t *= 6.283185307179586;

    vec2 cs1 = vec2(cos(t), sin(t));

    vec3 n = vec3(0.5832271348571118, 0.5669143498918592, 0.5688901803565138);
    n += vec3(0.5579918385059167, -0.123397919673585, -0.04010425190513076)*cs1.x;
    n += vec3(0.07377028386927893, 0.1108109634096618, -0.3122855451190785)*cs1.y;

    vec3 d = vec3(1.0);
    d += vec3(0.5048729122168087, -0.086202826156657, -0.02961476768472726)*cs1.x;
    d += vec3(0.1355863216235928, 0.1557689334676045, -0.1491132071515487)*cs1.y;

    return n/d;

}


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
	
	vcolour = pointcolour.rgba; //vec3(0.7,0.9,0.25);
	
	
	if (colourby==1)	{
		vcolour = ColourByHour(timestamp);
	}
	else if (colourby==4)	{
		vcolour = ColourByYear(timestamp);
	}
	else if (colourby==2)	{
		vcolour = ColourByWeekday(timestamp);
	}
	else {
		vcolour = vec4(hpluv(float(int(timestamp -uint(1262304000))- 31536000 * 3 + 31622400)/cycleSeconds),1.0);
	}
	

	uint m;
	m = (timestamp -uint(1262304000));
	m = (m % uint(3600*24*365)); //i think the number has trouble as so large

	float ftime;
	ftime = float(m);

	float sinehighlight=0.0;

	if (showhighlights)	{
		sinehighlight=expImpulse(mod(-ftime/traveltimebetweenhighlights + seconds/(secondsbetweenhighlights),1),20.0);
	}


	vcolour+=vec4(sinehighlight, sinehighlight, sinehighlight,sinehighlight);
}