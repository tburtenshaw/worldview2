#version 330
uniform vec2 resolution;

out vec4 fragColor;
in float timespent;

void main() {
	vec4 outputColour;
	fragColor=vec4(timespent, vec3(0.0));
}

