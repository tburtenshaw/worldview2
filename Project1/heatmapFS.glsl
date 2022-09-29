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
float pointradius=5.0;

vec2 uv = gl_FragCoord.xy;

// Circle
vec2 tempPos=vec2(400.0);

tempPos=(centre/vec2(2.0)+vec2(0.5))*vec2(resolution.xy);

float circleFactor = circle(uv, tempPos, pointradius);


vec4 outputColour;
outputColour=vec4(gcolour.r, vec3(0.0));

fragColor=outputColour;
}
