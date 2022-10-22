#version 330
out vec4 FragColor;

uniform vec2 resolution;
uniform vec2 blurdirection;
uniform sampler2D preblurtexture;
uniform int sizeofblurdata;

uniform float gaussianoffset[100];
uniform float gaussianweight[100];

void main()
{ 
    vec4 accumulatedcolor=vec4(0.0);
    for (int i=0; i<sizeofblurdata;i++) {
        vec2 uv=((gl_FragCoord.xy+vec2(0.0))+gaussianoffset[i]*blurdirection)/resolution.xy;
        accumulatedcolor.r += gaussianweight[i]*texture(preblurtexture, uv).r;
    }
    FragColor=accumulatedcolor;
}