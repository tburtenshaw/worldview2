#version 330
layout (location = 0) in vec2 westnorth;
layout (location = 1) in vec2 eastsouth;
layout (location = 2) in vec4 regioncolour;

uniform vec4 nswe;
uniform vec2 resolution;

out vec4 vcolour;
out vec2 bottomright;

void main()
{
	float width; float height; float midx,midy;
	width = (nswe.w-nswe.z); 
	height = (nswe.x-nswe.y);
	midx = (nswe.w+nswe.z)/2; 
	midy = (nswe.x+nswe.y)/2; 
	
	gl_Position = vec4(((westnorth.x-midx)/width*2),(westnorth.y-midy)/height*2,0,1.0);
	bottomright= vec2((eastsouth.x-midx)/width*2,(eastsouth.y-midy)/height*2);


	vcolour = regioncolour;
}