 #version 330

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

uniform vec2 resolution;
//uniform float pointradius;

uniform uint earliesttimetoshow;
uniform uint latesttimetoshow;
uniform uint minimumaccuracy;

in uint ts[];
in uint acc[];
out float timespent;    //the number of seconds at the sample, which is used as a "brightness value"

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


    float pointradius=1.5;

    vec2 p=vec2((pointradius+0.0)*2.0)/resolution.xy;   

    //constrain to inside the viewbox
    if ((gl_in[0].gl_Position.x+p.x<-1.0)||(gl_in[0].gl_Position.x-p.x>1.0)||(gl_in[0].gl_Position.y-p.y>1.0)||(gl_in[0].gl_Position.y+p.y<-1.0))   {
        EndPrimitive();
        return;
    }


    //we covert to uint with (arbitrarily) 1262304000 (jan 2010) taken off, which reduces the magnitude of the uint so its more precise when coverted to a float
    //and helps prevent overflow
    uint temptimestamp0 = ts[0]-uint(1262304000);
    uint temptimestamp1 = ts[1]-uint(1262304000);

    int timediff = int(temptimestamp1-temptimestamp0);

    timespent=max(0.0,float(timediff));   //assumes the time spent in a place, is the time before the next place
    //as we've corrected for DST, this discards time that's negative.
    


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