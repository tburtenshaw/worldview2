#version 330

uniform vec2 resolution;
uniform float linewidth;

out vec4 frag_colour;
in vec4 fcol;

in vec2 pointa;
in vec2 pointb;

//thanks https://www.shadertoy.com/view/tt23WW

float dline( vec2 p, vec2 a, vec2 b ) {
    
   
    vec2 v = a, w = b;
    
    float l2 = pow(distance(w, v), 2.);
    if(l2 == 0.0) return distance(p, v);
    
    float t = clamp(dot(p - v, w - v) / l2, 0., 1.);
    vec2 j = v + t * (w - v);
    
    return distance(p, j);
    
}
//consider https://www.shadertoy.com/view/WtVyz3 instead

/*
void LRyirLine(float width)
{
    vec2 point1 = vec2(iResolution * 0.5f);
    vec2 point2 = vec2(iMouse);
    
    vec2 a = (point2-point1)/length(point2-point1);
    vec2 b = fragCoord - point1;
    vec2 p = clamp(dot(a, b), 0.f, length(point2-point1)) * a + point1;
    
    float width = 20.f;
    return length(p - fragCoord) - width;
}
*/


void main() {
	vec2 uv;
	
	uv=gl_FragCoord.xy/resolution*2;
	uv+=vec2(-1,-1);

	
	float uvwidth =linewidth/resolution.x;
    float blur=min(3.0,linewidth)/resolution.x;

    float opacity = (smoothstep(uvwidth+blur,uvwidth,dline( uv, pointa, pointb ))); //this blurs the edges of the line (really a quadrilateral) to help avoid aliasing

    // Output to screen
    frag_colour = vec4(fcol.xyz,opacity*fcol.w);  
}