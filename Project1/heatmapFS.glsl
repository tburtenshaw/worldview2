#version 330
uniform vec2 resolution;
//uniform float pointradius;

out vec4 fragColor;
in vec4 gcolour;
in vec2 centre;

float circle(vec2 uv, vec2 pos, float rad) {
float d = length(pos - uv) - rad;
return 1.0-clamp(d, 0.0, 1.0);
}

void main() {
float pointradius=2.0;

vec2 uv = gl_FragCoord.xy;

// Circle
vec2 tempPos=vec2(400.0);

tempPos=(centre/vec2(2.0)+vec2(0.5))*vec2(resolution.xy);

float circleFactor = circle(uv, tempPos, pointradius);

//circleFactor=1.0-clamp(length(tempPos-uv)-1.0,0.0,1.0);

//float stddev=1.0;
//const float pi=3.1415926;
//circleFactor=1/(2*pi*stddev*stddev)*exp(-((tempPos.x-uv.x)*(tempPos.x-uv.x)+(tempPos.y-uv.y)*(tempPos.y-uv.y))/2*stddev*stddev);


vec4 outputColour;
outputColour=vec4(gcolour.r*circleFactor, vec3(0.0));

fragColor=outputColour;
}

