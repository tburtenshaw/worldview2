#include "header.h"
#include "mygl.h"
#include "options.h"
#include "nswe.h"
#include "regions.h"
#include "palettes.h"
#include "atlas.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <array>
#include <stb_image.h>
#include <iostream>


// Defined in winconsole.cpp
extern GlobalOptions globalOptions;

//Static members need setting
unsigned int GLRenderLayer::vboLocations = 0;
unsigned int GLRenderLayer::vaoSquare = 0;
unsigned int GLRenderLayer::vboSquare = 0;




void GLRenderLayer::SetupSquareVertices()	//this creates triangle mesh, gens vao/vbo for -1,-1, to 1,1 square
{
	if (vaoSquare)	return;	//if not already made

	DisplayIfGLError("before   GLRenderLayer::SetupSquareVertices()", false);
	static const GLfloat g_vertex_buffer_data[] = {
		-1.0f, -1.0f,
		 1.0f, -1.0f,
		-1.0f, 1.0f,
		 1.0f, 1.0f
		};

	glGenVertexArrays(1, &vaoSquare);
	glBindVertexArray(vaoSquare);

	glGenBuffers(1, &vboSquare);
	glBindBuffer(GL_ARRAY_BUFFER, vboSquare);

	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(0);

	DisplayIfGLError("after   GLRenderLayer::SetupSquareVertices()", false);
}

void GLRenderLayer::UseLocationVBO()
{
	vbo = vboLocations;
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	DisplayIfGLError("after UseLocationVBO()", false);
}

void GLRenderLayer::CreateLocationVBO(LODInfo& lodInfo)
{
	//Generate the buffer
	glGenBuffers(1, &vboLocations);
	glBindBuffer(GL_ARRAY_BUFFER, vboLocations);
	//printf("Size: %i.  front:%i\n", lodInfo.pathPlotLocations.size(), lodInfo.pathPlotLocations.front());
	glBufferData(GL_ARRAY_BUFFER, lodInfo.pathPlotLocations.size() * sizeof(PathPlotLocation), &lodInfo.pathPlotLocations.front(), GL_STATIC_DRAW);

	
	
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
	shader.LoadUniformLocation(&uniformDegreeSpan, "degreespan");
	shader.LoadUniformLocation(&uniformDegreeMidpoint, "degreemidpoint");

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
	//glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(PathPlotLocation), (void*)offsetof(PathPlotLocation, rgba));
	//glEnableVertexAttribArray(1);

	//timestamp
	glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(PathPlotLocation), (void*)offsetof(PathPlotLocation, timestamp));
	glEnableVertexAttribArray(2);
}

void PointsLayer::Draw(LODInfo& lodInfo, int lod, float width, float height, NSWE* nswe)
{
	//update uniform shader variables
	shader.UseMe();
	shader.SetUniform(uniformNswe, nswe);
	shader.SetUniform(uniformResolution, width, height);
	shader.SetUniform(uniformDegreeSpan, nswe->width(), nswe->height());
	shader.SetUniform(uniformDegreeMidpoint, (nswe->west + nswe->east) / 2.0, (nswe->north + nswe->south) / 2.0);


	shader.SetUniform(uniformPointRadius, globalOptions.pointdiameter / 2.0f);
	shader.SetUniform(uniformPointAlpha, globalOptions.pointalpha);
	shader.SetUniform(uniformSeconds, globalOptions.seconds);
	shader.SetUniform(uniformCycleSeconds, globalOptions.cycleSeconds);

	shader.SetUniform(uniformEarliestTimeToShow, globalOptions.earliestTimeToShow);
	shader.SetUniform(uniformLatestTimeToShow, globalOptions.latestTimeToShow);

	shader.SetUniform(uniformShowHighlights, globalOptions.showHighlights);
	shader.SetUniform(uniformSecondsBetweenHighlights, globalOptions.secondsbetweenhighlights);
	shader.SetUniform(uniformTravelTimeBetweenHighlights, globalOptions.minutestravelbetweenhighlights * 60.0f);

	shader.SetUniform(uniformColourBy, globalOptions.colourby);

	switch (globalOptions.colourby) {
	case 1:
		Palette_Handler::FillShaderPalette(palette, 24, globalOptions.indexPaletteHour);
		shader.SetUniform(uniformPalette, 24, &palette[0][0]);
		break;
	case 4:
		Palette_Handler::FillShaderPalette(palette, 24, globalOptions.indexPaletteYear);
		shader.SetUniform(uniformPalette, 24, &palette[0][0]);
		break;
	default:
		Palette_Handler::FillShaderPalette(palette, 24, globalOptions.indexPaletteWeekday);
		shader.SetUniform(uniformPalette, 24, &palette[0][0]);
		break;
	}

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindVertexArray(vao);
	DisplayIfGLError("before dapts", false);


	//Use lookup table to skip some
	GLint first = 0;
	GLsizei count = 0;

	lodInfo.LookupFirstAndCount(globalOptions.earliestTimeToShow, globalOptions.latestTimeToShow, lod, &first, &count);
	//printf("first: %i, count: %i.\n", first, count);
	glDrawArrays(GL_POINTS, first, count);
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
	DisplayIfGLError("before regions", false);
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

	shader.UseMe();
	shader.LoadUniformLocation(&uniformNswe, "nswe");
	shader.LoadUniformLocation(&uniformResolution, "resolution");
	shader.LoadUniformLocation(&uniformDegreeSpan, "degreespan");
	shader.LoadUniformLocation(&uniformDegreeMidpoint, "degreemidpoint");
	shader.LoadUniformLocation(&uniformDPPHoriz, "dpphoriz");
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

	//timestamp (maybe replace the whole array with a smaller copy, and let this be a colour)
	glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, sizeof(PathPlotLocation), (void*)offsetof(PathPlotLocation, timestamp));
	glEnableVertexAttribArray(1);

	//detail level
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(PathPlotLocation), (void*)offsetof(PathPlotLocation, detaillevel));
	glEnableVertexAttribArray(2);
}

void PathLayer::Draw(LODInfo& lodInfo, int lod, float width, float height, NSWE* nswe, float linewidth, float seconds, float cycleSeconds)
{
	//update uniform shader variables
	shader.UseMe();

	shader.SetUniform(uniformNswe, nswe);
	shader.SetUniform(uniformResolution, width, height);
	shader.SetUniform(uniformDegreeSpan, nswe->width(), nswe->height());
	shader.SetUniform(uniformDegreeMidpoint, (nswe->west + nswe->east) / 2.0, (nswe->north + nswe->south) / 2.0);
	shader.SetUniform(uniformDPPHoriz, (float)nswe->width()/width);
	
	//shader.SetUniformFromNSWE("nswe", nswe);
	shader.SetUniformFromFloats("seconds", seconds * 20.0f);
	//shader.SetUniformFromFloats("resolution", width, height);
	shader.SetUniformFromFloats("linewidth", linewidth);
	shader.SetUniformFromFloats("cycle", cycleSeconds);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindVertexArray(vao);
	
	//Use lookup table to skip some
	GLint first = 0;
	GLsizei count = 0;
	//Path has its own optimisations, so should use LOD 0
	lodInfo.LookupFirstAndCount(globalOptions.earliestTimeToShow, globalOptions.latestTimeToShow, 0, &first, &count);
	
	glDrawArrays(GL_LINE_STRIP, first, count);

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

void BackgroundLayer::Setup()
{
	DisplayIfGLError("before  BackgroundLayer::Setup", false);
	SetupSquareVertices();
	vao = vaoSquare;
	vbo = vboSquare;

	atlas.Setup();
}

void BackgroundLayer::SetupShaders()
{
	DisplayIfGLError("before  BackgroundLayer::SetupShaders", false);
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
	DisplayIfGLError("after  BackgroundLayer::SetupShaders", false);

	shader.LoadUniformLocation(&uniformNswe, "nswe");
	shader.LoadUniformLocation(&uniformResolution, "resolution");
	shader.LoadUniformLocation(&uniformDegreeSpan, "degreespan");
	shader.LoadUniformLocation(&uniformMaxHeatmapValue, "maxheatmapvalue");
	shader.LoadUniformLocation(&uniformPaletteNumber, "palette");

	shader.LoadUniformLocation(&uniformAtlasCount, "atlascount");
	shader.LoadUniformLocation(&uniformAtlasNSWE, "atlasnswe");
	shader.LoadUniformLocation(&uniformAtlasMult, "atlasmult");
	shader.LoadUniformLocation(&uniformAtlasAdd, "atlasadd");


}

void BackgroundLayer::SetupTextures()
{
	LoadBackgroundImageToTexture();
}

void BackgroundLayer::Draw(MainViewport* vp)
{

	constexpr int maxAtlasDraws = 8;	//check doesn't exceed what is in the backgroundFS shader

	int numberOfAtlasDraws = 0;
	vec2f highresMult[maxAtlasDraws] = { 0.f };
	vec2f highresAdd[maxAtlasDraws] = { 0.f };
	vec4f highresNSWE[maxAtlasDraws] = { 0.0f };

	if (vp->DegreesPerPixel() < 360.0 / 4096.0) {
		atlas.OutputDrawOrderedUVListForUniform(vp, &numberOfAtlasDraws, (vec4f*)highresNSWE, (vec2f*)highresMult, (vec2f*)highresAdd, maxAtlasDraws);
	}


	DisplayIfGLError("before BackgroundLayer::Draw glBindBuffer(GL_ARRAY_BUFFER, vbo);", false);
	glBindBuffer(GL_ARRAY_BUFFER, vboSquare);

	shader.UseMe();
	//shader.SetUniformFromFloats("seconds", seconds);


	shader.SetUniform(uniformNswe, &vp->viewNSWE);
	shader.SetUniform(uniformResolution, vp->windowDimensions.width, vp->windowDimensions.height);
	shader.SetUniform(uniformDegreeSpan, vp->viewNSWE.width(), vp->viewNSWE.height());

	DisplayIfGLError("before new uniforms", false);
	shader.SetUniform(uniformAtlasCount, numberOfAtlasDraws);

	glUniform2fv(uniformAtlasMult, numberOfAtlasDraws, (float*)highresMult);
	glUniform2fv(uniformAtlasAdd, numberOfAtlasDraws, (float*)highresAdd);
	glUniform4fv(uniformAtlasNSWE, numberOfAtlasDraws, (float*)highresNSWE);
	DisplayIfGLError("after new uniforms", false);



	//std::cout << "draw:" << highresNSWEfloat[0]<< highresNSWEfloat[1]<< highresNSWEfloat[2] << highresNSWEfloat[3] << "\n";


	shader.SetUniform(uniformMaxHeatmapValue, globalOptions.heatmapmaxvalue);
	shader.SetUniform(uniformPaletteNumber, globalOptions.heatmapPaletteIndex);

	//std::cout << "wt:" << worldTexture << ", hrt:" << highresTexture << ", atlastex:" << atlas.getTexture() << ", hmT " << heatmapTexture << "\n";

	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, worldTexture);

	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, atlas.getTexture());

	glActiveTexture(GL_TEXTURE0 + 2);
	if (globalOptions.showHeatmap) {
		glBindTexture(GL_TEXTURE_2D, heatmapTexture);
	}
	else {
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	DisplayIfGLError("before bva background", false);
	glBindVertexArray(vaoSquare);
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
	vao = vaoSquare;
	vbo = vboSquare;

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
	glGenFramebuffers(1, &fboToDrawHeatmap);
	glBindFramebuffer(GL_FRAMEBUFFER, fboToDrawHeatmap);

	//create the texture, which is a single channel float.
	glGenTextures(1, &texture);

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);	//could use linear, as blur shader might benefit from sampling between two pixels
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); //but this not worth the speed
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);	//prevent wrapping of data (still not accurate at edges)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);


	//FBO for the first blur
	glGenFramebuffers(1, &fboBlur);
	glBindFramebuffer(GL_FRAMEBUFFER, fboBlur);
	//create the blur texture, which is a single channel float.
	glGenTextures(1, &blurTexture);
	glBindTexture(GL_TEXTURE_2D, blurTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blurTexture, 0);

	//FBOs for the maxval finder
	glGenFramebuffers(2, fboMaxVal);
	glGenTextures(2, maxvalTexture);

	glBindFramebuffer(GL_FRAMEBUFFER, fboMaxVal[0]);
	glBindTexture(GL_TEXTURE_2D, maxvalTexture[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, maxvalTexture[0], 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		printf("ERROR::FRAMEBUFFER:: Framebuffer is not complete!");
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	glBindFramebuffer(GL_FRAMEBUFFER, fboMaxVal[1]);
	glBindTexture(GL_TEXTURE_2D, maxvalTexture[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, maxvalTexture[1], 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		printf("ERROR::FRAMEBUFFER:: Framebuffer is not complete!");
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	//Unbind everything
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//Set vao and vbo to zero.
	vao = 0;
	vbo = 0;

	SetupShaders();
}

void HeatmapLayer::SetupVertices()
{

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

void HeatmapLayer::Draw(const LODInfo& lodInfo, int lod, const RectDimension windowSize, const NSWE& nswe)
{
	if (NeedsRedraw(nswe, windowSize) == false) return;
	
	shader.UseMe();
	
	float width = static_cast<float>(windowSize.width);
	float height = static_cast<float>(windowSize.height);

	shader.SetUniform(uniformNswe, &nswe);
	shader.SetUniform(uniformResolution, width, height);
	shader.SetUniform(uniformDegreeSpan, nswe.width(), nswe.height());
	shader.SetUniform(uniformDegreeMidpoint, (nswe.west+nswe.east) / 2.0, (nswe.north+nswe.south) / 2.0);


	shader.SetUniform(uniformEarliestTimeToShow, globalOptions.earliestTimeToShow);
	shader.SetUniform(uniformLatestTimeToShow, globalOptions.latestTimeToShow);
	shader.SetUniform(uniformMinimumAccuracy, (unsigned long)globalOptions.minimumaccuracy);


	glBindFramebuffer(GL_FRAMEBUFFER, fboToDrawHeatmap);
	
	//change the blending options
	glBlendFunc(GL_ONE, GL_ONE);
	glBlendEquation(GL_FUNC_ADD);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindVertexArray(vao);

	//Use lookup table to skip some
	GLint first = 0;
	GLsizei count = 0;

	lodInfo.LookupFirstAndCount(globalOptions.earliestTimeToShow, globalOptions.latestTimeToShow, lod, &first, &count);
	//printf("first: %i, count: %i.\n", first, count);
	DisplayIfGLError("before dahm", false);
	//glDrawArrays(GL_LINE_STRIP, lodInfo.lodStart[lod], lodInfo.lodLength[lod]);
	glDrawArrays(GL_LINE_STRIP, first, count);

	//glDrawArrays(GL_POINTS, 0, locationsCount );
	DisplayIfGLError("after dahm", false);

	//then the blur pass.
	GaussianBlur(globalOptions.gaussianblur, width, height);

	//now find the brigtest point
	globalOptions.heatmapmaxvalue = FindMaxValueWithReductionShader(width, height, 4);	//8 might be slightly faster

	//now back to normal
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	

	UpdateSettingsWhenDrawn(nswe,windowSize);

}

void HeatmapLayer::UpdateSize(int width, int height)
{
	//if window resizes, this should reset the texture size.
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, NULL);

	glBindTexture(GL_TEXTURE_2D, blurTexture);
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
	shader.LoadUniformLocation(&uniformDegreeSpan, "degreespan");
	shader.LoadUniformLocation(&uniformDegreeMidpoint, "degreemidpoint");


	//then the blur shader and its uniforms and texture
	blurShader.LoadShaderFromFile("blurVS.glsl", GL_VERTEX_SHADER);
	blurShader.LoadShaderFromFile("blurFS.glsl", GL_FRAGMENT_SHADER);
	blurShader.CreateProgram();

	blurShader.LoadUniformLocation(&blurUniformResolution, "resolution");
	blurShader.LoadUniformLocation(&blurUniformDirection, "blurdirection");
	blurShader.LoadUniformLocation(&blurUniformGaussianValues, "gaussianweight");
	blurShader.LoadUniformLocation(&blurUniformGaussianOffsets, "gaussianoffset");
	blurShader.LoadUniformLocation(&blurUniformPixels, "sizeofblurdata");

	blurTextureLocation = glGetUniformLocation(blurShader.program, "preblurtexture");
	blurShader.UseMe();
	glUniform1i(blurTextureLocation, 0);

	//the the maxvalue shader
	maxvalShader.LoadShaderFromFile("maxvalVS.glsl", GL_VERTEX_SHADER);
	maxvalShader.LoadShaderFromFile("maxvalFS.glsl", GL_FRAGMENT_SHADER);
	maxvalShader.CreateProgram();
	//maxvalShader.LoadUniformLocation(&maxvalUniformResolution, "resolution");
	maxvalShader.UseMe();
	maxvalShader.LoadUniformLocation(&maxvalUniformSquareSize, "squaresize");
	maxvalShader.LoadUniformLocation(&maxvalTextureLocation, "texturetoreduce");
	//maxvalTextureLocation = glGetUniformLocation(maxvalShader.program, "texturetoreduce");
	glUniform1i(maxvalTextureLocation, 0);
	printf("maxvalTextureLocation: %i\n", maxvalTextureLocation);

}

void HeatmapLayer::GaussianBlur(float blurSigma, float width, float height)
{
	if (blurSigma < 0.3f) {	//smaller than this is undetectable.
		return;
	}
	
	//fetch gaussian data
	float uniformOffsets[100];
	float uniformWeights[100];

	size_t numberofblurpoints = FetchGaussianValues(blurSigma, uniformOffsets, uniformWeights, 100);

	blurShader.UseMe();
	blurShader.SetUniform(blurUniformResolution, width, height);
	blurShader.SetUniform(blurUniformDirection, 1.0f, 0.0f);
	//blurShader.SetUniform(blurUniformGaussianOffsets, numberofblurpoints, uniformOffsets); //overloading defaults to the 4 vector
	glUniform1fv(blurUniformGaussianOffsets, numberofblurpoints, uniformOffsets);
	glUniform1fv(blurUniformGaussianValues, numberofblurpoints, uniformWeights);
	blurShader.SetUniform(blurUniformPixels, (int)numberofblurpoints);

	glBindFramebuffer(GL_FRAMEBUFFER, fboBlur);
	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, texture);	//we bind the texture from the initial plot

	DisplayIfGLError("before blur", false);
	glBindVertexArray(vaoSquare);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	//second pass, back to first fbo
	blurShader.SetUniform(blurUniformDirection, 0.0f, 1.0f);	//the rest is the same as above

	glBindFramebuffer(GL_FRAMEBUFFER, fboToDrawHeatmap);
	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, blurTexture);	//we bind the texture from the initial plot

	DisplayIfGLError("within blur", false);
	glBindVertexArray(vaoSquare);

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);

}


float HeatmapLayer::ReadPixelsAndFindMax(int width, int height)
{
	float* data = (float*)malloc(sizeof(float) * width * height);
	if (!data) return 1.0f;

	glReadPixels(0, 0, width, height, GL_RED, GL_FLOAT, data);

	float maxVal = data[0];
	for (int i = 1; i < width * height; i++)
	{
		if (data[i] > maxVal) maxVal = data[i];
	}
	free(data);
	return maxVal;

}

bool HeatmapLayer::NeedsRedraw(const NSWE& currentNSWE, const RectDimension& windowDims)
{
	if ((optionsWhenDrawn.heatmapmaxvalue != globalOptions.heatmapmaxvalue) ||
		(optionsWhenDrawn.earliestTimeToShow != globalOptions.earliestTimeToShow) ||
		(optionsWhenDrawn.latestTimeToShow != globalOptions.latestTimeToShow) ||
		(optionsWhenDrawn.gaussianblur != globalOptions.gaussianblur) ||
		(optionsWhenDrawn.minimumaccuracy != globalOptions.minimumaccuracy)||
		(optionsWhenDrawn.heatmapPaletteIndex != globalOptions.heatmapPaletteIndex) ||
		(optionsWhenDrawn.debug != globalOptions.debug)
		)
		return true;

	if (nsweWhenDrawn != currentNSWE)	return true;
	if (dimensionsWhenDrawn != windowDims) return true;
	
	return false;
}

void HeatmapLayer::UpdateSettingsWhenDrawn(const NSWE& currentNSWE, const RectDimension& windowDim)
{
	optionsWhenDrawn = globalOptions;
	nsweWhenDrawn = currentNSWE;
	dimensionsWhenDrawn = windowDim;
}

float HeatmapLayer::FindMaxValueWithReductionShader(int width, int height, int reductionFactor)	//this method currently loses some of the edge due to rounding
{
	float returnMaxVal=500.0; //gets overridden
	
	maxvalShader.UseMe();
	maxvalShader.SetUniform(maxvalUniformSquareSize, reductionFactor);

	int smallerWidth = width;
	int smallerHeight = height;

	glBlendFunc(GL_ONE, GL_ZERO);

	unsigned int readTexture = texture;	//start with the heatmap
	unsigned int writeBufferNumber = 0;

	int passno = 0;
	while (smallerWidth > 16 && smallerHeight > 16 && passno<3) {
		passno++;
		smallerWidth = (smallerWidth+reductionFactor-1) / reductionFactor;
		smallerHeight = (smallerHeight + reductionFactor - 1) / reductionFactor;

		//printf("%i: %i x %i. Buffer %i. ", passno, smallerWidth, smallerHeight, writeBufferNumber);
		glBindTexture(GL_TEXTURE_2D, maxvalTexture[writeBufferNumber]);	//resize.
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, smallerWidth, smallerHeight, 0, GL_RED, GL_FLOAT, NULL);

		glBindFramebuffer(GL_FRAMEBUFFER, fboMaxVal[writeBufferNumber]);	//we draw onto the framebuffer
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, readTexture);	//we firstly bind the from the heatmap

		glBindVertexArray(vaoSquare);
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		readTexture = maxvalTexture[writeBufferNumber];	//update the next texture to "read"
		writeBufferNumber = 1 - writeBufferNumber; //alt from 0 to 1;

		//returnMaxVal = ReadPixelsAndFindMax(smallerWidth, smallerHeight);
		//printf("Max val from pass %i, w:%i h:%i. %f.\n", passno, smallerWidth, smallerHeight, returnMaxVal);

	}
	returnMaxVal = ReadPixelsAndFindMax(smallerWidth, smallerHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return returnMaxVal;
}
