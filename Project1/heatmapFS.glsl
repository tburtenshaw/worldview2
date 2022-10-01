#version 330
uniform vec2 resolution;

out vec4 fragColor;
in float timespent;
in vec2 centre;
in float pointradius;

float circle(vec2 uv, vec2 pos, float rad) {
float d = length(pos - uv) - rad;
return 1.0-clamp(d, 0.0, 1.0);
}

void main() {

vec2 uv = gl_FragCoord.xy;

// Circle
vec2 tempPos;

tempPos=(centre/vec2(2.0)+vec2(0.5))*vec2(resolution.xy);

float circleFactor = circle(uv, tempPos, pointradius);
//it's a bit pointless doing nice circle aliasing, as it's lost with the logarithm anyway
//also tried gaussian function with exp()s, which was far too slow.
//best to keep simple shape, and maybe make it look better in the background shader

circleFactor =1.0;

vec4 outputColour;
outputColour=vec4(timespent*circleFactor, vec3(0.0));

fragColor=outputColour;
}

