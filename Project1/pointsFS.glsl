#version 330
uniform vec2 resolution;
uniform float pointradius;

out vec4 fragColor;
in vec3 gcolour;
in vec2 centre;

float circle(vec2 uv, vec2 pos, float rad) {
float d = length(pos - uv) - rad;
return 1.0-clamp(d, 0.0, 1.0);
}

void main() {

vec2 uv = gl_FragCoord.xy;

// Circle
vec2 tempPos=vec2(400.0);

tempPos=(centre/vec2(2.0)+vec2(0.5))*vec2(resolution.xy);

float circleFactor = circle(uv, tempPos, pointradius/2.0);

//fragColor = vec4(1.0,0.2,0.2,min(0.3,circleFactor));
fragColor=vec4(gcolour,circleFactor/4.0);
}

/*
//OLD
void main()
{

    vec2 uv;
    vec3 col;

    //float s=100/resolution.x;   //smooth amount
	
	uv=gl_FragCoord.xy/resolution*2;
	uv+=vec2(-1,-1);
    
    float d =distance(uv.xy,centre)*resolution.x/pointradius;
    
    col = gcolour * (1-smoothstep(0.9,1,d+0.2));

    FragColor = vec4(gcolour,0.25);//vec4(col, 1-smoothstep(0.9,1,d));

} 
*/