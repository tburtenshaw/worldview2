#version 330

uniform sampler2D worldTexture;
uniform sampler2D highresTexture;
uniform sampler2D heatmapTexture;

uniform vec2 resolution;
uniform vec4 nswe;	//the view
uniform vec4 highresnswe;
uniform vec2 highresscale;

//uniform vec4 heatmapnswe;	//where the heatmap covers

uniform float maxheatmapvalue;
uniform int palette;

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

vec4 FloatToColour(float vraw, float mraw)	{
	
	if (vraw>2)	{
		vec4 c;
		float r=log(vraw)/log(mraw); //value/max
		float a=smoothstep(0.0,0.2,r);
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
	float width = (nswe.w-nswe.z); 
	float height = (nswe.x-nswe.y);

	vec2 uv=vec2(1,-1)*gl_FragCoord.xy/resolution.xy;
	uv*=vec2(width,height)/vec2(360,180); //convert the NSWE locations to between 0 and 1
	uv+=vec2(nswe.z+180,nswe.y-90)/vec2(360,-180); //shift them so aligned right

	vec4 wt=texture(worldTexture, uv);

	
	//high res image
	vec2 highresuv;
	float highreswidth, highresheight;
	highreswidth = (highresnswe.w-highresnswe.z);
	highresheight = (highresnswe.x-highresnswe.y);


	highresuv.y=uv.y/(highresheight/180.0);
	highresuv.y+=(highresnswe.x-90.0)/highresheight;

	
	highresuv.x=(gl_FragCoord.x/resolution.x*(nswe.w-nswe.z) + (nswe.z-highresnswe.z))/(highresnswe.w-highresnswe.z);


	if ((highresuv.x>0.0)&&(highresuv.y>0.0)&&(highresuv.x<1.0)&&(highresuv.y<1.0))	{
		//wt=texture(highresTexture, highresuv*highresscale);
	
		vec2 uvn=abs(highresuv-0.5)*2.0;
		float maxDist  = max(abs(uvn.x), abs(uvn.y));
		float square=1.0-smoothstep(0.9,1.0,maxDist);

		wt = mix(wt, texture(highresTexture, highresuv*highresscale),square);
	}
	
	
	
	//find the UV of the heatmap, then add it.
	vec2 heatmapuv;
	float heatmapwidth, heatmapheight;
	heatmapwidth = (nswe.w-nswe.z);
	heatmapheight = (nswe.x-nswe.y);

	heatmapuv.x=(width* gl_FragCoord.x/resolution.x + nswe.z-nswe.z)/heatmapwidth;

	heatmapuv.y=-(nswe.x - gl_FragCoord.y/resolution.y*height - nswe.y)/heatmapheight;

	float heatvalue;
	heatvalue=texture(heatmapTexture, heatmapuv).r;

	vec4 ht=FloatToColour(heatvalue, maxheatmapvalue);
	
	vec4 t;	//mix of world and heatmap
	t=AlphaOnOpaqueMix(wt.rgb,ht,1.0);
	
	frag_colour = t;
}