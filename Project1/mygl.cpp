#include "mygl.h";
#include <glad/glad.h>
#include <GLFW/glfw3.h>


void GLRenderLayer::SetupShaders()
{
	printf("Shader not done yet");
}

void GLRenderLayer::GenerateBackgroundSquareVertices()	//this creates triangle mesh, gens vao/vbo for -1,-1, to 1,1 square
{
	static const GLfloat g_vertex_buffer_data[] = {
		-1.0f, -1.0f,
		 1.0f, -1.0f,
		-1.0f, 1.0f,
		 1.0f, 1.0f
		};

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(0);
}

void MapPointsInfo::SetupShaders()
{
	shader.LoadShaderFromFile("pointsVS.glsl", GL_VERTEX_SHADER);
	shader.LoadShaderFromFile("pointsGS.glsl", GL_GEOMETRY_SHADER);
	shader.LoadShaderFromFile("pointsFS.glsl", GL_FRAGMENT_SHADER);
	shader.CreateProgram();

	//load the uniform locations by names into integers
	shader.LoadUniformLocation(&uniformNswe, "nswe");
	shader.LoadUniformLocation(&uniformResolution, "resolution");
	shader.LoadUniformLocation(&uniformPointRadius, "pointradius");
	shader.LoadUniformLocation(&uniformPointAlpha, "alpha");
	shader.LoadUniformLocation(&uniformSeconds, "seconds");
	shader.LoadUniformLocation(&uniformCycleSeconds, "cycleSeconds");

	//Which times to show
	shader.LoadUniformLocation(&uniformEarliestTimeToShow, "earliesttimetoshow");
	shader.LoadUniformLocation(&uniformLatestTimeToShow, "latesttimetoshow");

	//A highlight is used to give an indication of travel speed, travels through as the peak of a sine wave
	shader.LoadUniformLocation(&uniformShowHighlights, "showhighlights");
	shader.LoadUniformLocation(&uniformSecondsBetweenHighlights, "secondsbetweenhighlights");
	shader.LoadUniformLocation(&uniformTravelTimeBetweenHighlights, "traveltimebetweenhighlights");

	shader.LoadUniformLocation(&uniformPalette, "palette");
	shader.LoadUniformLocation(&uniformColourBy, "colourby");
}

void MapRegionsInfo::SetupShaders()
{
	shader.LoadShaderFromFile("regionsVS.glsl", GL_VERTEX_SHADER);
	shader.LoadShaderFromFile("regionsFS.glsl", GL_FRAGMENT_SHADER);
	shader.LoadShaderFromFile("regionsGS.glsl", GL_GEOMETRY_SHADER);
	shader.CreateProgram();
}

void MapPathInfo::SetupShaders()
{
	shader.LoadShaderFromFile("mappathsVS.glsl", GL_VERTEX_SHADER);
	shader.LoadShaderFromFile("mappathsFS.glsl", GL_FRAGMENT_SHADER);
	shader.LoadShaderFromFile("mappathsGS.glsl", GL_GEOMETRY_SHADER);
	shader.CreateProgram();
}

void BackgroundInfo::SetupShaders()
{
	//Create the shader program
	shader.LoadShaderFromFile("backgroundVS.glsl", GL_VERTEX_SHADER);
	shader.LoadShaderFromFile("backgroundFS.glsl", GL_FRAGMENT_SHADER);
	shader.CreateProgram();

	//unsigned int worldTextureLocation, heatmapTextureLocation;
	worldTextureLocation = glGetUniformLocation(shader.program, "worldTexture");
	highresTextureLocation = glGetUniformLocation(shader.program, "highresTexture");
	heatmapTextureLocation = glGetUniformLocation(shader.program, "heatmapTexture");

	// Then bind the uniform samplers to texture units:
	shader.UseMe();
	glUniform1i(worldTextureLocation, 0);
	glUniform1i(highresTextureLocation, 1);
	glUniform1i(heatmapTextureLocation, 2);
}
