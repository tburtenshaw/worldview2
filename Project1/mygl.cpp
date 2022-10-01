#include "header.h"
#include "mygl.h"
#include "nswe.h"
#include "regions.h"
#include "palettes.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <stb_image.h>

unsigned int GLRenderLayer::vboLocations = 0;

void GLRenderLayer::SetupSquareVertices()	//this creates triangle mesh, gens vao/vbo for -1,-1, to 1,1 square
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

void GLRenderLayer::UseLocationVBO()
{
	vbo = vboLocations;
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
}

void GLRenderLayer::CreateLocationVBO(std::vector<PathPlotLocation>& locs)
{
	glGenBuffers(1, &vboLocations);
	glBindBuffer(GL_ARRAY_BUFFER, vboLocations);
	glBufferData(GL_ARRAY_BUFFER, locs.size() * sizeof(PathPlotLocation), &locs.front(), GL_STATIC_DRAW);
}

void PointsLayer::SetupShaders()
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

void PointsLayer::SetupVertices()
{
	//glGenBuffers(1, &vbo);
	//glBindBuffer(GL_ARRAY_BUFFER, vbo);
	//glBufferData(GL_ARRAY_BUFFER, locs.size() * sizeof(PathPlotLocation), &locs.front(), GL_STATIC_DRAW);
	UseLocationVBO();

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	//lat,long
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(PathPlotLocation), (void*)offsetof(PathPlotLocation, longitude));
	glEnableVertexAttribArray(0);

	//rgba colour
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(PathPlotLocation), (void*)offsetof(PathPlotLocation, rgba));
	glEnableVertexAttribArray(1);

	//timestamp
	glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(PathPlotLocation), (void*)offsetof(PathPlotLocation, timestamp));
	glEnableVertexAttribArray(2);
}

void PointsLayer::Draw(std::vector<PathPlotLocation>& locs, float width, float height, NSWE* nswe, GlobalOptions* options)
{
	//update uniform shader variables
	shader.UseMe();
	shader.SetUniform(uniformNswe, nswe);
	shader.SetUniform(uniformResolution, width, height);
	shader.SetUniform(uniformPointRadius, options->pointdiameter / 2.0f);
	shader.SetUniform(uniformPointAlpha, options->pointalpha);
	shader.SetUniform(uniformSeconds, options->seconds);
	shader.SetUniform(uniformCycleSeconds, options->cycleSeconds);

	shader.SetUniform(uniformEarliestTimeToShow, options->earliestTimeToShow);
	shader.SetUniform(uniformLatestTimeToShow, options->latestTimeToShow);

	shader.SetUniform(uniformShowHighlights, options->showHighlights);
	shader.SetUniform(uniformSecondsBetweenHighlights, options->secondsbetweenhighlights);
	shader.SetUniform(uniformTravelTimeBetweenHighlights, options->minutestravelbetweenhighlights * 60.0f);

	shader.SetUniform(uniformColourBy, options->colourby);

	switch (options->colourby) {
	case 1:
		Palette_Handler::FillShaderPalette(palette, 24, options->indexPaletteHour);
		shader.SetUniform(uniformPalette, 24, &palette[0][0]);
		break;
	case 4:
		Palette_Handler::FillShaderPalette(palette, 24, options->indexPaletteYear);
		shader.SetUniform(uniformPalette, 24, &palette[0][0]);
		break;
	default:
		Palette_Handler::FillShaderPalette(palette, 24, options->indexPaletteWeekday);
		shader.SetUniform(uniformPalette, 24, &palette[0][0]);
		break;
	}

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindVertexArray(vao);
	glDrawArrays(GL_POINTS, 0, locs.size());
}



void RegionsLayer::SetupShaders()
{
	shader.LoadShaderFromFile("regionsVS.glsl", GL_VERTEX_SHADER);
	shader.LoadShaderFromFile("regionsFS.glsl", GL_FRAGMENT_SHADER);
	shader.LoadShaderFromFile("regionsGS.glsl", GL_GEOMETRY_SHADER);
	shader.CreateProgram();
}

void RegionsLayer::SetupVertices()
{
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glBufferData(GL_ARRAY_BUFFER, displayRegions.size() * sizeof(DisplayRegion), &displayRegions.front(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(DisplayRegion), 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(DisplayRegion), (void*)offsetof(DisplayRegion, east));
	glEnableVertexAttribArray(1);


	glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(DisplayRegion), (void*)offsetof(DisplayRegion, colour));
	glEnableVertexAttribArray(2);
}

void RegionsLayer::UpdateFromRegionsData(std::vector<Region*>& dataRegions)
{
	int sizeOfRegionVector = dataRegions.size();

	//We'll just do a global redraw if any region has changed
	bool needsRedraw = false;
	for (std::size_t i = 0; (i < sizeOfRegionVector) && (!needsRedraw); i++) {
		if (dataRegions[i]->needsRedraw) {
			needsRedraw = true;
			dataRegions[i]->needsRedraw = false;
		}
	}

	if (!needsRedraw) {
		return;
	}

	if (displayRegions.size() != sizeOfRegionVector - 1) {
		displayRegions.resize(sizeOfRegionVector - 1);
		//printf("changing displayregions size to: %i\n",sizeOfRegionVector-1);
	}

	for (int r = 1; r < sizeOfRegionVector; r++) {
		//printf("r:%i of %i.\t", r, sizeOfRegionVector);

		displayRegions[r - 1].west = dataRegions[r]->nswe.west;
		displayRegions[r - 1].north = dataRegions[r]->nswe.north;
		displayRegions[r - 1].east = dataRegions[r]->nswe.east;
		displayRegions[r - 1].south = dataRegions[r]->nswe.south;

		displayRegions[r - 1].colour = dataRegions[r]->colour;


		//printf("%f %f %f %f\n", mapRegionsInfo->displayRegions[r - 1].f[0], mapRegionsInfo->displayRegions[r - 1].f[1], mapRegionsInfo->displayRegions[r - 1].f[2], mapRegionsInfo->displayRegions[r - 1].f[3]);
		//printf("%i - %i = %i\n", &mapRegionsInfo->displayRegions[1].f, &mapRegionsInfo->displayRegions[0].f, ((long)&mapRegionsInfo->displayRegions[1].f) - ((long)&mapRegionsInfo->displayRegions[0].f));
	}

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, displayRegions.size() * sizeof(DisplayRegion), &displayRegions.front(), GL_STATIC_DRAW);

}

void RegionsLayer::Draw(float width, float height, NSWE* nswe)
{
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	shader.UseMe();
	shader.SetUniformFromFloats("resolution", width, height);
	shader.SetUniformFromNSWE("nswe", nswe);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glBindVertexArray(vao);
	glDrawArrays(GL_POINTS, 0, displayRegions.size());//needs to be the number of vertices (not lines)
	glBindVertexArray(0);
	DisplayIfGLError("After DrawRegions.", false);

}

void PathLayer::SetupShaders()
{
	shader.LoadShaderFromFile("mappathsVS.glsl", GL_VERTEX_SHADER);
	shader.LoadShaderFromFile("mappathsFS.glsl", GL_FRAGMENT_SHADER);
	shader.LoadShaderFromFile("mappathsGS.glsl", GL_GEOMETRY_SHADER);
	shader.CreateProgram();
}

void PathLayer::SetupVertices()
{
	//glGenBuffers(1, &vbo);
	//glBindBuffer(GL_ARRAY_BUFFER, vbo);
	//glBufferData(GL_ARRAY_BUFFER, locs.size() * sizeof(PathPlotLocation), &locs.front(), GL_STATIC_DRAW);

	UseLocationVBO();

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	//lat,long
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(PathPlotLocation), (void*)offsetof(PathPlotLocation, longitude));
	glEnableVertexAttribArray(0);

	//rgba colour
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(PathPlotLocation), (void*)offsetof(PathPlotLocation, rgba));
	glEnableVertexAttribArray(1);

	//timestamp (maybe replace the whole array with a smaller copy, and let this be a colour)
	//glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, sizeof(PathPlotLocation), (void*)offsetof(PathPlotLocation, timestamp));
	//glEnableVertexAttribArray(1);

	//detail level
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(PathPlotLocation), (void*)offsetof(PathPlotLocation, detaillevel));
	glEnableVertexAttribArray(2);
}

void PathLayer::BindBuffer()
{
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
}

void PathLayer::Draw(std::vector<PathPlotLocation>& locs, float width, float height, NSWE* nswe, float linewidth, float seconds, float cycleSeconds)
{
	//update uniform shader variables
	shader.UseMe();
	shader.SetUniformFromNSWE("nswe", nswe);
	shader.SetUniformFromFloats("seconds", seconds * 20.0f);
	shader.SetUniformFromFloats("resolution", width, height);
	shader.SetUniformFromFloats("linewidth", linewidth);
	shader.SetUniformFromFloats("cycle", cycleSeconds);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindVertexArray(vao);
	glDrawArrays(GL_LINE_STRIP, 0, locs.size());

}

void BackgroundLayer::LoadBackgroundImageToTexture()
{
	int width, height, nrChannels;
	unsigned char* data = stbi_load("world.200409.3x4096x2048.png", &width, &height, &nrChannels, 0);

	if (!data) {	//hack to try root until I get filenames better
		stbi_image_free(data);
		data = stbi_load("d:/world.200409.3x4096x2048.png", &width, &height, &nrChannels, 0);
	}
	if (!data) {
		printf("\nCan't load background\n");
		return;
	}

	glGenTextures(1, &worldTexture);
	glBindTexture(GL_TEXTURE_2D, worldTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	stbi_image_free(data);
}

void BackgroundLayer::MakeHighresImageTexture()
{
	GLint maxtexturesize;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxtexturesize);
	printf("Max texture size: %i\n", maxtexturesize);
	
	glGenTextures(1, &highresTexture);
	glBindTexture(GL_TEXTURE_2D, highresTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 8192, 8192, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glGenerateMipmap(GL_TEXTURE_2D);
}


void BackgroundLayer::SetupShaders()
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

void BackgroundLayer::SetupTextures()
{
	LoadBackgroundImageToTexture();
	MakeHighresImageTexture();
}

void BackgroundLayer::Draw(RectDimension windowsize, const NSWE &viewNSWE, const GlobalOptions &options)
{

	NSWE* highresnswe;

	highres.DecideBestTex(windowsize, viewNSWE);
	highresnswe = highres.GetBestNSWE(highresTexture);


	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	shader.UseMe();
	//shader.SetUniformFromFloats("seconds", seconds);
	shader.SetUniformFromFloats("resolution", windowsize.width, windowsize.height);
	shader.SetUniformFromFloats("nswe", viewNSWE.north, viewNSWE.south, viewNSWE.west, viewNSWE.east);
	shader.SetUniformFromNSWE("highresnswe", highresnswe);
	shader.SetUniformFromFloats("highresscale", (float)highres.width / 8192.0f, (float)highres.height / 8192.0f); //as we're just loading the
	//shader.SetUniformFromFloats("heatmapnswe", viewNSWE.north, viewNSWE.south, viewNSWE.west, viewNSWE.east);
	

	shader.SetUniformFromFloats("maxheatmapvalue", options.heatmapmaxvalue);// heatmap.maxPixel);
	shader.SetUniformFromInts("palette", options.palette);

	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, worldTexture);

	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, highresTexture);

	glActiveTexture(GL_TEXTURE0 + 2);
	glBindTexture(GL_TEXTURE_2D, NEWheatmapTexture);

	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindVertexArray(0);
}



void FrameBufferObjectInfo::BindToDrawTo()
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

void FrameBufferObjectInfo::SetupFrameBufferObject(int width, int height) //makes FBO and its VAO, VBO, vertices and shaders
{
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glGenTextures(1, &fboTexture);
	glBindTexture(GL_TEXTURE_2D, fboTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTexture, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		printf("Error: Framebuffer not completed\n");
	else printf("Fbo generation finished\n");

	//this sets up the vertices and shaders for the FBO
	SetupSquareVertices();
	shader.LoadShaderFromFile("fboVS.glsl", GL_VERTEX_SHADER);
	shader.LoadShaderFromFile("fboFS.glsl", GL_FRAGMENT_SHADER);
	shader.CreateProgram();
	//printf("After create program. glGetError %i\n", glGetError());

	shader.UseMe();
	int uniloc = glGetUniformLocation(shader.program, "screenTexture");
	//printf("FBO: After getuniloc. glGetError %i\n", glGetError());
	glUniform1i(uniloc, 4);
	//printf("FBO: After uniform set. glGetError %i\n", glGetError());

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	printf("After FBO all done. glGetError %i\n", glGetError());
}

void FrameBufferObjectInfo::Draw(float width, float height)
{
	shader.UseMe();
	shader.SetUniformFromFloats("resolution", width, height);

	glActiveTexture(GL_TEXTURE0 + 4);
	glBindTexture(GL_TEXTURE_2D, fboTexture);
	glDisable(GL_DEPTH_TEST);
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void FrameBufferObjectInfo::UpdateSize(int width, int height)
{
	glBindTexture(GL_TEXTURE_2D, fboTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

}

void HeatmapLayer::Setup(int width, int height)
{
	//Create the FBO which is drawn to
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	//create the texture, which is a single channel float.
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

	//Unbind everything
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//Set vao and vbo to zero.
	vao = 0;
	vbo = 0;

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		printf("ERROR::FRAMEBUFFER:: Framebuffer is not complete!");
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	SetupShaders();
}

void HeatmapLayer::SetupVertices()
{
	//glGenBuffers(1, &vbo);
	//glBindBuffer(GL_ARRAY_BUFFER, vbo);
	//glBufferData(GL_ARRAY_BUFFER, locs.size() * sizeof(PathPlotLocation), &locs.front(), GL_STATIC_DRAW);

	UseLocationVBO();

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	//lat,long
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(PathPlotLocation), (void*)offsetof(PathPlotLocation, longitude));
	glEnableVertexAttribArray(0);

	//timestamp
	glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, sizeof(PathPlotLocation), (void*)offsetof(PathPlotLocation, timestamp));
	glEnableVertexAttribArray(1);

	//accuracy
	glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(PathPlotLocation), (void*)offsetof(PathPlotLocation, accuracy));
	glEnableVertexAttribArray(2);

}

void HeatmapLayer::Draw(std::vector<PathPlotLocation>& locs, float width, float height, NSWE* nswe, GlobalOptions* options)
{
	shader.UseMe();
	
	shader.SetUniform(uniformNswe, nswe);
	shader.SetUniform(uniformResolution, width, height);
	shader.SetUniform(uniformEarliestTimeToShow, options->earliestTimeToShow);
	shader.SetUniform(uniformLatestTimeToShow, options->latestTimeToShow);
	shader.SetUniform(uniformMinimumAccuracy, (unsigned long)options->minimumaccuracy);


	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	//change the blending options

	glBlendFunc(GL_ONE, GL_ONE);
	glBlendEquation(GL_FUNC_ADD);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	//glActiveTexture(GL_TEXTURE0 + 2);
	glBindTexture(GL_TEXTURE_2D, texture);
	glDisable(GL_DEPTH_TEST);
	glBindVertexArray(vao);

	glDrawArrays(GL_POINTS, 0, locs.size());

	//now back to normal
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glBlendEquation(GL_FUNC_ADD);

}

void HeatmapLayer::UpdateSize(int width, int height)
{
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, NULL);
	
}

void HeatmapLayer::SetupShaders()
{
	shader.LoadShaderFromFile("heatmapVS.glsl", GL_VERTEX_SHADER);
	shader.LoadShaderFromFile("heatmapFS.glsl", GL_FRAGMENT_SHADER);
	shader.LoadShaderFromFile("heatmapGS.glsl", GL_GEOMETRY_SHADER);
	shader.CreateProgram();

	//load the uniform locations by names into integers
	shader.LoadUniformLocation(&uniformNswe, "nswe");
	shader.LoadUniformLocation(&uniformResolution, "resolution");
	shader.LoadUniformLocation(&uniformEarliestTimeToShow, "earliesttimetoshow");
	shader.LoadUniformLocation(&uniformLatestTimeToShow, "latesttimetoshow");
	shader.LoadUniformLocation(&uniformMinimumAccuracy, "minimumaccuracy");

}
