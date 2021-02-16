#version 330
uniform vec2 resolution;
uniform float pointradius;

out vec4 FragColor;
in vec3 gcolour;
in vec2 centre;

void main()
{
    vec2 uv;
    vec3 col;

    //float s=100/resolution.x;   //smooth amount
	
	uv=gl_FragCoord.xy/resolution*2;
	uv+=vec2(-1,-1);
    
    float d =distance(uv.xy,centre)*resolution.x/pointradius;
    
    col = gcolour * (1-smoothstep(0.9,1,d+0.2));

    FragColor = vec4(col, 1-smoothstep(0.9,1,d));
} 