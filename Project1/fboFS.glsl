#version 330 core
out vec4 FragColor;

uniform vec2 resolution;
  
uniform sampler2D screenTexture;

void main()
{ 
    vec2 uv=gl_FragCoord.xy/resolution;
    FragColor = texture(screenTexture, uv)+vec4(0.0,-0.1,uv.x/10.0,0.0);
}