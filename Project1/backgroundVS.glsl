#version 400

in vec2 vp;
uniform float seconds;


void main() {
	vec2 outvp;

	outvp=vp;
	gl_Position = vec4(outvp, 0.0, 1.0);
	
}
