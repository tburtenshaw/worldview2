#version 330
//uniform vec2 resolution;

out vec4 FragColor;
in vec3 gcolour;

void main()
{
    FragColor = vec4(gcolour, 0.3);
} 