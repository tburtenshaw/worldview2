#version 330

layout (location = 0) in vec2 vp;
layout (location = 1) in uint timestamp;
layout (location = 2) in uint accuracy;

uniform vec4 nswe;	//the viewpoint
uniform vec2 degreespan;
uniform vec2 degreemidpoint;


out uint ts;
out uint acc;


void main()
{
	ts=timestamp;	//pass on to the geometry shader
	acc=accuracy;

	vec2 correctedLongitude=vp;

	if ((nswe.z<-180.0)&&(nswe.w<180.0)&&(vp.x>nswe.w))	{
		correctedLongitude.x-=360.0;
	}

	if ((nswe.w>=180.0)&&(nswe.z>-180.0)&&(vp.x<nswe.z))	{
		correctedLongitude.x+=360.0;
	}

	gl_Position = vec4(((correctedLongitude-degreemidpoint)/degreespan*2.0),0.0,1.0);

}