 #version 400 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

uniform vec2 resolution;
uniform float pointradius;

in vec3 vcolour[];
out vec3 gcolour;
out vec2 centre;

void main()
{
    
    float p=(pointradius+1)/resolution.x;   //the square should be a pixel bigger to avoid artefact

    //constrain to inside the viewbox
    if ((gl_in[0].gl_Position.x+p<-1.0)||(gl_in[0].gl_Position.x-p>1.0)||(gl_in[0].gl_Position.y-p>1.0)||(gl_in[0].gl_Position.y+p<-1.0))   {
        EndPrimitive();
        return;
    }


    gcolour=vcolour[0];
    centre=gl_in[0].gl_Position.xy;

    gl_Position = gl_in[0].gl_Position + vec4(-p, -p, 0.0, 0.0);
	EmitVertex();

    gl_Position = gl_in[0].gl_Position + vec4(p, -p, 0.0, 0.0);
	EmitVertex();

    gl_Position = gl_in[0].gl_Position + vec4(-p, p, 0.0, 0.0);
	EmitVertex();

    gl_Position = gl_in[0].gl_Position + vec4(p, p, 0.0, 0.0);
	EmitVertex();



    EndPrimitive();

}