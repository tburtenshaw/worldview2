#version 330
out vec4 FragColor;

//uniform vec2 resolution;
uniform sampler2D texturetoreduce;
uniform int squaresize;



void main()
{ 
    vec4 accumulatedcolor=vec4(0.0);
    //ivec2 pointtosample = ivec2(floor(gl_FragCoord.xy*float(squaresize)));
    ivec2 pointtosample = ivec2(floor(gl_FragCoord.xy)*squaresize);

    for (int y=0;y<squaresize;y++)  {
        for (int x=0;x<squaresize;x++)  {
            accumulatedcolor = max(accumulatedcolor, texelFetchOffset(texturetoreduce, pointtosample, 0, ivec2(x,y)));

            //accumulatedcolor = max(accumulatedcolor, texture(texturetoreduce, vec2(pointtosample+ivec2(x,y))/vec2(1200.0,800.0), 0)); //tried this

        }
    }

    //accumulatedcolor=texture(texturetoreduce,gl_FragCoord.xy/vec2(1200.0,800.0),0);
    
    //accumulatedcolor=vec4(0.0);
    FragColor=accumulatedcolor+float(pointtosample*0);
}