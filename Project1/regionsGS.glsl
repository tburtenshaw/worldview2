 #version 330

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;


in vec2 bottomright[];
in vec3 vcolour[];


out vec3 gcolour;

void main()
{
    
   gcolour=vcolour[0];
  //gcolour=vec3(1.0,1.0,0.4);

    gl_Position = vec4(gl_in[0].gl_Position.x, gl_in[0].gl_Position.y, 0.1, 1.0);
	EmitVertex();

//gcolour=vec3(0.1,0.01,0.09);

        gl_Position = vec4(bottomright[0].x, gl_in[0].gl_Position.y, 0.2, 1.0);
	EmitVertex();

    gl_Position = vec4(gl_in[0].gl_Position.x,bottomright[0].y, 0.3, 1.0);
	EmitVertex();

//gcolour=vec3(0.3,0.8,0.9);


        gl_Position = vec4(bottomright[0].x, bottomright[0].y, 0.4, 1.0);
	EmitVertex();


//gcolour=vec3(0.9,0.3,0.4);



    EndPrimitive();
}