#version 330

uniform sampler2D worldTexture;
uniform sampler2D highresTexture;
uniform sampler2D heatmapTexture;

uniform vec2 resolution;
uniform vec4 nswe;	//the view
uniform vec4 highresnswe;
uniform vec4 heatmapnswe;	//where the heatmap covers

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

vec3 viridis_quintic( float x )
{
	vec4 x1 = vec4( 1.0, x, x * x, x * x * x ); // 1 x x2 x3
	vec4 x2 = x1 * x1.w * x; // x4 x5 x6 x7
	return vec3(
		dot( x1.xyzw, vec4( +0.280268003, -0.143510503, +2.225793877, -14.815088879 ) ) + dot( x2.xy, vec2( +25.212752309, -11.772589584 ) ),
		dot( x1.xyzw, vec4( -0.002117546, +1.617109353, -1.909305070, +2.701152864 ) ) + dot( x2.xy, vec2( -1.685288385, +0.178738871 ) ),
		dot( x1.xyzw, vec4( +0.300805501, +2.614650302, -12.019139090, +28.933559110 ) ) + dot( x2.xy, vec2( -33.491294770, +13.762053843 ) ) );
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

	vec4 wt=texture(worldTexture, uv);
	
	
	//high res image
	vec2 highresuv;
	float highreswidth, highresheight;
	highreswidth = (highresnswe.w-highresnswe.z);
	highresheight = (highresnswe.x-highresnswe.y);

	highresuv.x=uv.x/(highreswidth/360);
	highresuv.x-=(highresnswe.z+180)/highreswidth;

	highresuv.y=uv.y/(highresheight/180);
	highresuv.y+=(highresnswe.x-90)/highresheight;

	if ((highresuv.x>0)&&(highresuv.y>0)&&(highresuv.x<1)&&(highresuv.y<1))	{
		wt=texture(highresTexture, highresuv);
	}
	
	
	
	//find the UV of the heatmap, then add it.
	vec2 heatmapuv;
	float heatmapwidth, heatmapheight;
	heatmapwidth = (heatmapnswe.w-heatmapnswe.z);
	heatmapheight = (heatmapnswe.x-heatmapnswe.y);

	heatmapuv.x=uv.x/(heatmapwidth/360);
	heatmapuv.x-=(heatmapnswe.z+180)/heatmapwidth;

	heatmapuv.y=uv.y/(heatmapheight/180);
	heatmapuv.y+=(heatmapnswe.x-90)/heatmapheight;

	float heatvalue;
	heatvalue=texture(heatmapTexture, heatmapuv).r;
	vec4 ht=FloatToColour(heatvalue, maxheatmapvalue);
	
	vec4 t;	//mix of world and heatmap
	t=AlphaOnOpaqueMix(wt.rgb,ht,1.0);

	vec2 sunloc;
	sunloc=vec2(0.5,-0.7);

	float specular =0.0;
	if (t.b>t.g+t.r)	{
		specular= pow(clamp(1.0-distance(uv,sunloc),0.0,1.0),30);
		}
	
	frag_colour = t+vec4(specular);
}