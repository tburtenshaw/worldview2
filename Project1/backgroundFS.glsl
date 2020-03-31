#version 330

uniform vec2 resolution;
uniform sampler2D worldTexture;
uniform sampler2D heatmapTexture;

uniform vec4 nswe;	//the view
uniform vec4 heatmapnswe;	//where the heatmap covers

uniform float maxheatmapvalue;
uniform float linewidth;
uniform float c;

out vec4 frag_colour;

vec4 FloatToColour(float vraw, float mraw)	{
	vec4 clear=	vec4(0,0,0,0);
	vec4 red=	vec4(1.0,0.0,00.,0.8);
	vec4 yellow=vec4(1.0,1.0,0.0,1.0);
	vec4 white=	vec4(1.0,1.0,1.0,1.0);

	float v=log(vraw);
	float m=log(mraw);

	float step1=0.0;
	float step2=0.05;
	float step3=0.4;

	vec4 color;

	color = mix(clear, red, smoothstep(step1, step2, v/m));
	color = mix(color, yellow, smoothstep(step2, step3, v/m));
	color = mix(color, white, smoothstep(step3, 1, v/m));
	
	return color;
}

vec4 AlphaOnOpaqueMix(vec3 dest, vec4 source, float strength)	{
//we make the dest vec3 as it'll be fully opaque, and we help flag a mix up
	vec4 outputcol;
	
	float overallalpha=source.a*strength;
	
	outputcol.a=1.0;
	outputcol.rgb=source.rgb*overallalpha + dest.rgb * (1-overallalpha);

	return outputcol;
}

void main() {
	float width = (nswe.w-nswe.z); 
	float height = (nswe.x-nswe.y);

	vec2 uv=vec2(1,-1)*gl_FragCoord.xy/resolution;
	uv*=vec2(width,height)/vec2(360,180); //convert the NSWE locations to between 0 and 1
	uv+=vec2(nswe.z+180,nswe.y-90)/vec2(360,-180); //shift them so aligned right

    // Output to screen    
	vec4 t;	//mix of world and heatmap

	vec4 wt=texture(worldTexture, uv);
	
	vec2 heatmapuv;
	float heatmapwidth, heatmapheight;
	heatmapwidth = (heatmapnswe.w-heatmapnswe.z);
	heatmapheight = (heatmapnswe.x-heatmapnswe.y);

	//heatmapuv=vec2(1/(uv.x*c), gl_FragCoord.y*height/heatmapheight);	//get the right width and height
	
	heatmapuv.x=uv.x/(heatmapwidth/360);
	heatmapuv.x-=(heatmapnswe.z+180)/heatmapwidth;

	heatmapuv.y=uv.y/(heatmapheight/180);
	heatmapuv.y+=(heatmapnswe.x-90)/heatmapheight;

	//do a gaussian blur
	vec2 heatmappix = 1/vec2(2048,2048);


	float heatvalue;
	heatvalue=texture(heatmapTexture, heatmapuv).r;
	
	//5x5 gaussian //i'll need to move this to the CPU, was just a trial
	heatvalue*=41;
	heatvalue+=texture(heatmapTexture, heatmapuv+heatmappix*vec2(0,1)).r*26;
	heatvalue+=texture(heatmapTexture, heatmapuv+heatmappix*vec2(0,-1)).r*26;
	heatvalue+=texture(heatmapTexture, heatmapuv+heatmappix*vec2(0,2)).r*7;
	heatvalue+=texture(heatmapTexture, heatmapuv+heatmappix*vec2(0,-2)).r*7;

	heatvalue+=texture(heatmapTexture, heatmapuv+heatmappix*vec2(-1,0)).r*26;
	heatvalue+=texture(heatmapTexture, heatmapuv+heatmappix*vec2(-1,1)).r*16;
	heatvalue+=texture(heatmapTexture, heatmapuv+heatmappix*vec2(-1,-1)).r*16;
	heatvalue+=texture(heatmapTexture, heatmapuv+heatmappix*vec2(-1,2)).r*4;
	heatvalue+=texture(heatmapTexture, heatmapuv+heatmappix*vec2(-1,-2)).r*4;

	heatvalue+=texture(heatmapTexture, heatmapuv+heatmappix*vec2(-2,0)).r*7;
	heatvalue+=texture(heatmapTexture, heatmapuv+heatmappix*vec2(-2,1)).r*4;
	heatvalue+=texture(heatmapTexture, heatmapuv+heatmappix*vec2(-2,-1)).r*4;
	heatvalue+=texture(heatmapTexture, heatmapuv+heatmappix*vec2(-2,2)).r*1;
	heatvalue+=texture(heatmapTexture, heatmapuv+heatmappix*vec2(-2,-2)).r*1;


	heatvalue+=texture(heatmapTexture, heatmapuv+heatmappix*vec2(1,0)).r*26;
	heatvalue+=texture(heatmapTexture, heatmapuv+heatmappix*vec2(1,1)).r*16;
	heatvalue+=texture(heatmapTexture, heatmapuv+heatmappix*vec2(1,-1)).r*16;
	heatvalue+=texture(heatmapTexture, heatmapuv+heatmappix*vec2(1,2)).r*4;
	heatvalue+=texture(heatmapTexture, heatmapuv+heatmappix*vec2(1,-2)).r*4;

	heatvalue+=texture(heatmapTexture, heatmapuv+heatmappix*vec2(2,0)).r*7;
	heatvalue+=texture(heatmapTexture, heatmapuv+heatmappix*vec2(2,1)).r*4;
	heatvalue+=texture(heatmapTexture, heatmapuv+heatmappix*vec2(2,-1)).r*4;
	heatvalue+=texture(heatmapTexture, heatmapuv+heatmappix*vec2(2,2)).r*1;
	heatvalue+=texture(heatmapTexture, heatmapuv+heatmappix*vec2(2,-2)).r*1;

	heatvalue/=273;

	/*
	//this is a 1x7 blue
	heatvalue*=20;
	heatvalue+=texture(heatmapTexture, heatmapuv+heatmappix*vec2(0,1)).r*15;
	heatvalue+=texture(heatmapTexture, heatmapuv+heatmappix*vec2(0,-1)).r*15;

	heatvalue+=texture(heatmapTexture, heatmapuv+heatmappix*vec2(0,2)).r*6;
	heatvalue+=texture(heatmapTexture, heatmapuv+heatmappix*vec2(0,-2)).r*6;

	heatvalue+=texture(heatmapTexture, heatmapuv+heatmappix*vec2(0,3)).r*1;
	heatvalue+=texture(heatmapTexture, heatmapuv+heatmappix*vec2(0,-3)).r*1;

	heatvalue/=64;
	*/

	vec4 ht=FloatToColour(heatvalue, maxheatmapvalue);
	
	//t=mix(wt,ht,0.5);
	t=AlphaOnOpaqueMix(wt.rgb,ht,1.0);

	vec2 sunloc;
	sunloc=vec2(0.5,-0.7);

	float specular =0.0;
	if (t.b>t.g+t.r)	{
		specular= pow(clamp(1.0-distance(uv,sunloc),0.0,1.0),30);
		}
	
	frag_colour = t+vec4(specular);
}