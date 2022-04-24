#version 330
//uniform vec2 resolution;

out vec4 FragColor;
in vec4 gcolour;

void main()
{
    FragColor = vec4(gcolour.rgb, 0.3);
} 