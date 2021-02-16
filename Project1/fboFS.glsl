#version 330
out vec4 FragColor;

uniform vec2 resolution;
  
uniform sampler2D screenTexture;

void main()
{ 
    vec2 uv=gl_FragCoord.xy/resolution.xy;
    FragColor = texture(screenTexture, uv);
}