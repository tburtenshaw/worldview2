#version 330

uniform sampler2D worldTexture;
uniform sampler2D highresTexture;
uniform sampler2D heatmapTexture;

uniform vec2 resolution;
uniform vec4 nswe;	//the view
uniform vec2 degreespan;

uniform int atlascount;
uniform vec4 atlasnswe[8];
uniform vec2 atlasmult[8];
uniform vec2 atlasadd[8];



uniform float maxheatmapvalue;
uniform int palette;

uniform vec4 palettearray[16];
uniform int palettesize;


out vec4 frag_colour;

vec3 inferno(float t) {
	//https://www.shadertoy.com/view/WlfXRN
    const vec3 c0 = vec3(0.0002189403691192265, 0.001651004631001012, -0.01948089843709184);
    const vec3 c1 = vec3(0.1065134194856116, 0.5639564367884091, 3.932712388889277);
    const vec3 c2 = vec3(11.60249308247187, -3.972853965665698, -15.9423941062914);
    const vec3 c3 = vec3(-41.70399613139459, 17.43639888205313, 44.35414519872813);
    const vec3 c4 = vec3(77.162935699427, -33.40235894210092, -81.80730925738993);
    const vec3 c5 = vec3(-71.31942824499214, 32.62606426397723, 73.20951985803202);
    const vec3 c6 = vec3(25.13112622477341, -12.24266895238567, -23.07032500287172);

    return c0+t*(c1+t*(c2+t*(c3+t*(c4+t*(c5+t*c6)))));

}

vec3 viridis(float t) {

    const vec3 c0 = vec3(0.2777273272234177, 0.005407344544966578, 0.3340998053353061);
    const vec3 c1 = vec3(0.1050930431085774, 1.404613529898575, 1.384590162594685);
    const vec3 c2 = vec3(-0.3308618287255563, 0.214847559468213, 0.09509516302823659);
    const vec3 c3 = vec3(-4.634230498983486, -5.799100973351585, -19.33244095627987);
    const vec3 c4 = vec3(6.228269936347081, 14.17993336680509, 56.69055260068105);
    const vec3 c5 = vec3(4.776384997670288, -13.74514537774601, -65.35303263337234);
    const vec3 c6 = vec3(-5.435455855934631, 4.645852612178535, 26.3124352495832);

    return c0+t*(c1+t*(c2+t*(c3+t*(c4+t*(c5+t*c6)))));

}

vec3 turbo(float t) {
	//https://www.shadertoy.com/view/3lBXR3
	//https://ai.googleblog.com/2019/08/turbo-improved-rainbow-colormap-for.html
    const vec3 c0 = vec3(0.1140890109226559, 0.06288340699912215, 0.2248337216805064);
    const vec3 c1 = vec3(6.716419496985708, 3.182286745507602, 7.571581586103393);
    const vec3 c2 = vec3(-66.09402360453038, -4.9279827041226, -10.09439367561635);
    const vec3 c3 = vec3(228.7660791526501, 25.04986699771073, -91.54105330182436);
    const vec3 c4 = vec3(-334.8351565777451, -69.31749712757485, 288.5858850615712);
    const vec3 c5 = vec3(218.7637218434795, 67.52150567819112, -305.2045772184957);
    const vec3 c6 = vec3(-52.88903478218835, -21.54527364654712, 110.5174647748972);

    return c0+t*(c1+t*(c2+t*(c3+t*(c4+t*(c5+t*c6)))));

}

vec4 EvenSpacedLinearPoints(float t)	{
	int n=int(t*float(palettesize-1));
    float samplePlace=fract(t*float(palettesize-1));
   
    return mix(palettearray[n],palettearray[n+1],samplePlace);
}

vec4 FloatToColour(float vraw, float mraw)	{
	
	if (vraw>0.0)	{
		vec4 c;
		float r=log(vraw+1.0)/log(mraw+1.0); //value/max
		
		c= EvenSpacedLinearPoints(r);

		float a=smoothstep(0.0,0.05,r);

		return c*vec4(1.0,1.0,1.0,a);

		if (palette==1)	{
			c=vec4(inferno(r),a);
		}
		else if (palette==2)	{
			c=vec4(turbo(r),a);
		}
		else {
			c=vec4(viridis(r),a);
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
	outputcol.rgb=source.rgb*overallalpha + dest.rgb * (1.0-overallalpha);

	return outputcol;
}

void main() {
	vec2 uv=vec2(1,-1)*gl_FragCoord.xy/resolution.xy;
	uv*=degreespan/vec2(360.0,180.0); //convert the NSWE locations to between 0 and 1
	uv+=vec2(nswe.z+180.0, nswe.y-90.0)/vec2(360.0,-180.0); //shift them so aligned right

	vec4 wt=texture(worldTexture, uv);

	for (int i=0;i<atlascount;i++)	{
		if ((gl_FragCoord.y <atlasnswe[i].x) && (gl_FragCoord.y >atlasnswe[i].y) &&  (gl_FragCoord.x > atlasnswe[i].z) && (gl_FragCoord.x < atlasnswe[i].w))	{
			float squarefade=max(abs((gl_FragCoord.x - atlasnswe[i].z)/(atlasnswe[i].w-atlasnswe[i].z)-0.5) , abs((gl_FragCoord.y - atlasnswe[i].x)/(atlasnswe[i].y-atlasnswe[i].x)-0.5))*2.0;
			squarefade=1.0-smoothstep(0.8,1.0,squarefade);
			wt=mix(wt, texture(highresTexture,vec2(gl_FragCoord.xy/resolution)*atlasmult[i]+atlasadd[i]), squarefade);

		}
	}


	
	float heatvalue=texelFetch(heatmapTexture,ivec2(gl_FragCoord.xy),0).r;

	vec4 ht=FloatToColour(heatvalue, maxheatmapvalue);
	
	vec4 t;	//mix of world and heatmap
	t=AlphaOnOpaqueMix(wt.rgb,ht,1.0);
	
	frag_colour = t;
}