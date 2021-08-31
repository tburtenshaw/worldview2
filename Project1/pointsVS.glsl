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

	int year;
if (t<uint(1293840000) )        {
        year=2010;
        return palette[year % 7];
}

if (t<uint(1325376000) )        {
        year=2011;
        return palette[year % 7];
}

if (t<uint(1356998400) )        {
        year=2012;
        return palette[year % 7];
}

if (t<uint(1388534400) )        {
        year=2013;
        return palette[year % 7];
}

if (t<uint(1420070400) )        {
        year=2014;
        return palette[year % 7];
}

if (t<uint(1451606400) )        {
        year=2015;
        return palette[year % 7];
}

if (t<uint(1483228800) )        {
        year=2016;
        return palette[year % 7];
}

if (t<uint(1514764800) )        {
        year=2017;
        return palette[year % 7];
}

if (t<uint(1546300800) )        {
        year=2018;
        return palette[year % 7];
}

if (t<uint(1577836800) )        {
        year=2019;
        return palette[year % 7];
}

if (t<uint(1609459200) )        {
        year=2020;
        return palette[year % 7];
}

if (t<uint(1640995200) )        {
        year=2021;
        return palette[year % 7];
}

if (t<uint(1672531200) )        {
        year=2022;
        return palette[year % 7];
}

if (t<uint(1704067200) )        {
        year=2023;
        return palette[year % 7];
}

if (t<uint(1735689600) )        {
        year=2024;
        return palette[year % 7];
}

if (t<uint(1767225600) )        {
        year=2025;
        return palette[year % 7];
}

if (t<uint(1798761600) )        {
        year=2026;
        return palette[year % 7];
}

if (t<uint(1830297600) )        {
        year=2027;
        return palette[year % 7];
}

if (t<uint(1861920000) )        {
        year=2028;
        return palette[year % 7];
}

if (t<uint(1893456000) )        {
        year=2029;
        return palette[year % 7];
}

if (t<uint(1924992000) )        {
        year=2030;
        return palette[year % 7];
}

if (t<uint(1956528000) )        {
        year=2031;
        return palette[year % 7];
}

if (t<uint(1988150400) )        {
        year=2032;
        return palette[year % 7];
}

if (t<uint(2019686400) )        {
        year=2033;
        return palette[year % 7];
}

if (t<uint(2051222400) )        {
        year=2034;
        return palette[year % 7];
}

if (t<uint(2082758400) )        {
        year=2035;
        return palette[year % 7];
}

if (t<uint(2114380800) )        {
        year=2036;
        return palette[year % 7];
}

if (t<uint(2145916800) )        {
        year=2037;
        return palette[year % 7];
}
	year=2038;

	return vec4(1.0);
	//return palette[year % 24];
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
	sinehighlight=expImpulse(mod(-ts/traveltimebetweenhighlights + seconds/(secondsbetweenhighlights),1),14.0);
	
	vcolour+=vec4(sinehighlight, sinehighlight, sinehighlight,sinehighlight);
}