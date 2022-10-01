 #version 330

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

uniform vec2 resolution;
//uniform float pointradius;

uniform uint earliesttimetoshow;
uniform uint latesttimetoshow;
uniform uint minimumaccuracy;

in uint ts[];
in uint acc[];
out float timespent;    //the number of seconds at the sample, which is used as a "brightness value"
out vec2 centre;
out float pointradius;

void main()
{
    
    //get rid of dates outside of specified range
    if (ts[0] < earliesttimetoshow)  {
        EndPrimitive();
        return;
    }
    if (ts[0] >latesttimetoshow)  {
        EndPrimitive();
        return;
    }
    if (acc[0]>minimumaccuracy) {   //i've called in min, but it's really the max error allowed.
        EndPrimitive();
        return;
    }


    pointradius=1.0;

    vec2 p=vec2((pointradius+0.0)*2.0)/resolution.xy;   

    //constrain to inside the viewbox
    if ((gl_in[0].gl_Position.x+p.x<-1.0)||(gl_in[0].gl_Position.x-p.x>1.0)||(gl_in[0].gl_Position.y-p.y>1.0)||(gl_in[0].gl_Position.y+p.y<-1.0))   {
        EndPrimitive();
        return;
    }


    timespent=1.0;
    centre=gl_in[0].gl_Position.xy;

    gl_Position = gl_in[0].gl_Position + vec4(-p.x, -p.y, 0.0, 0.0);
	EmitVertex();

    gl_Position = gl_in[0].gl_Position + vec4(p.x, -p.y, 0.0, 0.0);
	EmitVertex();

    gl_Position = gl_in[0].gl_Position + vec4(-p.x, p.y, 0.0, 0.0);
	EmitVertex();

    gl_Position = gl_in[0].gl_Position + vec4(p.x, p.y, 0.0, 0.0);
	EmitVertex();



    EndPrimitive();

}