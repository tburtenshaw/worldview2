#version 330

uniform vec2 resolution;
uniform sampler2D worldTexture;
uniform sampler2D heatmapTexture;

uniform vec4 nswe;	//the view
uniform vec4 heatmapnswe;	//where the heatmap covers

uniform float maxheatmapvalue;
uniform float linewidth;
uniform int palette;

out vec4 frag_colour;


//https://www.shadertoy.com/view/WlfXRN
vec3 inferno(float t) {

    const vec3 c0 = vec3(0.0002189403691192265, 0.001651004631001012, -0.01948089843709184);
    const vec3 c1 = vec3(0.1065134194856116, 0.5639564367884091, 3.932712388889277);
    const vec3 c2 = vec3(11.60249308247187, -3.972853965665698, -15.9423941062914);
    const vec3 c3 = vec3(-41.70399613139459, 17.43639888205313, 44.35414519872813);
    const vec3 c4 = vec3(77.162935699427, -33.40235894210092, -81.80730925738993);
    const vec3 c5 = vec3(-71.31942824499214, 32.62606426397723, 73.20951985803202);
    const vec3 c6 = vec3(25.13112622477341, -12.24266895238567, -23.07032500287172);

    return c0+t*(c1+t*(c2+t*(c3+t*(c4+t*(c5+t*c6)))));

}

vec3 viridis_quintic( float x )
{
	vec4 x1 = vec4( 1.0, x, x * x, x * x * x ); // 1 x x2 x3
	vec4 x2 = x1 * x1.w * x; // x4 x5 x6 x7
	return vec3(
		dot( x1.xyzw, vec4( +0.280268003, -0.143510503, +2.225793877, -14.815088879 ) ) + dot( x2.xy, vec2( +25.212752309, -11.772589584 ) ),
		dot( x1.xyzw, vec4( -0.002117546, +1.617109353, -1.909305070, +2.701152864 ) ) + dot( x2.xy, vec2( -1.685288385, +0.178738871 ) ),
		dot( x1.xyzw, vec4( +0.300805501, +2.614650302, -12.019139090, +28.933559110 ) ) + dot( x2.xy, vec2( -33.491294770, +13.762053843 ) ) );
}



vec4 FloatToColour(float vraw, float mraw)	{
	
	if (vraw>2)	{
		vec4 c;
		float r=log(vraw)/log(mraw); //value/max
		float a=smoothstep(0.0,0.01,r);
		if (palette==1)	{
			c=vec4(inferno(r),a);
		}
		else {
			c=vec4(viridis_quintic(r),a);
		}
		return 	c;
	}
	
	return vec4(0.0);
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
	
	/*
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