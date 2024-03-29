#version 330
layout (location = 0) in vec2 vp;
layout (location = 1) in uint ts;
layout (location = 2) in float detail;

uniform float seconds;
uniform float cycle;
uniform vec4 nswe;
uniform vec2 resolution;
uniform vec2 degreespan;
uniform vec2 degreemidpoint;

out VS_OUT {
    vec4 color;
	float detaillevel;		//whether or not we draw the (?next) line
	vec2 origcoords;	//these are the map coords
	float timefrom2010;
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
	
	return sqrt(sin( (f+vec3(0.0,2.0,1.0)/3.0)*2.0*3.14159265359 ) * .5 + .5);
}



void main() {
	vs_out.origcoords=vp;
	vs_out.detaillevel=detail;

	gl_Position = vec4((2.0*(vp-degreemidpoint)/degreespan),0.0,1.0);

	//temp
	vs_out.color=vec4(rainbow(float(ts/uint(60*60))/24.0),0.9);

	//vs_out.timefrom2010 = float(ts-uint(1262304000));
	vs_out.timefrom2010 = float(ts-uint(1500000000));
}