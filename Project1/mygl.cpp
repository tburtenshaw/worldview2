#include "header.h"
#include "mygl.h"
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

//Static members need setting
unsigned int GLRenderLayer::vboLocations = 0;
unsigned int GLRenderLayer::vaoSquare = 0;
unsigned int GLRenderLayer::vboSquare = 0;

TimeLookup GLRenderLayer::timeLookup[] = { 0 };
TimeLookup GLRenderLayer::knownStart = { 0 };
TimeLookup GLRenderLayer::knownEnd = { 0 };


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

void GLRenderLayer::CreateTimeLookupTable(LODInfo& lodInfo, int lod)
{
	//Lookup table for start/end times
	//we divide the list into 33 (pieces+1) sections, (start and end are known), so can start DrawArray at that point
	size_t interval = lodInfo.lodLength[lod] / (lookupPieces+1);
	size_t c = lodInfo.lodStart[lod];
	
	for (int i = 0; i < lookupPieces; i++) {
		c += interval;
		timeLookup[i].index = c;
		timeLookup[i].t = lodInfo.pathPlotLocations[c].timestamp;

		//printf("Lookup %i. Count: %i, ts:%i\n", i, c, locs[c].timestamp);
	}
}

void GLRenderLayer::UseLocationVBO()
{
	vbo = vboLocations;
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	DisplayIfGLError("after UseLocationVBO()", false);
}

void GLRenderLayer::LookupFirstAndCount(unsigned long starttime, unsigned long endtime, int lod, GLint* first, GLsizei* count)
{
	if (lod > 0) {
		return;
	}
	
	//Check if we've already calculated it
	if ((starttime == knownStart.t) && (endtime == knownEnd.t)) {
		*first = knownStart.index;
		*count = knownEnd.index- knownStart.index;
		//printf("k");
		return;
	}
	
	
	*first = 0;
	int i = 0;
	for (; (i < 32) && (timeLookup[i].t < starttime); i++) {	//don't run if we've exceeded start time
		*first = timeLookup[i].index;	//will only be changed if
	}
	knownStart.t = starttime;
	knownStart.index = *first;

	//after this, 'i' will be at the next lookup index
	//printf("start time:%i. i %i, s: %i. \n", starttime, i, *first);
	for (int e = i; e < 32; e++) {
		//printf("endtime %i. e:%i, t:%i.\n", endtime, e, timeLookup[e].t);
		if (timeLookup[e].t >= endtime) {
			*count = timeLookup[e].index-1 - *first;
			knownEnd.t = endtime;
			knownEnd.index = timeLookup[e].index-1;
			//printf("E:%i\n", e);
			return;
		}
	}
	
	//*count = locationsCount - *first;
	knownEnd.t = endtime;
	//knownEnd.index = locationsCount-1;


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

void PointsLayer::Draw(LODInfo& lodInfo, int lod, float width, float height, NSWE* nswe, GlobalOptions* options)
{
	//update uniform shader variables
	shader.UseMe();
	shader.SetUniform(uniformNswe, nswe);
	shader.SetUniform(uniformResolution, width, height);
	shader.SetUniform(uniformDegreeSpan, nswe->width(), nswe->height());
	shader.SetUniform(uniformDegreeMidpoint, (nswe->west + nswe->east) / 2.0, (nswe->north + nswe->south) / 2.0);


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
	DisplayIfGLError("before dapts", false);
	glDrawArrays(GL_POINTS, lodInfo.lodStart[lod], lodInfo.lodLength[lod]);
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
	glDrawArrays(GL_LINE_STRIP, lodInfo.lodStart[lod], lodInfo.lodLength[lod]);

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
	MakeHighresImageTexture();
}

void BackgroundLayer::Draw(MainViewport* vp, const GlobalOptions &options)
{

	constexpr int maxAtlasDraws = 8;	//check doesn't exceed what is in the backgroundFS shader

	int numberOfAtlasDraws=0;
	vec2f highresMult[maxAtlasDraws] = { 0.f};
	vec2f highresAdd[maxAtlasDraws] = { 0.f};
	vec4f highresNSWE[maxAtlasDraws] = { 0.0f };

	if (vp->DegreesPerPixel() < 360.0/4096.0) {
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


	shader.SetUniform(uniformMaxHeatmapValue, options.heatmapmaxvalue);
	shader.SetUniform(uniformPaletteNumber, options.palette);

	//std::cout << "wt:" << worldTexture << ", hrt:" << highresTexture << ", atlastex:" << atlas.getTexture() << ", hmT " << heatmapTexture << "\n";

	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, worldTexture);

	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, atlas.getTexture());

	glActiveTexture(GL_TEXTURE0 + 2);
	glBindTexture(GL_TEXTURE_2D, heatmapTexture);

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

void HeatmapLayer::Draw(LODInfo& lodInfo, int lod, float width, float height, NSWE* nswe, GlobalOptions* options)
{
	shader.UseMe();
	
	shader.SetUniform(uniformNswe, nswe);
	shader.SetUniform(uniformResolution, width, height);
	shader.SetUniform(uniformDegreeSpan, nswe->width(), nswe->height());
	shader.SetUniform(uniformDegreeMidpoint, (nswe->west+nswe->east) / 2.0, (nswe->north+nswe->south) / 2.0);


	shader.SetUniform(uniformEarliestTimeToShow, options->earliestTimeToShow);
	shader.SetUniform(uniformLatestTimeToShow, options->latestTimeToShow);
	shader.SetUniform(uniformMinimumAccuracy, (unsigned long)options->minimumaccuracy);


	glBindFramebuffer(GL_FRAMEBUFFER, fboToDrawHeatmap);
	//change the blending options

	glBlendFunc(GL_ONE, GL_ONE);
	glBlendEquation(GL_FUNC_ADD);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindVertexArray(vao);

	//Use lookup table to skip some
	//GLint first = 0;
	//GLsizei count = locationsCount;

	//LookupFirstAndCount(options->earliestTimeToShow, options->latestTimeToShow, lod, &first, &count);
	//printf("first: %i, count: %i.\n", first, count);
	DisplayIfGLError("before dahm", false);
	glDrawArrays(GL_LINE_STRIP, lodInfo.lodStart[lod], lodInfo.lodLength[lod]);
	
	//glDrawArrays(GL_POINTS, 0, locationsCount );
	DisplayIfGLError("after dahm", false);

	//then the blur pass.
	GaussianBlur(options->gaussianblur, width, height);

	//now find the brigtest point
	options->heatmapmaxvalue = FindMaxValueWithReductionShader(width, height, 4);	//8 might be slightly faster

	//now back to normal
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glBlendEquation(GL_FUNC_ADD);

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

/*
size_t HeatmapLayer::FetchGaussianValuesCalculate(float sigma, float* offsets, float* weights, int maxnumber)
{
	//ripped all this from my trial, need to increase efficiency	
	float p = 0.0;
	float cumul = 0.0;


	float storedvalues[50] = { 0.0 };
	float storedpixels[50] = { 0.0 };



	int n = 0;
	for (float x = -0.5; ((1.0 - cumul * 2.0) > 0.0001) && n < maxnumber/2; x += 1.0) {
		float f1 = 0.5 * (1 + std::erf(x / sigma * std::sqrt(2)));
		float f2 = 0.5 * (1 + std::erf((x + 1.0) / sigma * std::sqrt(2)));
		p = f2 - f1;
		cumul += p;
		if (x < 0.5) { cumul /= 2; }    //use half of the first one

		storedvalues[n] = p;
		storedpixels[n] = x + 0.5;

		//printf("%i %.1f, %f\n",n,x+0.5,p);
		n++;
	}
	

	int k = 0;
	for (int i = -n + 1; i < n; i++) {
		if (i < 0) {
			offsets[k] = -storedpixels[std::abs(i)];
			weights[k] = storedvalues[std::abs(i)];
		}
		else {
			offsets[k] = storedpixels[i];
			weights[k] = storedvalues[i];
		}
		k++;
	
	}
	
	return k;
	
	//return size_t();
}
*/

size_t HeatmapLayer::FetchGaussianValues(float sigma, float* offsets, float* weights, int maxnumber) {	//casts sigma to nearest 0.1, uses precomputed values
	//theres a lot of duplication (near 50%) duplication in the table, but saves doing a + and - in the shader.
	
	static const int numberOfValuesPerRow[251] = { 1,
		1, 1, 3, 3, 3, 3, 3, 5, 5, 5, 5, 5, 7, 7, 7, 7, 7, 9, 9, 9, 9, 9, 9, 11, 11, 11, 11, 11, 13, 13, 13, 13, 13, 15, 15, 15, 15, 15, 17, 17, 17, 17, 17, 19, 19, 19, 19, 19, 21, 21,
		21, 21, 21, 23, 23, 23, 23, 23, 23, 25, 25, 25, 25, 25, 27, 27, 27, 27, 27, 29, 29, 29, 29, 29, 31, 31, 31, 31, 31, 33, 33, 33, 33, 33, 35, 35, 35, 35, 35, 37, 37, 37, 37, 37, 37, 39, 39, 39, 39, 39,
		41, 41, 41, 41, 41, 43, 43, 43, 43, 43, 45, 45, 45, 45, 45, 47, 47, 47, 47, 47, 49, 49, 49, 49, 49, 51, 51, 51, 51, 51, 51, 53, 53, 53, 53, 53, 55, 55, 55, 55, 55, 57, 57, 57, 57, 57, 59, 59, 59, 59,
		59, 61, 61, 61, 61, 61, 63, 63, 63, 63, 63, 65, 65, 65, 65, 65, 65, 67, 67, 67, 67, 67, 69, 69, 69, 69, 69, 71, 71, 71, 71, 71, 73, 73, 73, 73, 73, 75, 75, 75, 75, 75, 77, 77, 77, 77, 77, 79, 79, 79,
		79, 79, 79, 81, 81, 81, 81, 81, 83, 83, 83, 83, 83, 85, 85, 85, 85, 85, 87, 87, 87, 87, 87, 89, 89, 89, 89, 89, 91, 91, 91, 91, 91, 93, 93, 93, 93, 93, 93, 95, 95, 95, 95, 95, 97, 97, 97, 97, 97, 99,
	};

	static const float gaussianMatrix[251][99] = {
		{1}, //Row:[0]. 1 items, sigma=0.0
		{ 1 }, //Row:[1]. 1 items, sigma=0.1
		{ 1 }, //Row:[2]. 1 items, sigma=0.2
		{ 0.00042906033, 0.99914188, 0.00042906033 }, //Row:[3]. 3 items, sigma=0.3
		{ 0.0062096653, 0.98758067, 0.0062096653 }, //Row:[4]. 3 items, sigma=0.4
		{ 0.022750132, 0.95449974, 0.022750132 }, //Row:[5]. 3 items, sigma=0.5
		{ 0.047790257, 0.90441949, 0.047790257 }, //Row:[6]. 3 items, sigma=0.6
		{ 0.07656069, 0.84687862, 0.07656069 }, //Row:[7]. 3 items, sigma=0.7
		{ 8.8417162e-05, 0.10556136, 0.78870045, 0.10556136, 8.8417162e-05 }, //Row:[8]. 5 items, sigma=0.8
		{ 0.00042905203, 0.13283121, 0.73347948, 0.13283121, 0.00042905203 }, //Row:[9]. 5 items, sigma=0.9
		{ 0.001349726, 0.15730547, 0.68268961, 0.15730547, 0.001349726 }, //Row:[10]. 5 items, sigma=1.0
		{ 0.0031913671, 0.17845916, 0.63669896, 0.17845916, 0.0031913671 }, //Row:[11]. 5 items, sigma=1.1
		{ 0.0062003927, 0.1961249, 0.59534942, 0.1961249, 0.0062003927 }, //Row:[12]. 5 items, sigma=1.2
		{ 5.9967292e-05, 0.010448145, 0.21037005, 0.55824368, 0.21037005, 0.010448145, 5.9967292e-05 }, //Row:[13]. 7 items, sigma=1.3
		{ 0.00017731494, 0.015884848, 0.22146306, 0.52494956, 0.22146306, 0.015884848, 0.00017731494 }, //Row:[14]. 7 items, sigma=1.4
		{ 0.00042796703, 0.022321509, 0.22974284, 0.49501536, 0.22974284, 0.022321509, 0.00042796703 }, //Row:[15]. 7 items, sigma=1.5
		{ 0.00088468842, 0.029509071, 0.2355909, 0.46803068, 0.2355909, 0.029509071, 0.00088468842 }, //Row:[16]. 7 items, sigma=1.6
		{ 0.001621171, 0.037177232, 0.23938605, 0.4436311, 0.23938605, 0.037177232, 0.001621171 }, //Row:[17]. 7 items, sigma=1.7
		{ 5.0129152e-05, 0.0026863134, 0.045053814, 0.24146707, 0.42148534, 0.24146707, 0.045053814, 0.0026863134, 5.0129152e-05 }, //Row:[18]. 9 items, sigma=1.8
		{ 0.00011386211, 0.0041349908, 0.05292485, 0.24216058, 0.40133143, 0.24216058, 0.05292485, 0.0041349908, 0.00011386211 }, //Row:[19]. 9 items, sigma=1.9
		{ 0.00022998644, 0.0059777913, 0.060598291, 0.24173109, 0.38292568, 0.24173109, 0.060598291, 0.0059777913, 0.00022998644 }, //Row:[20]. 9 items, sigma=2.0
		{ 0.00042197661, 0.0082069356, 0.067931777, 0.24040764, 0.36606334, 0.24040764, 0.067931777, 0.0082069356, 0.00042197661 }, //Row:[21]. 9 items, sigma=2.1
		{ 0.00071505833, 0.010794316, 0.074824485, 0.2383819, 0.35056849, 0.2383819, 0.074824485, 0.010794316, 0.00071505833 }, //Row:[22]. 9 items, sigma=2.2
		{ 0.0011338583, 0.013696658, 0.081211801, 0.23581273, 0.3362899, 0.23581273, 0.081211801, 0.013696658, 0.0011338583 }, //Row:[23]. 9 items, sigma=2.3
		{ 8.6545197e-05, 0.001680967, 0.016841873, 0.087039764, 0.23281176, 0.32307818, 0.23281176, 0.087039764, 0.016841873, 0.001680967, 8.6545197e-05 }, //Row:[24]. 11 items, sigma=2.4
		{ 0.00015468015, 0.0023970058, 0.020195986, 0.092320522, 0.22950957, 0.31084447, 0.22950957, 0.092320522, 0.020195986, 0.0023970058, 0.00015468015 }, //Row:[25]. 11 items, sigma=2.5
		{ 0.00025902122, 0.0032815408, 0.02368934, 0.097048546, 0.22598169, 0.29947972, 0.22598169, 0.097048546, 0.02368934, 0.0032815408, 0.00025902122 }, //Row:[26]. 11 items, sigma=2.6
		{ 0.00041016124, 0.0043379163, 0.027264973, 0.10124091, 0.22229721, 0.28889765, 0.22229721, 0.10124091, 0.027264973, 0.0043379163, 0.00041016124 }, //Row:[27]. 11 items, sigma=2.7
		{ 0.00061888978, 0.0055635863, 0.030870869, 0.10492339, 0.21851181, 0.27902291, 0.21851181, 0.10492339, 0.030870869, 0.0055635863, 0.00061888978 }, //Row:[28]. 11 items, sigma=2.8
		{ 7.1270659e-05, 0.00088257707, 0.0069378806, 0.034448328, 0.10811436, 0.2146573, 0.26977657, 0.2146573, 0.10811436, 0.034448328, 0.0069378806, 0.00088257707, 7.1270659e-05 }, //Row:[29]. 13 items, sigma=2.9
		{ 0.00011665272, 0.0012281614, 0.0084665604, 0.037976153, 0.11086603, 0.21078722, 0.26111845, 0.21078722, 0.11086603, 0.037976153, 0.0084665604, 0.0012281614, 0.00011665272 }, //Row:[30]. 13 items, sigma=3.0
		{ 0.00018218034, 0.0016550146, 0.01012623, 0.041414061, 0.11320598, 0.2069219, 0.25298926, 0.2069219, 0.11320598, 0.041414061, 0.01012623, 0.0016550146, 0.00018218034 }, //Row:[31]. 13 items, sigma=3.1
		{ 0.000273015, 0.0021680804, 0.011898855, 0.044735836, 0.11516932, 0.2030833, 0.24534317, 0.2030833, 0.11516932, 0.044735836, 0.011898855, 0.0021680804, 0.000273015 }, //Row:[32]. 13 items, sigma=3.2
		{ 0.00039450015, 0.002770235, 0.01376525, 0.047921325, 0.11679033, 0.1992886, 0.23813952, 0.1992886, 0.11679033, 0.047921325, 0.01376525, 0.002770235, 0.00039450015 }, //Row:[33]. 13 items, sigma=3.3
		{ 6.1334414e-05, 0.00054249353, 0.0034528575, 0.015696496, 0.050946364, 0.11809242, 0.1955417, 0.23133268, 0.1955417, 0.11809242, 0.050946364, 0.015696496, 0.0034528575, 0.00054249353, 6.1334414e-05 }, //Row:[34]. 15 items, sigma=3.4
		{ 9.3995934e-05, 0.00073586248, 0.0042286723, 0.017687351, 0.053814808, 0.11912046, 0.19186673, 0.22490425, 0.19186673, 0.11912046, 0.053814808, 0.017687351, 0.0042286723, 0.00073586248, 9.3995934e-05 }, //Row:[35]. 15 items, sigma=3.5
		{ 0.00013905027, 0.00097283648, 0.005088506, 0.019713335, 0.056514391, 0.11989717, 0.18826516, 0.21881911, 0.18826516, 0.11989717, 0.056514391, 0.019713335, 0.005088506, 0.00097283648, 0.00013905027 }, //Row:[36]. 15 items, sigma=3.6
		{ 0.00019929801, 0.001256897, 0.0060275985, 0.021757154, 0.059042118, 0.12044906, 0.18474236, 0.21305102, 0.18474236, 0.12044906, 0.059042118, 0.021757154, 0.0060275985, 0.001256897, 0.00019929801 }, //Row:[37]. 15 items, sigma=3.7
		{ 0.0002776565, 0.0015907539, 0.0070399813, 0.023803048, 0.061397747, 0.12080051, 0.18130211, 0.20757639, 0.18130211, 0.12080051, 0.061397747, 0.023803048, 0.0070399813, 0.0015907539, 0.0002776565 }, //Row:[38]. 15 items, sigma=3.8
		{ 5.4228217e-05, 0.00036983578, 0.0019690562, 0.0081115489, 0.025829705, 0.063576033, 0.1209666, 0.17793964, 0.2023667, 0.17793964, 0.1209666, 0.063576033, 0.025829705, 0.0081115489, 0.0019690562, 0.00036983578, 5.4228217e-05 }, //Row:[39]. 17 items, sigma=3.9
		{ 7.8986233e-05, 0.00048986523, 0.0024039957, 0.0092459669, 0.027835942, 0.065591874, 0.12097884, 0.17466758, 0.19741391, 0.17466758, 0.12097884, 0.065591874, 0.027835942, 0.0092459669, 0.0024039957, 0.00048986523, 7.8986233e-05 }, //Row:[40]. 17 items, sigma=4.0
		{ 0.00011192551, 0.00063543502, 0.0028907575, 0.010430365, 0.029806129, 0.067445345, 0.12085068, 0.17148156, 0.19269561, 0.17148156, 0.12085068, 0.067445345, 0.029806129, 0.010430365, 0.0028907575, 0.00063543502, 0.00011192551 }, //Row:[41]. 17 items, sigma=4.1
		{ 0.00015469266, 0.00080879874, 0.0034285507, 0.011656547, 0.03173111, 0.069142333, 0.12059866, 0.1683814, 0.18819582, 0.1683814, 0.12059866, 0.069142333, 0.03173111, 0.011656547, 0.0034285507, 0.00080879874, 0.00015469266 }, //Row:[42]. 17 items, sigma=4.2
		{ 0.00020900622, 0.0010118987, 0.0040158697, 0.012916377, 0.033603297, 0.070689441, 0.12023773, 0.16536638, 0.18389999, 0.16536638, 0.12023773, 0.070689441, 0.033603297, 0.012916377, 0.0040158697, 0.0010118987, 0.00020900622 }, //Row:[43]. 17 items, sigma=4.3
		{ 4.8817751e-05, 0.00027087103, 0.0012405793, 0.0046448432, 0.014196196, 0.035410813, 0.072088014, 0.11977559, 0.1624297, 0.17978915, 0.1624297, 0.11977559, 0.072088014, 0.035410813, 0.014196196, 0.0046448432, 0.0012405793, 0.00027087103, 4.8817751e-05 }, //Row:[44]. 19 items, sigma=4.4
		{ 6.8294901e-05, 0.00035121653, 0.0015052411, 0.005322016, 0.015497634, 0.037158048, 0.073354629, 0.11923355, 0.15957918, 0.17586038, 0.15957918, 0.11923355, 0.073354629, 0.037158048, 0.015497634, 0.005322016, 0.0015052411, 0.00035121653, 6.8294901e-05 }, //Row:[45]. 19 items, sigma=4.5
		{ 9.3463316e-05, 0.00044759974, 0.0018025736, 0.0060401969, 0.016809453, 0.038837273, 0.074492689, 0.11861839, 0.15680912, 0.1720985, 0.15680912, 0.11861839, 0.074492689, 0.038837273, 0.016809453, 0.0060401969, 0.0018025736, 0.00044759974, 9.3463316e-05 }, //Row:[46]. 19 items, sigma=4.6
		{ 0.000125353, 0.00056148296, 0.0021328732, 0.0067958754, 0.018124929, 0.040445821, 0.075509616, 0.11793977, 0.15411764, 0.16849326, 0.15411764, 0.11793977, 0.075509616, 0.040445821, 0.018124929, 0.0067958754, 0.0021328732, 0.00056148296, 0.000125353 }, //Row:[47]. 19 items, sigma=4.7
		{ 0.00016503885, 0.00069419385, 0.0024960504, 0.0075853117, 0.019437891, 0.041981954, 0.076412753, 0.11720638, 0.1515028, 0.16503527, 0.1515028, 0.11720638, 0.076412753, 0.041981954, 0.019437891, 0.0075853117, 0.0024960504, 0.00069419385, 0.00016503885 }, //Row:[48]. 19 items, sigma=4.8
		{ 4.4516349e-05, 0.00020893379, 0.00084221317, 0.0028869687, 0.0083999393, 0.020738061, 0.043440057, 0.077204604, 0.11642128, 0.14895785, 0.16171116, 0.14895785, 0.11642128, 0.077204604, 0.043440057, 0.020738061, 0.0083999393, 0.0028869687, 0.00084221317, 0.00020893379,
			4.4516349e-05 }, //Row:[49]. 21 items, sigma=4.9
		{ 6.0273319e-05, 0.00026585225, 0.0010142398, 0.003312561, 0.0092435305, 0.022028143, 0.044827611, 0.077899866, 0.11559913, 0.14648844, 0.15852069, 0.14648844, 0.11559913, 0.077899866, 0.044827611, 0.022028143, 0.0092435305, 0.003312561, 0.0010142398, 0.00026585225,
			6.0273319e-05 }, //Row:[50]. 21 items, sigma=5.0
		{ 8.0152669e-05, 0.00033341499, 0.0012076033, 0.003768293, 0.010108692, 0.023300247, 0.046141513, 0.078501818, 0.11474272, 0.14408903, 0.15545304, 0.14408903, 0.11474272, 0.078501818, 0.046141513, 0.023300247, 0.010108692, 0.003768293, 0.0012076033, 0.00033341499,
			8.0152669e-05 }, //Row:[51]. 21 items, sigma=5.1
		{ 0.00010483114, 0.00041259304, 0.0014228211, 0.0042527668, 0.010991487, 0.024550438, 0.047382549, 0.079016857, 0.11385752, 0.14175745, 0.15250137, 0.14175745, 0.11385752, 0.079016857, 0.047382549, 0.024550438, 0.010991487, 0.0042527668, 0.0014228211, 0.00041259304,
			0.00010483114 }, //Row:[52]. 21 items, sigma=5.2
		{ 0.00013501422, 0.00050429584, 0.0016602029, 0.0047643689, 0.011888093, 0.025775314, 0.048551918, 0.07945109, 0.11294846, 0.13949158, 0.14965932, 0.13949158, 0.11294846, 0.07945109, 0.048551918, 0.025775314, 0.011888093, 0.0047643689, 0.0016602029, 0.00050429584,
			0.00013501422 }, //Row:[53]. 21 items, sigma=5.3
		{ 4.0988592e-05, 0.00016752142, 0.00060545198, 0.0019159516, 0.0052974078, 0.012790939, 0.026968079, 0.049647252, 0.079806418, 0.11201601, 0.13728543, 0.1469171, 0.13728543, 0.11201601, 0.079806418, 0.049647252, 0.026968079, 0.012790939, 0.0052974078, 0.0019159516,
			0.00060545198, 0.00016752142, 4.0988592e-05 }, //Row:[54]. 23 items, sigma=5.4
		{ 5.4017392e-05, 0.00020964856, 0.00072337028, 0.0021965454, 0.0058565284, 0.013703107, 0.028132878, 0.050676923, 0.08009491, 0.11107065, 0.1351435, 0.14427584, 0.1351435, 0.11107065, 0.08009491, 0.050676923, 0.028132878, 0.013703107, 0.0058565284, 0.0021965454,
			0.00072337028, 0.00020964856, 5.4017392e-05 }, //Row:[55]. 23 items, sigma=5.5
		{ 7.0129778e-05, 0.00025916288, 0.00085573636, 0.0024987565, 0.0064367561, 0.014618363, 0.029264775, 0.051639998, 0.080318808, 0.11011292, 0.13306084, 0.14172751, 0.13306084, 0.11011292, 0.080318808, 0.051639998, 0.029264775, 0.014618363, 0.0064367561, 0.0024987565,
			0.00085573636, 0.00025916288, 7.0129778e-05 }, //Row:[56]. 23 items, sigma=5.6
		{ 8.9788463e-05, 0.00031672717, 0.0010030409, 0.002822101, 0.0070359678, 0.015533612, 0.030362148, 0.052538621, 0.080482974, 0.10914587, 0.13103547, 0.13926736, 0.13103547, 0.10914587, 0.080482974, 0.052538621, 0.030362148, 0.015533612, 0.0070359678, 0.002822101,
			0.0010030409, 0.00031672717, 8.9788463e-05 }, //Row:[57]. 23 items, sigma=5.7
		{ 0.00011347513, 0.00038297479, 0.0011656606, 0.003165941, 0.0076520088, 0.01644599, 0.031423743, 0.053375055, 0.080591975, 0.10817224, 0.12906546, 0.13689095, 0.12906546, 0.10817224, 0.080591975, 0.053375055, 0.031423743, 0.01644599, 0.0076520088, 0.003165941,
			0.0011656606, 0.00038297479, 0.00011347513 }, //Row:[58]. 23 items, sigma=5.8
		{ 0.0001416842, 0.00045850046, 0.0013438572, 0.0035295028, 0.0082827217, 0.01735287, 0.032448633, 0.054151647, 0.080650102, 0.10719445, 0.12714895, 0.13459417, 0.12714895, 0.10719445, 0.080650102, 0.054151647, 0.032448633, 0.01735287, 0.0082827217, 0.0035295028,
			0.0013438572, 0.00045850046, 0.0001416842 }, //Row:[59]. 23 items, sigma=5.9
		{ 4.8991279e-05, 0.00017065619, 0.00053959205, 0.0015335177, 0.0039076355, 0.008921711, 0.018247604, 0.03343193, 0.05486654, 0.080657113, 0.10621039, 0.12527987, 0.1323689, 0.12527987, 0.10621039, 0.080657113, 0.05486654, 0.03343193, 0.018247604, 0.008921711,
			0.0039076355, 0.0015335177, 0.00053959205, 0.00017065619, 4.8991279e-05 }, //Row:[60]. 25 items, sigma=6.0
		{ 6.2323093e-05, 0.00020825223, 0.00063410496, 0.0017420388, 0.0043067111, 0.0095742461, 0.019135399, 0.034380632, 0.055529521, 0.080624133, 0.10522935, 0.12346384, 0.1302189, 0.12346384, 0.10522935, 0.080624133, 0.055529521, 0.034380632, 0.019135399, 0.0095742461,
			0.0043067111, 0.0017420388, 0.00063410496, 0.00020825223, 6.2323093e-05 }, //Row:[61]. 25 items, sigma=6.1
		{ 7.8349746e-05, 0.00025163033, 0.0007391395, 0.0019660145, 0.0047223254, 0.010234962, 0.020010996, 0.035291274, 0.056139688, 0.080551345, 0.10424968, 0.12169586, 0.12813746, 0.12169586, 0.10424968, 0.080551345, 0.056139688, 0.035291274, 0.020010996, 0.010234962,
			0.0047223254, 0.0019660145, 0.0007391395, 0.00025163033, 7.8349746e-05 }, //Row:[62]. 25 items, sigma=6.2
		{ 9.741108e-05, 0.00030124077, 0.00085503845, 0.0022052438, 0.0051533176, 0.010901875, 0.020872663, 0.036163904, 0.056699433, 0.08044201, 0.10327286, 0.11997429, 0.12612142, 0.11997429, 0.10327286, 0.08044201, 0.056699433, 0.036163904, 0.020872663, 0.010901875,
			0.0051533176, 0.0022052438, 0.00085503845, 0.00030124077, 9.741108e-05 }, //Row:[63]. 25 items, sigma=6.3
		{ 0.00011985612, 0.00035751376, 0.00098207789, 0.0024594314, 0.0055984805, 0.011573088, 0.021718881, 0.036998749, 0.057211118, 0.080299166, 0.10230019, 0.11829756, 0.12416778, 0.11829756, 0.10230019, 0.080299166, 0.057211118, 0.036998749, 0.021718881, 0.011573088,
			0.0055984805, 0.0024594314, 0.00098207789, 0.00035751376, 0.00011985612 }, //Row:[64]. 25 items, sigma=6.4
		{ 4.4857432e-05, 0.0001424506, 0.00041726577, 0.0011168774, 0.0027246079, 0.0060529875, 0.012243215, 0.02254474, 0.037792602, 0.057673475, 0.080122058, 0.10132921, 0.11666056, 0.12227017, 0.11666056, 0.10132921, 0.080122058, 0.057673475, 0.037792602, 0.02254474,
			0.012243215, 0.0060529875, 0.0027246079, 0.0011168774, 0.00041726577, 0.0001424506, 4.4857432e-05 }, //Row:[65]. 27 items, sigma=6.5
		{ 5.607662e-05, 0.00017182985, 0.00048715186, 0.0012658568, 0.003006595, 0.0065218642, 0.012916837, 0.023355388, 0.03855226, 0.058095054, 0.079919607, 0.10036723, 0.11506813, 0.12043223, 0.11506813, 0.10036723, 0.079919607, 0.058095054, 0.03855226, 0.023355388,
			0.012916837, 0.0065218642, 0.003006595, 0.0012658568, 0.00048715186, 0.00017182985, 5.607662e-05 }, //Row:[66]. 27 items, sigma=6.6
		{ 6.9391721e-05, 0.00020548822, 0.00056465524, 0.0014262307, 0.0033020081, 0.0070009944, 0.013589507, 0.024147014, 0.039275492, 0.058475211, 0.079691411, 0.099412273, 0.113516, 0.11864864, 0.113516, 0.099412273, 0.079691411, 0.058475211, 0.039275492, 0.024147014,
			0.013589507, 0.0070009944, 0.0033020081, 0.0014262307, 0.00056465524, 0.00020548822, 6.9391721e-05 }, //Row:[67]. 27 items, sigma=6.7
		{ 8.504924e-05, 0.00024375256, 0.00065006157, 0.0015979876, 0.0036102436, 0.0074891091, 0.014259727, 0.024918806, 0.039963006, 0.058816076, 0.079439739, 0.098465083, 0.11200283, 0.11691706, 0.11200283, 0.098465083, 0.079439739, 0.058816076, 0.039963006, 0.024918806,
			0.014259727, 0.0074891091, 0.0036102436, 0.0015979876, 0.00065006157, 0.00024375256, 8.504924e-05 }, //Row:[68]. 27 items, sigma=6.8
		{ 0.00010330235, 0.00028693904, 0.00074361691, 0.0017810531, 0.0039306476, 0.0079849574, 0.01492611, 0.025670107, 0.040615585, 0.05911971, 0.079166701, 0.097526322, 0.1105273, 0.11523529, 0.1105273, 0.097526322, 0.079166701, 0.05911971, 0.040615585, 0.025670107,
			0.01492611, 0.0079849574, 0.0039306476, 0.0017810531, 0.00074361691, 0.00028693904, 0.00010330235 }, //Row:[69]. 27 items, sigma=6.9
		{ 4.1392557e-05, 0.00012134241, 0.00033228376, 0.00084246013, 0.0019722275, 0.0042594595, 0.0084842489, 0.015584313, 0.026397334, 0.041231013, 0.05938504, 0.078871191, 0.096593492, 0.10908512, 0.11359818, 0.10908512, 0.096593492, 0.078871191, 0.05938504, 0.041231013,
			0.026397334, 0.015584313, 0.0084842489, 0.0042594595, 0.0019722275, 0.00084246013, 0.00033228376, 0.00012134241, 4.1392557e-05 }, //Row:[70]. 29 items, sigma=7.0
		{ 5.0967922e-05, 0.00014485165, 0.00038549495, 0.00095217713, 0.0021767448, 0.0046013755, 0.0089912167, 0.016238594, 0.027105518, 0.041815615, 0.059619405, 0.078560454, 0.095672503, 0.10768049, 0.11200919, 0.10768049, 0.095672503, 0.078560454, 0.059619405, 0.041815615,
			0.027105518, 0.016238594, 0.0089912167, 0.0046013755, 0.0021767448, 0.00095217713, 0.00038549495, 0.00014485165, 5.0967922e-05 }, //Row:[71]. 29 items, sigma=7.1
		{ 6.2205627e-05, 0.00017160876, 0.0004443584, 0.0010704086, 0.0023918853, 0.004953165, 0.0095022315, 0.016885412, 0.027791907, 0.042367855, 0.059822172, 0.078233703, 0.094761296, 0.1063098, 0.11046399, 0.1063098, 0.094761296, 0.078233703, 0.059822172, 0.042367855,
			0.027791907, 0.016885412, 0.0095022315, 0.004953165, 0.0023918853, 0.0010704086, 0.0004443584, 0.00017160876, 6.2205627e-05 }, //Row:[72]. 29 items, sigma=7.2
		{ 7.5288756e-05, 0.00020185575, 0.00050910451, 0.0011972197, 0.0026173494, 0.0053140477, 0.010016176, 0.017523797, 0.02845632, 0.042888691, 0.059995096, 0.077892496, 0.093860214, 0.10497193, 0.10896083, 0.10497193, 0.093860214, 0.077892496, 0.059995096, 0.042888691,
			0.02845632, 0.017523797, 0.010016176, 0.0053140477, 0.0026173494, 0.0011972197, 0.00050910451, 0.00020185575, 7.5288756e-05 }, //Row:[73]. 29 items, sigma=7.3
		{ 9.0405301e-05, 0.00023582885, 0.00057993964, 0.0013326338, 0.0028527959, 0.0056832337, 0.010531984, 0.018152882, 0.029098672, 0.043379107, 0.060139862, 0.07753828, 0.092969545, 0.10366581, 0.10749804, 0.10366581, 0.092969545, 0.07753828, 0.060139862, 0.043379107,
			0.029098672, 0.018152882, 0.010531984, 0.0056832337, 0.0028527959, 0.0013326338, 0.00057993964, 0.00023582885, 9.0405301e-05 }, //Row:[74]. 29 items, sigma=7.4
		{ 3.8442909e-05, 0.00010509541, 0.0002711051, 0.0006543935, 0.0014739832, 0.0030951958, 0.0060572787, 0.011045987, 0.018769241, 0.029716311, 0.043837446, 0.060255428, 0.077169746, 0.092086874, 0.10238778, 0.10607139, 0.10238778, 0.092086874, 0.077169746, 0.060255428,
			0.043837446, 0.029716311, 0.018769241, 0.011045987, 0.0060572787, 0.0030951958, 0.0014739832, 0.0006543935, 0.0002711051, 0.00010509541, 3.8442909e-05 }, //Row:[75]. 31 items, sigma=7.5
		{ 4.6713424e-05, 0.00012428436, 0.00031263496, 0.0007373527, 0.0016259446, 0.0033488713, 0.0064401248, 0.011561959, 0.019376924, 0.030314046, 0.044269445, 0.060348066, 0.076792871, 0.091217128, 0.10114157, 0.10468412, 0.10114157, 0.091217128, 0.076792871, 0.060348066,
			0.044269445, 0.030314046, 0.019376924, 0.011561959, 0.0064401248, 0.0033488713, 0.0016259446, 0.0007373527, 0.00031263496, 0.00012428436, 4.6713424e-05 }, //Row:[76]. 31 items, sigma=7.6
		{ 5.632466e-05, 0.00014599226, 0.00035845179, 0.00082677137, 0.0017862532, 0.0036112128, 0.0068288169, 0.012076823, 0.019973163, 0.030889849, 0.044673935, 0.060417071, 0.076406637, 0.090358287, 0.099924085, 0.10333265, 0.099924085, 0.090358287, 0.076406637, 0.060417071,
			0.044673935, 0.030889849, 0.019973163, 0.012076823, 0.0068288169, 0.0036112128, 0.0017862532, 0.00082677137, 0.00035845179, 0.00014599226, 5.632466e-05 }, //Row:[77]. 31 items, sigma=7.7
		{ 6.7415405e-05, 0.00017040159, 0.0004087394, 0.00092274001, 0.0019547734, 0.0038817521, 0.0072225765, 0.012589723, 0.020557434, 0.031443916, 0.045051908, 0.060463835, 0.076012106, 0.089510462, 0.0987344, 0.10201563, 0.0987344, 0.089510462, 0.076012106, 0.060463835,
			0.045051908, 0.031443916, 0.020557434, 0.012589723, 0.0072225765, 0.0038817521, 0.0019547734, 0.00092274001, 0.0004087394, 0.00017040159, 6.7415405e-05 }, //Row:[78]. 31 items, sigma=7.8
		{ 8.0128109e-05, 0.00019769167, 0.00046366665, 0.0010253216, 0.002131338, 0.004160003, 0.0076206417, 0.013099861, 0.02112929, 0.031976496, 0.04540435, 0.060489682, 0.075610262, 0.08867373, 0.097571646, 0.10073178, 0.097571646, 0.08867373, 0.075610262, 0.060489682,
			0.04540435, 0.031976496, 0.02112929, 0.013099861, 0.0076206417, 0.004160003, 0.002131338, 0.0010253216, 0.00046366665, 0.00019769167, 8.0128109e-05 }, //Row:[79]. 31 items, sigma=7.9
		{ 3.5899051e-05, 9.2291815e-05, 0.00022572117, 0.00052107028, 0.0011322356, 0.0023134343, 0.0044431502, 0.0080199548, 0.013604179, 0.021686041, 0.032485566, 0.045729918, 0.060493559, 0.0751997, 0.087845828, 0.096432665, 0.099477573, 0.096432665, 0.087845828, 0.0751997,
			0.060493559, 0.045729918, 0.032485566, 0.021686041, 0.013604179, 0.0080199548, 0.0044431502, 0.0023134343, 0.0011322356, 0.00052107028, 0.00022572117, 9.2291815e-05, 3.5899051e-05 }, //Row:[80]. 33 items, sigma=8.0
		{ 4.3116043e-05, 0.00010821978, 0.00025882542, 0.0005852527, 0.0012476579, 0.0025050057, 0.0047348511, 0.0084239637, 0.014106158, 0.022231545, 0.03297563, 0.046033738, 0.060480831, 0.074785431, 0.087030948, 0.095320821, 0.098256011, 0.095320821, 0.087030948, 0.074785431,
			0.060480831, 0.046033738, 0.03297563, 0.022231545, 0.014106158, 0.0084239637, 0.0047348511, 0.0025050057, 0.0012476579, 0.0005852527, 0.00025882542, 0.00010821978, 4.3116043e-05 }, //Row:[81]. 33 items, sigma=8.1
		{ 5.1430134e-05, 0.00012613805, 0.00029524486, 0.00065441094, 0.0013696505, 0.0027038842, 0.0050326699, 0.0088300545, 0.014603247, 0.022763635, 0.033445134, 0.046314831, 0.060450718, 0.074366318, 0.086227176, 0.094233418, 0.09706408, 0.094233418, 0.086227176, 0.074366318,
			0.060450718, 0.046314831, 0.033445134, 0.022763635, 0.014603247, 0.0088300545, 0.0050326699, 0.0027038842, 0.0013696505, 0.00065441094, 0.00029524486, 0.00012613805, 5.1430134e-05 }, //Row:[82]. 33 items, sigma=8.2
		{ 6.0948425e-05, 0.00014618654, 0.00033512591, 0.00072863833, 0.0014981653, 0.0029097952, 0.0053360832, 0.009237555, 0.015094864, 0.023282116, 0.033894471, 0.046574115, 0.060404295, 0.073943077, 0.085434491, 0.093169708, 0.09590073, 0.093169708, 0.085434491, 0.073943077,
			0.060404295, 0.046574115, 0.033894471, 0.023282116, 0.015094864, 0.009237555, 0.0053360832, 0.0029097952, 0.0014981653, 0.00072863833, 0.00033512591, 0.00014618654, 6.0948425e-05 }, //Row:[83]. 33 items, sigma=8.3
		{ 7.1780788e-05, 0.00016850344, 0.00037860555, 0.0008080097, 0.0016331307, 0.0031224459, 0.0056445682, 0.0096458224, 0.015580478, 0.023786845, 0.034324057, 0.046812486, 0.060342585, 0.073516373, 0.084652856, 0.092128973, 0.09476496, 0.092128973, 0.084652856, 0.073516373,
			0.060342585, 0.046812486, 0.034324057, 0.023786845, 0.015580478, 0.0096458224, 0.0056445682, 0.0031224459, 0.0016331307, 0.0008080097, 0.00037860555, 0.00016850344, 7.1780788e-05 }, //Row:[84]. 33 items, sigma=8.4
		{ 3.3680876e-05, 8.1997919e-05, 0.00019118299, 0.00042376913, 0.00089054008, 0.0017724114, 0.0033394867, 0.0059555647, 0.010052203, 0.016057566, 0.024275688, 0.034732288, 0.047028778, 0.060264515, 0.073084779, 0.083880179, 0.091108482, 0.093653777, 0.091108482, 0.083880179,
			0.073084779, 0.060264515, 0.047028778, 0.034732288, 0.024275688, 0.016057566, 0.010052203, 0.0059555647, 0.0033394867, 0.0017724114, 0.00089054008, 0.00042376913, 0.00019118299, 8.1997919e-05, 3.3680876e-05 }, //Row:[85]. 35 items, sigma=8.5
		{ 4.0034732e-05, 9.5410611e-05, 0.00021805457, 0.00047442967, 0.00097996488, 0.0019195902, 0.0035642938, 0.0062722579, 0.010459812, 0.016529395, 0.024752292, 0.035123315, 0.04722754, 0.060174703, 0.072652562, 0.083120092, 0.090111268, 0.092569972, 0.090111268, 0.083120092,
			0.072652562, 0.060174703, 0.04722754, 0.035123315, 0.024752292, 0.016529395, 0.010459812, 0.0062722579, 0.0035642938, 0.0019195902, 0.00097996488, 0.00047442967, 0.00021805457, 9.5410611e-05, 4.0034732e-05 }, //Row:[86]. 35 items, sigma=8.6
		{ 4.7297756e-05, 0.0001104216, 0.00024753434, 0.00052897984, 0.0010745927, 0.0020728223, 0.0037948243, 0.0065924326, 0.010866388, 0.016993866, 0.025214938, 0.035495892, 0.047407889, 0.06007231, 0.072218535, 0.082370811, 0.089134983, 0.091510966, 0.089134983, 0.082370811,
			0.072218535, 0.06007231, 0.047407889, 0.035495892, 0.025214938, 0.016993866, 0.010866388, 0.0065924326, 0.0037948243, 0.0020728223, 0.0010745927, 0.00052897984, 0.00024753434, 0.0001104216, 4.7297756e-05 }, //Row:[87]. 35 items, sigma=8.7
		{ 5.5553907e-05, 0.00012713976, 0.00027973939, 0.00058750643, 0.0011744207, 0.0022319513, 0.0040307337, 0.0069155908, 0.01127141, 0.017450632, 0.025663649, 0.035850493, 0.047570624, 0.059958156, 0.071783179, 0.081632244, 0.088179011, 0.090475932, 0.088179011, 0.081632244,
			0.071783179, 0.059958156, 0.047570624, 0.035850493, 0.025663649, 0.017450632, 0.01127141, 0.0069155908, 0.0040307337, 0.0022319513, 0.0011744207, 0.00058750643, 0.00027973939, 0.00012713976, 5.5553907e-05 }, //Row:[88]. 35 items, sigma=8.8
		{ 6.4889263e-05, 0.00014567308, 0.00031478067, 0.00065008367, 0.001279429, 0.0023968051, 0.0042716716, 0.0072412472, 0.011674385, 0.017899389, 0.026098482, 0.036187594, 0.047716521, 0.05983302, 0.071346938, 0.080904294, 0.087242759, 0.089464075, 0.087242759, 0.080904294,
			0.071346938, 0.05983302, 0.047716521, 0.036187594, 0.026098482, 0.017899389, 0.011674385, 0.0072412472, 0.0042716716, 0.0023968051, 0.001279429, 0.00065008367, 0.00031478067, 0.00014567308, 6.4889263e-05 }, //Row:[89]. 35 items, sigma=8.9
		{ 3.1728336e-05, 7.3578502e-05, 0.00016431494, 0.0003509493, 0.00071496006, 0.001387768, 0.0025653842, 0.0045154711, 0.0075671169, 0.012073042, 0.018338057, 0.02651771, 0.036505867, 0.047844517, 0.059695826, 0.07090841, 0.080185044, 0.086323843, 0.088472826, 0.086323843,
			0.080185044, 0.07090841, 0.059695826, 0.047844517, 0.036505867, 0.02651771, 0.018338057, 0.012073042, 0.0075671169, 0.0045154711, 0.0025653842, 0.001387768, 0.00071496006, 0.0003509493, 0.00016431494, 7.3578502e-05, 3.1728336e-05 }, //Row:[90]. 37 items, sigma=9.0
		{ 3.7365975e-05, 8.5014458e-05, 0.00018647348, 0.00039164607, 0.0007854883, 0.0015026895, 0.0027407935, 0.0047650805, 0.0078960477, 0.012470256, 0.018769713, 0.026924753, 0.036809102, 0.047958641, 0.059550574, 0.070471275, 0.079477685, 0.085425019, 0.087504767, 0.085425019,
			0.079477685, 0.070471275, 0.059550574, 0.047958641, 0.036809102, 0.026924753, 0.018769713, 0.012470256, 0.0078960477, 0.0047650805, 0.0027407935, 0.0015026895, 0.0007854883, 0.00039164607, 0.00018647348, 8.5014458e-05, 3.7365975e-05 }, //Row:[91]. 37 items, sigma=9.1
		{ 4.3765484e-05, 9.7752251e-05, 0.00021071568, 0.00043542558, 0.00086016999, 0.0016225923, 0.0029212885, 0.0050186102, 0.0082260651, 0.012864096, 0.019192628, 0.027318212, 0.037096252, 0.048058059, 0.059396392, 0.070034349, 0.078780567, 0.084544224, 0.086557671, 0.084544224,
			0.078780567, 0.070034349, 0.059396392, 0.048058059, 0.037096252, 0.027318212, 0.019192628, 0.012864096, 0.0082260651, 0.0050186102, 0.0029212885, 0.0016225923, 0.00086016999, 0.00043542558, 0.00021071568, 9.7752251e-05, 4.3765484e-05 }, //Row:[92]. 37 items, sigma=9.2
		{ 5.0993602e-05, 0.00011187778, 0.00023713567, 0.00048236487, 0.00093902476, 0.0017473918, 0.0031066462, 0.0052757029, 0.0085567425, 0.013254191, 0.019606638, 0.027698246, 0.037367798, 0.048143446, 0.0592339, 0.06959795, 0.078093566, 0.083680948, 0.085630874, 0.083680948,
			0.078093566, 0.06959795, 0.0592339, 0.048143446, 0.037367798, 0.027698246, 0.019606638, 0.013254191, 0.0085567425, 0.0052757029, 0.0031066462, 0.0017473918, 0.00093902476, 0.00048236487, 0.00023713567, 0.00011187778, 5.0993602e-05 }, //Row:[93]. 37 items, sigma=9.3
		{ 5.9118708e-05, 0.00012747652, 0.0002658236, 0.00053253234, 0.0010220598, 0.0018769903, 0.0032966355, 0.0055360048, 0.0088876712, 0.013640199, 0.020011611, 0.028065028, 0.037624221, 0.04821545, 0.059063686, 0.069162369, 0.077416555, 0.082834698, 0.08472374, 0.082834698,
			0.077416555, 0.069162369, 0.059063686, 0.04821545, 0.037624221, 0.028065028, 0.020011611, 0.013640199, 0.0088876712, 0.0055360048, 0.0032966355, 0.0018769903, 0.0010220598, 0.00053253234, 0.0002658236, 0.00012747652, 5.9118708e-05 }, //Row:[94]. 37 items, sigma=9.4
		{ 6.8210491e-05, 0.00014463303, 0.00029686507, 0.0005859876, 0.0011092702, 0.0020112783, 0.0034910192, 0.0057991668, 0.0092184607, 0.014021806, 0.020407441, 0.028418751, 0.037865996, 0.048274698, 0.058886306, 0.068727877, 0.076749404, 0.082005001, 0.083835661, 0.082005001,
			0.076749404, 0.068727877, 0.058886306, 0.048274698, 0.037865996, 0.028418751, 0.020407441, 0.014021806, 0.0092184607, 0.0057991668, 0.0034910192, 0.0020112783, 0.0011092702, 0.0005859876, 0.00029686507, 0.00014463303, 6.8210491e-05 }, //Row:[95]. 37 items, sigma=9.5
		{ 3.5032133e-05, 7.6445975e-05, 0.00016153689, 0.0003284471, 0.00064088764, 0.0011987457, 0.0021482416, 0.0036876612, 0.0060629522, 0.0095468455, 0.014396833, 0.020792157, 0.028757727, 0.038091699, 0.048319897, 0.058700392, 0.068292826, 0.076090086, 0.081189506, 0.082964159,
			0.081189506, 0.076090086, 0.068292826, 0.058700392, 0.048319897, 0.038091699, 0.028757727, 0.020792157, 0.014396833, 0.0095468455, 0.0060629522, 0.0036876612, 0.0021482416, 0.0011987457, 0.00064088764, 0.0003284471, 0.00016153689, 7.6445975e-05, 3.5032133e-05 }, //Row:[96]. 39 items, sigma=9.6
		{ 4.0713567e-05, 8.7376578e-05, 0.00018174963, 0.00036412521, 0.00070075423, 0.0012939385, 0.0022912301, 0.0038897959, 0.0063305051, 0.0098759528, 0.014768501, 0.021169187, 0.029085657, 0.038305277, 0.048355108, 0.058509922, 0.067860924, 0.075441949, 0.080391256, 0.082112156,
			0.080391256, 0.075441949, 0.067860924, 0.058509922, 0.048355108, 0.038305277, 0.029085657, 0.021169187, 0.014768501, 0.0098759528, 0.0063305051, 0.0038897959, 0.0022912301, 0.0012939385, 0.00070075423, 0.00036412521, 0.00018174963, 8.7376578e-05, 4.0713567e-05 }, //Row:[97]. 39 items, sigma=9.7
		{ 4.7093494e-05, 9.9449541e-05, 0.00020372612, 0.00040234481, 0.00076399564, 0.0013931858, 0.0024384802, 0.0040955507, 0.0065998733, 0.010203824, 0.015134951, 0.021536876, 0.029401144, 0.038505564, 0.048379265, 0.058313742, 0.067430755, 0.074803232, 0.079608201, 0.081277493,
			0.079608201, 0.074803232, 0.067430755, 0.058313742, 0.048379265, 0.038505564, 0.029401144, 0.021536876, 0.015134951, 0.010203824, 0.0065998733, 0.0040955507, 0.0024384802, 0.0013931858, 0.00076399564, 0.00040234481, 0.00020372612, 9.9449541e-05, 4.7093494e-05 }, //Row:[98]. 39 items, sigma=9.8
		{ 5.4226913e-05, 0.00011273331, 0.00022753993, 0.00044316671, 0.00083063251, 0.0014964358, 0.0025898418, 0.0043046732, 0.0068707348, 0.010530142, 0.01549597, 0.021895213, 0.029704427, 0.038693005, 0.048392902, 0.058112293, 0.067002507, 0.074173797, 0.078839931, 0.080459653,
			0.078839931, 0.074173797, 0.067002507, 0.058112293, 0.048392902, 0.038693005, 0.029704427, 0.021895213, 0.01549597, 0.010530142, 0.0068707348, 0.0043046732, 0.0025898418, 0.0014964358, 0.00083063251, 0.00044316671, 0.00022753993, 0.00011273331, 5.4226913e-05 }, //Row:[99]. 39 items, sigma=9.9
		{ 6.2169869e-05, 0.00012729582, 0.00025326154, 0.00048664555, 0.00090067657, 0.001603627, 0.002745158, 0.0045169112, 0.007142777, 0.010854606, 0.015851369, 0.022244205, 0.02999575, 0.038868043, 0.048396531, 0.057905993, 0.066576353, 0.073553506, 0.078086051, 0.079658141,
			0.078086051, 0.073553506, 0.066576353, 0.057905993, 0.048396531, 0.038868043, 0.02999575, 0.022244205, 0.015851369, 0.010854606, 0.007142777, 0.0045169112, 0.002745158, 0.001603627, 0.00090067657, 0.00048664555, 0.00025326154, 0.00012729582, 6.2169869e-05 }, //Row:[100]. 39 items, sigma=10.0
		{ 3.297386e-05, 6.9288236e-05, 0.00014151321, 0.00027926712, 0.00053113862, 0.00097243984, 0.0017129981, 0.0029025749, 0.004730322, 0.0074140067, 0.011175245, 0.016199289, 0.022582187, 0.030273671, 0.039029416, 0.048388953, 0.057693545, 0.066150759, 0.072940528, 0.077344488,
			0.078870791, 0.077344488, 0.072940528, 0.066150759, 0.057693545, 0.048388953, 0.039029416, 0.030273671, 0.022582187, 0.016199289, 0.011175245, 0.0074140067, 0.004730322, 0.0029025749, 0.0017129981, 0.00097243984, 0.00053113862, 0.00027926712, 0.00014151321, 6.9288236e-05,
			3.297386e-05 }, //Row:[101]. 41 items, sigma=10.1
		{ 3.805173e-05, 7.8760934e-05, 0.00015857291, 0.00030874164, 0.00057980925, 0.0010490384, 0.0018275918, 0.0030650461, 0.0049477778, 0.0076872536, 0.011494917, 0.016542705, 0.022912323, 0.030541568, 0.039180668, 0.048373763, 0.057478444, 0.06572899, 0.072337847, 0.076617995,
			0.078100268, 0.076617995, 0.072337847, 0.06572899, 0.057478444, 0.048373763, 0.039180668, 0.030541568, 0.022912323, 0.016542705, 0.011494917, 0.0076872536, 0.0049477778, 0.0030650461, 0.0018275918, 0.0010490384, 0.00057980925, 0.00030874164, 0.00015857291, 7.8760934e-05,
			3.805173e-05 }, //Row:[102]. 41 items, sigma=10.2
		{ 4.3723845e-05, 8.9184597e-05, 0.00017707834, 0.00034028357, 0.0006312317, 0.0011289975, 0.0019458606, 0.0032309389, 0.0051675704, 0.0079607761, 0.011811918, 0.016880029, 0.023233209, 0.03079824, 0.039320751, 0.048349954, 0.05725958, 0.065309715, 0.071743865, 0.075904757,
			0.077344673, 0.075904757, 0.071743865, 0.065309715, 0.05725958, 0.048349954, 0.039320751, 0.03079824, 0.023233209, 0.016880029, 0.011811918, 0.0079607761, 0.0051675704, 0.0032309389, 0.0019458606, 0.0011289975, 0.0006312317, 0.00034028357, 0.00017707834, 8.9184597e-05,
			4.3723845e-05 }, //Row:[103]. 41 items, sigma=10.3
		{ 5.0034935e-05, 0.00010061463, 0.00019708988, 0.00037394596, 0.00068543241, 0.0012122932, 0.0020677087, 0.0034000758, 0.0053894544, 0.0082343024, 0.012126016, 0.017211151, 0.023544914, 0.031043946, 0.039450066, 0.048317958, 0.057037284, 0.064893052, 0.071158442, 0.075204428,
			0.076603582, 0.075204428, 0.071158442, 0.064893052, 0.057037284, 0.048317958, 0.039450066, 0.031043946, 0.023544914, 0.017211151, 0.012126016, 0.0082343024, 0.0053894544, 0.0034000758, 0.0020677087, 0.0012122932, 0.00068543241, 0.00037394596, 0.00019708988, 0.00010061463,
			5.0034935e-05 }, //Row:[104]. 41 items, sigma=10.4
		{ 5.7030575e-05, 0.00011310618, 0.00021866581, 0.00040977747, 0.00074243126, 0.0012988942, 0.0021930338, 0.0035722772, 0.0056131883, 0.0085075725, 0.012437001, 0.017535977, 0.023847517, 0.03127895, 0.039569005, 0.048278191, 0.056811869, 0.064479105, 0.07058144, 0.074516675,
			0.075876589, 0.074516675, 0.07058144, 0.064479105, 0.056811869, 0.048278191, 0.039569005, 0.03127895, 0.023847517, 0.017535977, 0.012437001, 0.0085075725, 0.0056131883, 0.0035722772, 0.0021930338, 0.0012988942, 0.00074243126, 0.00040977747, 0.00021866581, 0.00011310618,
			5.7030575e-05 }, //Row:[105]. 41 items, sigma=10.5
		{ 3.114503e-05, 6.3237728e-05, 0.00012519458, 0.00024034281, 0.00044630294, 0.00080072236, 0.0013872423, 0.0023202086, 0.0037458424, 0.0058370161, 0.0087788188, 0.012743155, 0.017852913, 0.024139592, 0.031501993, 0.039676429, 0.048229531, 0.056582109, 0.064066451, 0.070011203,
			0.073839659, 0.075161782, 0.073839659, 0.070011203, 0.064066451, 0.056582109, 0.048229531, 0.039676429, 0.031501993, 0.024139592, 0.017852913, 0.012743155, 0.0087788188, 0.0058370161, 0.0037458424, 0.0023202086, 0.0013872423, 0.00080072236, 0.00044630294, 0.00024034281,
			0.00012519458, 6.3237728e-05, 3.114503e-05 }, //Row:[106]. 43 items, sigma=10.6
		{ 3.5710726e-05, 7.1518928e-05, 0.00013974952, 0.00026499008, 0.00048637764, 0.00086312855, 0.001480108, 0.0024519359, 0.0039234052, 0.0060635222, 0.0090506202, 0.013047115, 0.018164715, 0.024424057, 0.031716157, 0.039775525, 0.048175176, 0.056351099, 0.063657992, 0.069450411,
			0.073175885, 0.074461602, 0.073175885, 0.069450411, 0.063657992, 0.056351099, 0.048175176, 0.039775525, 0.031716157, 0.024424057, 0.018164715, 0.013047115, 0.0090506202, 0.0060635222, 0.0039234052, 0.0024519359, 0.001480108, 0.00086312855, 0.00048637764, 0.00026499008,
			0.00013974952, 7.1518928e-05, 3.5710726e-05 }, //Row:[107]. 43 items, sigma=10.7
		{ 4.0786165e-05, 8.059975e-05, 0.00015550245, 0.00029133678, 0.00052871484, 0.00092832914, 0.0015761179, 0.0025867766, 0.0041034618, 0.0062911597, 0.0093214306, 0.013347395, 0.018470014, 0.0246997, 0.031920383, 0.039865335, 0.048114171, 0.056117779, 0.063252483, 0.068897611,
			0.072523732, 0.073774364, 0.072523732, 0.068897611, 0.063252483, 0.056117779, 0.048114171, 0.039865335, 0.031920383, 0.0246997, 0.018470014, 0.013347395, 0.0093214306, 0.0062911597, 0.0041034618, 0.0025867766, 0.0015761179, 0.00092832914, 0.00052871484, 0.00029133678,
			0.00015550245, 8.059975e-05, 4.0786165e-05 }, //Row:[108]. 43 items, sigma=10.8
		{ 4.6408095e-05, 9.052549e-05, 0.00017250325, 0.00031942877, 0.00057334262, 0.00099631701, 0.0016752122, 0.0027246075, 0.0042858286, 0.0065197077, 0.0095910351, 0.013643842, 0.018768778, 0.024966639, 0.032114931, 0.039946207, 0.048046864, 0.055882397, 0.062849992, 0.068352667,
			0.071882908, 0.073099719, 0.071882908, 0.068352667, 0.062849992, 0.055882397, 0.048046864, 0.039946207, 0.032114931, 0.024966639, 0.018768778, 0.013643842, 0.0095910351, 0.0065197077, 0.0042858286, 0.0027246075, 0.0016752122, 0.00099631701, 0.00057334262, 0.00031942877,
			0.00017250325, 9.052549e-05, 4.6408095e-05 }, //Row:[109]. 43 items, sigma=10.9
		{ 5.261395e-05, 0.00010134132, 0.00019080034, 0.00034930872, 0.00062028422, 0.0010670791, 0.0017773256, 0.0028653023, 0.0044703231, 0.0067489515, 0.0098592301, 0.013936319, 0.019060985, 0.025224997, 0.03230006, 0.040018483, 0.047973588, 0.055645185, 0.062450578, 0.067815447,
			0.071253134, 0.072437328, 0.071253134, 0.067815447, 0.062450578, 0.055645185, 0.047973588, 0.040018483, 0.03230006, 0.025224997, 0.019060985, 0.013936319, 0.0098592301, 0.0067489515, 0.0044703231, 0.0028653023, 0.0017773256, 0.0010670791, 0.00062028422, 0.00034930872,
			0.00019080034, 0.00010134132, 5.261395e-05 }, //Row:[110]. 43 items, sigma=11.0
		{ 2.9509257e-05, 5.8069187e-05, 0.00011171957, 0.00020906799, 0.00037964349, 0.0006681855, 0.0011392242, 0.0018810152, 0.0030073591, 0.0046553917, 0.006977311, 0.010124452, 0.01422333, 0.019345254, 0.025473534, 0.032474654, 0.040081118, 0.047893292, 0.055404989, 0.062052923,
			0.067284449, 0.070632763, 0.071785494, 0.070632763, 0.067284449, 0.062052923, 0.055404989, 0.047893292, 0.040081118, 0.032474654, 0.025473534, 0.019345254, 0.01422333, 0.010124452, 0.006977311, 0.0046553917, 0.0030073591, 0.0018810152, 0.0011392242, 0.0006681855,
			0.00037964349, 0.00020906799, 0.00011171957, 5.8069187e-05, 2.9509257e-05 }, //Row:[111]. 45 items, sigma=11.1
		{ 3.36366e-05, 6.5365272e-05, 0.00012425768, 0.00022990422, 0.00041302217, 0.00071961308, 0.001215281, 0.0019887589, 0.0031531998, 0.0048434087, 0.0072071387, 0.010389072, 0.014507317, 0.01962414, 0.025714936, 0.032641521, 0.040136987, 0.047808833, 0.055164569, 0.061659625,
			0.066762098, 0.070024087, 0.071146458, 0.070024087, 0.066762098, 0.061659625, 0.055164569, 0.047808833, 0.040136987, 0.032641521, 0.025714936, 0.01962414, 0.014507317, 0.010389072, 0.0072071387, 0.0048434087, 0.0031531998, 0.0019887589, 0.001215281, 0.00071961308,
			0.00041302217, 0.00022990422, 0.00012425768, 6.5365272e-05, 3.36366e-05 }, //Row:[112]. 45 items, sigma=11.2
		{ 3.8204471e-05, 7.3339822e-05, 0.00013779832, 0.00025215097, 0.00044827621, 0.00077337473, 0.0012940192, 0.0020992763, 0.0033014905, 0.0050329968, 0.0074370403, 0.010651721, 0.01478698, 0.019896454, 0.025948144, 0.032799712, 0.0401852, 0.047719304, 0.05492292, 0.061269525,
			0.066247066, 0.06942565, 0.070518716, 0.06942565, 0.066247066, 0.061269525, 0.05492292, 0.047719304, 0.0401852, 0.032799712, 0.025948144, 0.019896454, 0.01478698, 0.010651721, 0.0074370403, 0.0050329968, 0.0033014905, 0.0020992763, 0.0012940192, 0.00077337473,
			0.00044827621, 0.00025215097, 0.00013779832, 7.3339822e-05, 3.8204471e-05 }, //Row:[113]. 45 items, sigma=11.3
		{ 4.3243338e-05, 8.2030205e-05, 0.00015238296, 0.00027584767, 0.00048543313, 0.00082947315, 0.0013754029, 0.0022124827, 0.0034520958, 0.0052239812, 0.0076668287, 0.010912239, 0.015062231, 0.020162217, 0.026173299, 0.032949474, 0.040226056, 0.047624983, 0.054680224, 0.060882655,
			0.065739227, 0.068837205, 0.069901975, 0.068837205, 0.065739227, 0.060882655, 0.054680224, 0.047624983, 0.040226056, 0.032949474, 0.026173299, 0.020162217, 0.015062231, 0.010912239, 0.0076668287, 0.0052239812, 0.0034520958, 0.0022124827, 0.0013754029, 0.00082947315,
			0.00048543313, 0.00027584767, 0.00015238296, 8.2030205e-05, 4.3243338e-05 }, //Row:[114]. 45 items, sigma=11.4
		{ 4.8784228e-05, 9.1473746e-05, 0.00016805202, 0.00030103146, 0.00052451685, 0.00088790644, 0.0014593916, 0.00232829, 0.0036048796, 0.0054161905, 0.0078963241, 0.011170476, 0.015332994, 0.020421457, 0.02639055, 0.03309105, 0.040259846, 0.047526139, 0.054436653, 0.060499045,
			0.065238458, 0.068258513, 0.069295954, 0.068258513, 0.065238458, 0.060499045, 0.054436653, 0.047526139, 0.040259846, 0.03309105, 0.02639055, 0.020421457, 0.015332994, 0.011170476, 0.0078963241, 0.0054161905, 0.0036048796, 0.00232829, 0.0014593916, 0.00088790644,
			0.00052451685, 0.00030103146, 0.00016805202, 9.1473746e-05, 4.8784228e-05 }, //Row:[115]. 45 items, sigma=11.5
		{ 2.803746e-05, 5.3612515e-05, 0.00010046148, 0.00018359869, 0.00032649096, 0.00056430152, 0.00094742213, 0.0015446944, 0.002445361, 0.0037584597, 0.0056082108, 0.0081241084, 0.011425047, 0.015597958, 0.020672967, 0.026598797, 0.033223434, 0.040285606, 0.047421779, 0.054191124,
			0.060117473, 0.064743392, 0.067688097, 0.068699133, 0.067688097, 0.064743392, 0.060117473, 0.054191124, 0.047421779, 0.040285606, 0.033223434, 0.026598797, 0.020672967, 0.015597958, 0.011425047, 0.0081241084, 0.0056082108, 0.0037584597, 0.002445361, 0.0015446944,
			0.00094742213, 0.00056430152, 0.00032649096, 0.00018359869, 0.00010046148, 5.3612515e-05, 2.803746e-05 }, //Row:[116]. 47 items, sigma=11.6
		{ 3.1786731e-05, 6.0085617e-05, 0.00011135583, 0.00020138653, 0.000354584, 0.00060712933, 0.0010103351, 0.0016335879, 0.0025659277, 0.0039150256, 0.005802204, 0.0083523427, 0.01167815, 0.01585939, 0.020919116, 0.026800519, 0.033349188, 0.040305933, 0.047314475, 0.053946114,
			0.059740283, 0.064256234, 0.067128057, 0.068113577, 0.067128057, 0.064256234, 0.059740283, 0.053946114, 0.047314475, 0.040305933, 0.033349188, 0.026800519, 0.020919116, 0.01585939, 0.01167815, 0.0083523427, 0.005802204, 0.0039150256, 0.0025659277, 0.0016335879,
			0.0010103351, 0.00060712933, 0.000354584, 0.00020138653, 0.00011135583, 6.0085617e-05, 3.1786731e-05 }, //Row:[117]. 47 items, sigma=11.7
		{ 3.5919296e-05, 6.7139105e-05, 0.00012309668, 0.00022035557, 0.0003842437, 0.00065191674, 0.0010755339, 0.0017249227, 0.0026887976, 0.0040733454, 0.0059969138, 0.0085797736, 0.011928569, 0.01611615, 0.021158859, 0.026994771, 0.033467447, 0.040319995, 0.047203362, 0.053700672,
			0.059366396, 0.063775771, 0.066577082, 0.067537934, 0.066577082, 0.063775771, 0.059366396, 0.053700672, 0.047203362, 0.040319995, 0.033467447, 0.026994771, 0.021158859, 0.01611615, 0.011928569, 0.0085797736, 0.0059969138, 0.0040733454, 0.0026887976, 0.0017249227,
			0.0010755339, 0.00065191674, 0.0003842437, 0.00022035557, 0.00012309668, 6.7139105e-05, 3.5919296e-05 }, //Row:[118]. 47 items, sigma=11.8
		{ 4.0460628e-05, 7.4804065e-05, 0.0001357187, 0.00024053968, 0.00041549595, 0.00069867201, 0.0011429986, 0.0018186415, 0.002813872, 0.0042332836, 0.006192184, 0.0088062507, 0.012176193, 0.0163682, 0.021392253, 0.027181707, 0.033578438, 0.040328046, 0.047088664, 0.053454932,
			0.058995821, 0.063301886, 0.066034959, 0.066971959, 0.066034959, 0.063301886, 0.058995821, 0.053454932, 0.047088664, 0.040328046, 0.033578438, 0.027181707, 0.021392253, 0.0163682, 0.012176193, 0.0088062507, 0.006192184, 0.0042332836, 0.002813872, 0.0018186415,
			0.0011429986, 0.00069867201, 0.00041549595, 0.00024053968, 0.0001357187, 7.4804065e-05, 4.0460628e-05 }, //Row:[119]. 47 items, sigma=11.9
		{ 4.5436659e-05, 8.311159e-05, 0.00014925586, 0.00026197106, 0.00044836387, 0.00074739982, 0.0012127052, 0.0019146836, 0.0029410505, 0.0043947061, 0.0063878628, 0.0090316314, 0.012420921, 0.016615508, 0.021619359, 0.02736148, 0.033682384, 0.040330332, 0.046970593, 0.053209021,
			0.058628566, 0.062834465, 0.065501484, 0.066415414, 0.065501484, 0.062834465, 0.058628566, 0.053209021, 0.046970593, 0.040330332, 0.033682384, 0.02736148, 0.021619359, 0.016615508, 0.012420921, 0.0090316314, 0.0063878628, 0.0043947061, 0.0029410505, 0.0019146836,
			0.0012127052, 0.00074739982, 0.00044836387, 0.00026197106, 0.00014925586, 8.311159e-05, 4.5436659e-05 }, //Row:[120]. 47 items, sigma=12.0
		{ 2.6706118e-05, 4.9737271e-05, 9.095625e-05, 0.00016260487, 0.00028354368, 0.00048173144, 0.00079696492, 0.0012834899, 0.002011849, 0.0030690953, 0.0045563437, 0.0065826667, 0.0092546436, 0.012661523, 0.016856918, 0.021839109, 0.02753311, 0.033778365, 0.040325953, 0.046848217,
			0.052961922, 0.0582635, 0.062372261, 0.064975323, 0.065866933, 0.064975323, 0.062372261, 0.0582635, 0.052961922, 0.046848217, 0.040325953, 0.033778365, 0.02753311, 0.021839109, 0.016856918, 0.012661523, 0.0092546436, 0.0065826667, 0.0045563437, 0.0030690953,
			0.002011849, 0.0012834899, 0.00079696492, 0.00048173144, 0.00028354368, 0.00016260487, 9.095625e-05, 4.9737271e-05, 2.6706118e-05 }, //Row:[121]. 49 items, sigma=12.1
		{ 3.0127026e-05, 5.551637e-05, 0.00010049614, 0.00017792521, 0.00030741339, 0.00051774355, 0.00084949234, 0.0013574487, 0.0021121989, 0.0032000315, 0.0047201931, 0.0067785809, 0.0094772867, 0.012900042, 0.017094543, 0.022053702, 0.027698877, 0.033868723, 0.040317267, 0.046723856,
			0.052715874, 0.05790275, 0.061917289, 0.064458412, 0.06532842, 0.064458412, 0.061917289, 0.05790275, 0.052715874, 0.046723856, 0.040317267, 0.033868723, 0.027698877, 0.022053702, 0.017094543, 0.012900042, 0.0094772867, 0.0067785809, 0.0047201931, 0.0032000315,
			0.0021121989, 0.0013574487, 0.00084949234, 0.00051774355, 0.00030741339, 0.00017792521, 0.00010049614, 5.551637e-05, 3.0127026e-05 }, //Row:[122]. 49 items, sigma=12.2
		{ 3.3883491e-05, 6.1795651e-05, 0.00011075662, 0.00019424289, 0.00033260169, 0.00055540991, 0.00090397138, 0.0014335424, 0.0022146594, 0.0033327513, 0.0048851204, 0.0069744631, 0.0096984347, 0.013135397, 0.017327371, 0.022262209, 0.027857932, 0.033952662, 0.04030349, 0.046596691,
			0.052469978, 0.057545312, 0.061468439, 0.063949558, 0.064798657, 0.063949558, 0.061468439, 0.057545312, 0.052469978, 0.046596691, 0.04030349, 0.033952662, 0.027857932, 0.022262209, 0.017327371, 0.013135397, 0.0096984347, 0.0069744631, 0.0048851204, 0.0033327513,
			0.0022146594, 0.0014335424, 0.00090397138, 0.00055540991, 0.00033260169, 0.00019424289, 0.00011075662, 6.1795651e-05, 3.3883491e-05 }, //Row:[123]. 49 items, sigma=12.3
		{ 3.7996975e-05, 6.8601173e-05, 0.00012176689, 0.00021158702, 0.0003591323, 0.00059474165, 0.00096039229, 0.0015117326, 0.0023191592, 0.0034671509, 0.0050509987, 0.0071701802, 0.009917973, 0.013367519, 0.0175554, 0.022464708, 0.028010427, 0.034030383, 0.040284837, 0.0464669,
			0.052224332, 0.057191179, 0.061025603, 0.063448582, 0.064277435, 0.063448582, 0.061025603, 0.057191179, 0.052224332, 0.0464669, 0.040284837, 0.034030383, 0.028010427, 0.022464708, 0.0175554, 0.013367519, 0.009917973, 0.0071701802, 0.0050509987, 0.0034671509,
			0.0023191592, 0.0015117326, 0.00096039229, 0.00059474165, 0.0003591323, 0.00021158702, 0.00012176689, 6.8601173e-05, 3.7996975e-05 }, //Row:[124]. 49 items, sigma=12.4
		{ 4.2489316e-05, 7.5959033e-05, 0.00013355562, 0.00022998546, 0.00038702689, 0.00063574713, 0.0010187422, 0.0015919782, 0.0024256249, 0.0036031267, 0.0052177032, 0.0073656043, 0.010135794, 0.013596346, 0.017778637, 0.022661281, 0.028156515, 0.034102087, 0.040261512, 0.04633465,
			0.051979028, 0.056840347, 0.060588677, 0.062955307, 0.063764551, 0.062955307, 0.060588677, 0.056840347, 0.051979028, 0.04633465, 0.040261512, 0.034102087, 0.028156515, 0.022661281, 0.017778637, 0.013596346, 0.010135794, 0.0073656043, 0.0052177032, 0.0036031267,
			0.0024256249, 0.0015919782, 0.0010187422, 0.00063574713, 0.00038702689, 0.00022998546, 0.00013355562, 7.5959033e-05, 4.2489316e-05 }, //Row:[125]. 49 items, sigma=12.5
		{ 2.5496004e-05, 4.634202e-05, 8.2854636e-05, 0.00014511026, 0.0002484241, 0.00041526437, 0.00067739135, 0.0010779647, 0.0016731946, 0.002532941, 0.0037395347, 0.0053840717, 0.0075595718, 0.010350756, 0.013820783, 0.017996051, 0.022850974, 0.028295308, 0.034166927, 0.040232674,
			0.046199064, 0.051733113, 0.056491764, 0.060156518, 0.062468522, 0.063258771, 0.062468522, 0.060156518, 0.056491764, 0.051733113, 0.046199064, 0.040232674, 0.034166927, 0.028295308, 0.022850974, 0.017996051, 0.013820783, 0.010350756, 0.0075595718, 0.0053840717,
			0.0037395347, 0.002532941, 0.0016731946, 0.0010779647, 0.00067739135, 0.00041526437, 0.0002484241, 0.00014511026, 8.2854636e-05, 4.634202e-05, 2.5496004e-05 }, //Row:[126]. 51 items, sigma=12.6
		{ 2.8629905e-05, 5.1530897e-05, 9.1267309e-05, 0.00015841159, 0.00026888148, 0.00044581559, 0.0007216306, 0.0011399943, 0.0017572892, 0.0026429847, 0.0038782256, 0.0055519385, 0.0077539183, 0.010564718, 0.014042734, 0.018209612, 0.023035826, 0.028428908, 0.034227045, 0.040200469,
			0.046062247, 0.051488622, 0.056147375, 0.059730979, 0.061990017, 0.062761858, 0.061990017, 0.059730979, 0.056147375, 0.051488622, 0.046062247, 0.040200469, 0.034227045, 0.028428908, 0.023035826, 0.018209612, 0.014042734, 0.010564718, 0.0077539183, 0.0055519385,
			0.0038782256, 0.0026429847, 0.0017572892, 0.0011399943, 0.0007216306, 0.00044581559, 0.00026888148, 0.00015841159, 9.1267309e-05, 5.1530897e-05, 2.8629905e-05 }, //Row:[127]. 51 items, sigma=12.7
		{ 3.2059236e-05, 5.7153763e-05, 0.00010029799, 0.00017256145, 0.00029045651, 0.00047777106, 0.00076754037, 0.0012038847, 0.0018432884, 0.0027547542, 0.0040181728, 0.0057202629, 0.0079476064, 0.010776667, 0.014261235, 0.018418417, 0.023215003, 0.028556541, 0.034281701, 0.040164158,
			0.045923424, 0.051244706, 0.055806242, 0.059311037, 0.061518704, 0.062272706, 0.061518704, 0.059311037, 0.055806242, 0.051244706, 0.045923424, 0.040164158, 0.034281701, 0.028556541, 0.023215003, 0.018418417, 0.014261235, 0.010776667, 0.0079476064, 0.0057202629,
			0.0040181728, 0.0027547542, 0.0018432884, 0.0012038847, 0.00076754037, 0.00047777106, 0.00029045651, 0.00017256145, 0.00010029799, 5.7153763e-05, 3.2059236e-05 }, //Row:[128]. 51 items, sigma=12.8
		{ 3.5802203e-05, 6.3232617e-05, 0.0001099714, 0.00018758489, 0.00031317064, 0.00051114319, 0.0008151176, 0.001269611, 0.0019311411, 0.0028681707, 0.0041592747, 0.0058889316, 0.0081405279, 0.010986522, 0.014476248, 0.018622491, 0.023388596, 0.028678354, 0.034331076, 0.040123919,
			0.045782735, 0.051001436, 0.055468351, 0.058896596, 0.06105443, 0.061791135, 0.06105443, 0.058896596, 0.055468351, 0.051001436, 0.045782735, 0.040123919, 0.034331076, 0.028678354, 0.023388596, 0.018622491, 0.014476248, 0.010986522, 0.0081405279, 0.0058889316,
			0.0041592747, 0.0028681707, 0.0019311411, 0.001269611, 0.0008151176, 0.00051114319, 0.00031317064, 0.00018758489, 0.0001099714, 6.3232617e-05, 3.5802203e-05 }, //Row:[129]. 51 items, sigma=12.9
		{ 3.9877327e-05, 6.9789518e-05, 0.00012031189, 0.00020350604, 0.00033704375, 0.00054594222, 0.0008643567, 0.001337146, 0.002020794, 0.002983155, 0.004301431, 0.0060578344, 0.0083325795, 0.011194205, 0.014687743, 0.018821864, 0.023556697, 0.028794492, 0.034375344, 0.040079923,
			0.045640312, 0.05075888, 0.055133686, 0.05848756, 0.060597041, 0.061316975, 0.060597041, 0.05848756, 0.055133686, 0.05075888, 0.045640312, 0.040079923, 0.034375344, 0.028794492, 0.023556697, 0.018821864, 0.014687743, 0.011194205, 0.0083325795, 0.0060578344,
			0.004301431, 0.002983155, 0.002020794, 0.001337146, 0.0008643567, 0.00054594222, 0.00033704375, 0.00020350604, 0.00012031189, 6.9789518e-05, 3.9877327e-05 }, //Row:[130]. 51 items, sigma=13.0
		{ 4.43034e-05, 7.6846512e-05, 0.00013134338, 0.00022034803, 0.00036209411, 0.00058217626, 0.00091524962, 0.0014064598, 0.0021121923, 0.0030996274, 0.0044445427, 0.0062268644, 0.0085236629, 0.011399646, 0.014895697, 0.019016568, 0.0237194, 0.028905098, 0.034414676, 0.040032337,
			0.045496284, 0.050517098, 0.054802229, 0.058083837, 0.06014639, 0.060850058, 0.06014639, 0.058083837, 0.054802229, 0.050517098, 0.045496284, 0.040032337, 0.034414676, 0.028905098, 0.0237194, 0.019016568, 0.014895697, 0.011399646, 0.0085236629, 0.0062268644,
			0.0044445427, 0.0030996274, 0.0021121923, 0.0014064598, 0.00091524962, 0.00058217626, 0.00036209411, 0.00022034803, 0.00013134338, 7.6846512e-05, 4.43034e-05 }, //Row:[131]. 51 items, sigma=13.1
		{ 2.7272832e-05, 4.8029912e-05, 8.3356055e-05, 0.00014201976, 0.00023706342, 0.00038726886, 0.00061878175, 0.00096671641, 0.0014764512, 0.0022042103, 0.0032164386, 0.0045874431, 0.0063948487, 0.0087126152, 0.011601712, 0.015099021, 0.019205573, 0.023875731, 0.029009245, 0.034448167,
			0.039980251, 0.045349703, 0.050275081, 0.054472892, 0.057684266, 0.059701266, 0.060389152, 0.059701266, 0.057684266, 0.054472892, 0.050275081, 0.045349703, 0.039980251, 0.034448167, 0.029009245, 0.023875731, 0.019205573, 0.015099021, 0.011601712, 0.0087126152,
			0.0063948487, 0.0045874431, 0.0032164386, 0.0022042103, 0.0014764512, 0.00096671641, 0.00061878175, 0.00038726886, 0.00023706342, 0.00014201976, 8.3356055e-05, 4.8029912e-05, 2.7272832e-05 }, //Row:[132]. 53 items, sigma=13.2
		{ 3.0415868e-05, 5.3091838e-05, 9.1355785e-05, 0.00015437968, 0.00025568899, 0.00041459876, 0.00065777834, 0.0010207601, 0.0015491018, 0.0022988063, 0.0033355247, 0.0047320522, 0.0065637031, 0.0089013639, 0.011802358, 0.015299715, 0.019390934, 0.024027801, 0.029109088, 0.034477997,
			0.039925836, 0.045202702, 0.050034898, 0.054147673, 0.057290773, 0.059263545, 0.059936118, 0.059263545, 0.057290773, 0.054147673, 0.050034898, 0.045202702, 0.039925836, 0.034477997, 0.029109088, 0.024027801, 0.019390934, 0.015299715, 0.011802358, 0.0089013639,
			0.0065637031, 0.0047320522, 0.0033355247, 0.0022988063, 0.0015491018, 0.0010207601, 0.00065777834, 0.00041459876, 0.00025568899, 0.00015437968, 9.1355785e-05, 5.3091838e-05, 3.0415868e-05 }, //Row:[133]. 53 items, sigma=13.3
		{ 3.3835901e-05, 5.8551356e-05, 9.9910229e-05, 0.00016748823, 0.00027528745, 0.00044314, 0.00069821068, 0.0010764088, 0.0016234189, 0.0023949643, 0.0034558491, 0.0048773191, 0.006732374, 0.009088868, 0.012000573, 0.015496813, 0.019571738, 0.02417475, 0.029203809, 0.034503364,
			0.039868283, 0.045054435, 0.049795643, 0.053825594, 0.056902314, 0.058832135, 0.059489845, 0.058832135, 0.056902314, 0.053825594, 0.049795643, 0.045054435, 0.039868283, 0.034503364, 0.029203809, 0.02417475, 0.019571738, 0.015496813, 0.012000573, 0.009088868,
			0.006732374, 0.0048773191, 0.0034558491, 0.0023949643, 0.0016234189, 0.0010764088, 0.00069821068, 0.00044314, 0.00027528745, 0.00016748823, 9.9910229e-05, 5.8551356e-05, 3.3835901e-05 }, //Row:[134]. 53 items, sigma=13.4
		{ 3.7548742e-05, 6.4427226e-05, 0.00010904017, 0.00018136633, 0.00029587682, 0.00047290359, 0.00074007791, 0.0011336447, 0.0016993641, 0.0024926244, 0.0035773319, 0.0050231514, 0.0069007679, 0.0092750479, 0.012196308, 0.015690309, 0.019748031, 0.024316674, 0.029293541, 0.034524422,
			0.039807735, 0.044905008, 0.049557364, 0.053506632, 0.056518804, 0.058406904, 0.059050186, 0.058406904, 0.056518804, 0.053506632, 0.049557364, 0.044905008, 0.039807735, 0.034524422, 0.029293541, 0.024316674, 0.019748031, 0.015690309, 0.012196308, 0.0092750479,
			0.0069007679, 0.0050231514, 0.0035773319, 0.0024926244, 0.0016993641, 0.0011336447, 0.00074007791, 0.00047290359, 0.00029587682, 0.00018136633, 0.00010904017, 6.4427226e-05, 3.7548742e-05 }, //Row:[135]. 53 items, sigma=13.5
		{ 4.1570429e-05, 7.0738222e-05, 0.00011876609, 0.00019603415, 0.00031747388, 0.00050389886, 0.00078337716, 0.0011924482, 0.0017768977, 0.0025917257, 0.0036998939, 0.0051694588, 0.0070687947, 0.0094598286, 0.012389518, 0.015880203, 0.019919861, 0.024453669, 0.029378419, 0.034541319,
			0.039744329, 0.044754521, 0.049320104, 0.053190766, 0.056140158, 0.057987726, 0.058616997, 0.057987726, 0.056140158, 0.053190766, 0.049320104, 0.044754521, 0.039744329, 0.034541319, 0.029378419, 0.024453669, 0.019919861, 0.015880203, 0.012389518, 0.0094598286,
			0.0070687947, 0.0051694588, 0.0036998939, 0.0025917257, 0.0017768977, 0.0011924482, 0.00078337716, 0.00050389886, 0.00031747388, 0.00019603415, 0.00011876609, 7.0738222e-05, 4.1570429e-05 }, //Row:[136]. 53 items, sigma=13.6
		{ 2.6037216e-05, 4.4934654e-05, 7.6520545e-05, 0.00012812554, 0.00021052852, 0.00033911163, 0.00053515088, 0.00082712108, 0.0012518153, 0.0018549959, 0.002691224, 0.0038224741, 0.0053151704, 0.0072353855, 0.009642157, 0.012579181, 0.016065513, 0.020086297, 0.024584851, 0.02945759,
			0.034553217, 0.039677217, 0.04460209, 0.049082921, 0.05287699, 0.055765311, 0.057573493, 0.058189157, 0.057573493, 0.055765311, 0.05287699, 0.049082921, 0.04460209, 0.039677217, 0.034553217, 0.02945759, 0.024584851, 0.020086297, 0.016065513, 0.012579181,
			0.009642157, 0.0072353855, 0.0053151704, 0.0038224741, 0.002691224, 0.0018549959, 0.0012518153, 0.00082712108, 0.00053515088, 0.00033911163, 0.00021052852, 0.00012812554, 7.6520545e-05, 4.4934654e-05, 2.6037216e-05 }, //Row:[137]. 55 items, sigma=13.7
		{ 2.8928286e-05, 4.9513773e-05, 8.3648825e-05, 0.00013899417, 0.00022672395, 0.00036266026, 0.00056852159, 0.00087315895, 0.0013135785, 0.0019354721, 0.0027929135, 0.0039468507, 0.0054620563, 0.0074023132, 0.0098238229, 0.012767116, 0.016248101, 0.020249248, 0.02471217, 0.029533038,
			0.034562112, 0.039608382, 0.04444966, 0.04884771, 0.052567135, 0.055396038, 0.05716594, 0.057768386, 0.05716594, 0.055396038, 0.052567135, 0.04884771, 0.04444966, 0.039608382, 0.034562112, 0.029533038, 0.02471217, 0.020249248, 0.016248101, 0.012767116,
			0.0098238229, 0.0074023132, 0.0054620563, 0.0039468507, 0.0027929135, 0.0019354721, 0.0013135785, 0.00087315895, 0.00056852159, 0.00036266026, 0.00022672395, 0.00013899417, 8.3648825e-05, 4.9513773e-05, 2.8928286e-05 }, //Row:[138]. 55 items, sigma=13.8
		{ 3.20652e-05, 5.4441561e-05, 9.1258867e-05, 0.00015050848, 0.00024375538, 0.00038724999, 0.00060313256, 0.00092059953, 0.0013768301, 0.0020173996, 0.0028958493, 0.0040720644, 0.0056091501, 0.0075686157, 0.010003882, 0.01295241, 0.016427096, 0.020407885, 0.024834842, 0.029604007,
			0.034567257, 0.039537064, 0.044296439, 0.048613623, 0.052260296, 0.055031377, 0.056764067, 0.05735367, 0.056764067, 0.055031377, 0.052260296, 0.048613623, 0.044296439, 0.039537064, 0.034567257, 0.029604007, 0.024834842, 0.020407885, 0.016427096, 0.01295241,
			0.010003882, 0.0075686157, 0.0056091501, 0.0040720644, 0.0028958493, 0.0020173996, 0.0013768301, 0.00092059953, 0.00060313256, 0.00038724999, 0.00024375538, 0.00015050848, 9.1258867e-05, 5.4441561e-05, 3.20652e-05 }, //Row:[139]. 55 items, sigma=13.9
		{ 3.5461534e-05, 5.9734073e-05, 9.9368504e-05, 0.00016268665, 0.00026163902, 0.00041289191, 0.00063898594, 0.00096943202, 0.0014415431, 0.0021007333, 0.0029999686, 0.0041980387, 0.0057563699, 0.0077342165, 0.010182275, 0.013135035, 0.016602508, 0.020562264, 0.024952962, 0.02967062,
			0.034568785, 0.039463382, 0.044142508, 0.048380692, 0.051956446, 0.054671252, 0.05636776, 0.056944883, 0.05636776, 0.054671252, 0.051956446, 0.048380692, 0.044142508, 0.039463382, 0.034568785, 0.02967062, 0.024952962, 0.020562264, 0.016602508, 0.013135035,
			0.010182275, 0.0077342165, 0.0057563699, 0.0041980387, 0.0029999686, 0.0021007333, 0.0014415431, 0.00096943202, 0.00063898594, 0.00041289191, 0.00026163902, 0.00016268665, 9.9368504e-05, 5.9734073e-05, 3.5461534e-05 }, //Row:[140]. 55 items, sigma=14.0
		{ 3.9131058e-05, 6.5407385e-05, 0.00010799533, 0.00017554627, 0.00028039013, 0.00043959577, 0.00067608224, 0.0010196439, 0.0015076886, 0.0021854269, 0.0031052086, 0.0043246989, 0.0059036364, 0.0078990427, 0.010358948, 0.013314968, 0.016774353, 0.020712443, 0.025066624, 0.029732998,
			0.034566824, 0.039387451, 0.043987948, 0.048148948, 0.05165556, 0.054315584, 0.055976908, 0.056541899, 0.055976908, 0.054315584, 0.05165556, 0.048148948, 0.043987948, 0.039387451, 0.034566824, 0.029732998, 0.025066624, 0.020712443, 0.016774353, 0.013314968,
			0.010358948, 0.0078990427, 0.0059036364, 0.0043246989, 0.0031052086, 0.0021854269, 0.0015076886, 0.0010196439, 0.00067608224, 0.00043959577, 0.00028039013, 0.00017554627, 0.00010799533, 6.5407385e-05, 3.9131058e-05 }, //Row:[141]. 55 items, sigma=14.1
		{ 2.4907592e-05, 4.2181976e-05, 7.0571832e-05, 0.00011625095, 0.00018819862, 0.00029911724, 0.00046646424, 0.00071351467, 0.0010703154, 0.0015743311, 0.0022705275, 0.0032106009, 0.0044510652, 0.0060499668, 0.0080621187, 0.010532946, 0.013491282, 0.016941745, 0.020857576, 0.025175018,
			0.029790353, 0.034560592, 0.039308474, 0.043831928, 0.047917511, 0.051356706, 0.053963396, 0.055590495, 0.056143692, 0.055590495, 0.053963396, 0.051356706, 0.047917511, 0.043831928, 0.039308474, 0.034560592, 0.029790353, 0.025175018, 0.020857576, 0.016941745,
			0.013491282, 0.010532946, 0.0080621187, 0.0060499668, 0.0044510652, 0.0032106009, 0.0022705275, 0.0015743311, 0.0010703154, 0.00071351467, 0.00046646424, 0.00029911724, 0.00018819862, 0.00011625095, 7.0571832e-05, 4.2181976e-05, 2.4907592e-05 }, //Row:[142]. 57 items, sigma=14.2
		{ 2.7575761e-05, 4.6342792e-05, 7.6957866e-05, 0.00012586681, 0.00020237448, 0.00031954806, 0.00049521883, 0.00075299503, 0.0011231451, 0.0016431535, 0.002357702, 0.0033177974, 0.0045787799, 0.0061970011, 0.008225093, 0.010705935, 0.013665676, 0.017106417, 0.020999435, 0.025279952,
			0.029844516, 0.034551925, 0.039228272, 0.043676236, 0.047688123, 0.051061574, 0.053616326, 0.055210131, 0.055751859, 0.055210131, 0.053616326, 0.051061574, 0.047688123, 0.043676236, 0.039228272, 0.034551925, 0.029844516, 0.025279952, 0.020999435, 0.017106417,
			0.013665676, 0.010705935, 0.008225093, 0.0061970011, 0.0045787799, 0.0033177974, 0.002357702, 0.0016431535, 0.0011231451, 0.00075299503, 0.00049521883, 0.00031954806, 0.00020237448, 0.00012586681, 7.6957866e-05, 4.6342792e-05, 2.7575761e-05 }, //Row:[143]. 57 items, sigma=14.3
		{ 3.0463116e-05, 5.0810999e-05, 8.3764734e-05, 0.00013604288, 0.00021727283, 0.00034087815, 0.00052504862, 0.00079370253, 0.0011772992, 0.0017133069, 0.0024460853, 0.0034259194, 0.0047069552, 0.0063438506, 0.0083870857, 0.010877057, 0.01383732, 0.017267577, 0.021137267, 0.025380702,
			0.029894784, 0.034540124, 0.03914613, 0.043520123, 0.04745999, 0.050769318, 0.053273489, 0.054834897, 0.055365471, 0.054834897, 0.053273489, 0.050769318, 0.04745999, 0.043520123, 0.03914613, 0.034540124, 0.029894784, 0.025380702, 0.021137267, 0.017267577,
			0.01383732, 0.010877057, 0.0083870857, 0.0063438506, 0.0047069552, 0.0034259194, 0.0024460853, 0.0017133069, 0.0011772992, 0.00079370253, 0.00052504862, 0.00034087815, 0.00021727283, 0.00013604288, 8.3764734e-05, 5.0810999e-05, 3.0463116e-05 }, //Row:[144]. 57 items, sigma=14.4
		{ 3.358138e-05, 5.5600408e-05, 9.1007806e-05, 0.00014679499, 0.0002329082, 0.00036311826, 0.00055595758, 0.00083563113, 0.0012327588, 0.0017847573, 0.0025356284, 0.0035349049, 0.0048355212, 0.0064904453, 0.0085480368, 0.011046272, 0.014006203, 0.01742525, 0.021271131, 0.025477362,
			0.029941268, 0.034525302, 0.039062146, 0.043363656, 0.047233133, 0.050479913, 0.052934812, 0.054464693, 0.054984417, 0.054464693, 0.052934812, 0.050479913, 0.047233133, 0.043363656, 0.039062146, 0.034525302, 0.029941268, 0.025477362, 0.021271131, 0.01742525,
			0.014006203, 0.011046272, 0.0085480368, 0.0064904453, 0.0048355212, 0.0035349049, 0.0025356284, 0.0017847573, 0.0012327588, 0.00083563113, 0.00055595758, 0.00036311826, 0.0002329082, 0.00014679499, 9.1007806e-05, 5.5600408e-05, 3.358138e-05 }, //Row:[145]. 57 items, sigma=14.5
		{ 3.6942443e-05, 6.0724866e-05, 9.8702284e-05, 0.00015813854, 0.00024929437, 0.0003862781, 0.00058794839, 0.00087877338, 0.0012895035, 0.0018574696, 0.0026262818, 0.0036446924, 0.0049644096, 0.0066367179, 0.0087078894, 0.011213543, 0.014172319, 0.017579463, 0.021401089, 0.02557002,
			0.029984077, 0.034507569, 0.038976414, 0.043206897, 0.047007571, 0.050193331, 0.052600228, 0.05409942, 0.054608587, 0.05409942, 0.052600228, 0.050193331, 0.047007571, 0.043206897, 0.038976414, 0.034507569, 0.029984077, 0.02557002, 0.021401089, 0.017579463,
			0.014172319, 0.011213543, 0.0087078894, 0.0066367179, 0.0049644096, 0.0036446924, 0.0026262818, 0.0018574696, 0.0012895035, 0.00087877338, 0.00058794839, 0.0003862781, 0.00024929437, 0.00015813854, 9.8702284e-05, 6.0724866e-05, 3.6942443e-05 }, //Row:[146]. 57 items, sigma=14.6
		{ 2.3870997e-05, 3.9720754e-05, 6.536064e-05, 0.00010602559, 0.00016925087, 0.00026560677, 0.00040952873, 0.00062018486, 0.00092228286, 0.0013466742, 0.0019305707, 0.0027171583, 0.0037543835, 0.0050927161, 0.0067817658, 0.0088657521, 0.011377999, 0.014334825, 0.017729407, 0.021526368,
			0.025657931, 0.030022478, 0.034486195, 0.038888188, 0.043049066, 0.046782485, 0.049908707, 0.052268832, 0.053738147, 0.05423704, 0.053738147, 0.052268832, 0.049908707, 0.046782485, 0.043049066, 0.038888188, 0.034486195, 0.030022478, 0.025657931, 0.021526368,
			0.017729407, 0.014334825, 0.011377999, 0.0088657521, 0.0067817658, 0.0050927161, 0.0037543835, 0.0027171583, 0.0019305707, 0.0013466742, 0.00092228286, 0.00062018486, 0.00040952873, 0.00026560677, 0.00016925087, 0.00010602559, 6.536064e-05, 3.9720754e-05, 2.3870997e-05 }, //Row:[147]. 59 items, sigma=14.7
		{ 2.6341032e-05, 4.3516952e-05, 7.1110039e-05, 0.00011458096, 0.0001817349, 0.00028344608, 0.00043446619, 0.00065425559, 0.00096773788, 0.0014058364, 0.0020056123, 0.0028097967, 0.0038655066, 0.0052219642, 0.0069271151, 0.0090231626, 0.011541199, 0.014495308, 0.017876703, 0.021648618,
			0.025742771, 0.030058165, 0.034462871, 0.038799143, 0.042891811, 0.04655948, 0.049627603, 0.051942145, 0.053382368, 0.053871263, 0.053382368, 0.051942145, 0.049627603, 0.04655948, 0.042891811, 0.038799143, 0.034462871, 0.030058165, 0.025742771, 0.021648618,
			0.017876703, 0.014495308, 0.011541199, 0.0090231626, 0.0069271151, 0.0052219642, 0.0038655066, 0.0028097967, 0.0020056123, 0.0014058364, 0.00096773788, 0.00065425559, 0.00043446619, 0.00028344608, 0.0001817349, 0.00011458096, 7.1110039e-05, 4.3516952e-05, 2.6341032e-05 }, //Row:[148]. 59 items, sigma=14.8
		{ 2.900731e-05, 4.7585477e-05, 7.7229024e-05, 0.0001236251, 0.00019484672, 0.00030206592, 0.00046033918, 0.00068940163, 0.0010143692, 0.0014662087, 0.0020817995, 0.0029033894, 0.0039772446, 0.0053513326, 0.0070719481, 0.0091793153, 0.011702359, 0.014653013, 0.018020625, 0.021767145,
			0.025823871, 0.030090481, 0.034436942, 0.038708606, 0.042734426, 0.046337811, 0.049349233, 0.051619346, 0.053031237, 0.053510398, 0.053031237, 0.051619346, 0.049349233, 0.046337811, 0.042734426, 0.038708606, 0.034436942, 0.030090481, 0.025823871, 0.021767145,
			0.018020625, 0.014653013, 0.011702359, 0.0091793153, 0.0070719481, 0.0053513326, 0.0039772446, 0.0029033894, 0.0020817995, 0.0014662087, 0.0010143692, 0.00068940163, 0.00046033918, 0.00030206592, 0.00019484672, 0.0001236251, 7.7229024e-05, 4.7585477e-05, 2.900731e-05 }, //Row:[149]. 59 items, sigma=14.9
		{ 3.1880008e-05, 5.1938277e-05, 8.3730907e-05, 0.00013317183, 0.00020859932, 0.00032147649, 0.00048715275, 0.00072562026, 0.0010621638, 0.0015277657, 0.0021590942, 0.0029978863, 0.0040895388, 0.0054807595, 0.0072162069, 0.009334165, 0.011861453, 0.01480794, 0.018161206, 0.021882013,
			0.025901317, 0.030119525, 0.034408503, 0.038616657, 0.042576963, 0.046117493, 0.049073569, 0.05130037, 0.052684665, 0.053154349, 0.052684665, 0.05130037, 0.049073569, 0.046117493, 0.042576963, 0.038616657, 0.034408503, 0.030119525, 0.025901317, 0.021882013,
			0.018161206, 0.01480794, 0.011861453, 0.009334165, 0.0072162069, 0.0054807595, 0.0040895388, 0.0029978863, 0.0021590942, 0.0015277657, 0.0010621638, 0.00072562026, 0.00048715275, 0.00032147649, 0.00020859932, 0.00013317183, 8.3730907e-05, 5.1938277e-05, 3.1880008e-05 }, //Row:[150]. 59 items, sigma=15.0
		{ 3.4969442e-05, 5.6587336e-05, 9.0628874e-05, 0.00014323465, 0.00022300507, 0.00034168715, 0.00051491092, 0.00076290758, 0.0011111075, 0.0015904813, 0.0022374576, 0.0030932375, 0.0042023314, 0.0056101846, 0.0073598361, 0.0094876694, 0.012018461, 0.014960098, 0.018298483, 0.021993284,
			0.025975195, 0.030145393, 0.03437765, 0.038523375, 0.042419471, 0.045898536, 0.048800584, 0.050985157, 0.052342566, 0.05280302, 0.052342566, 0.050985157, 0.048800584, 0.045898536, 0.042419471, 0.038523375, 0.03437765, 0.030145393, 0.025975195, 0.021993284,
			0.018298483, 0.014960098, 0.012018461, 0.0094876694, 0.0073598361, 0.0056101846, 0.0042023314, 0.0030932375, 0.0022374576, 0.0015904813, 0.0011111075, 0.00076290758, 0.00051491092, 0.00034168715, 0.00022300507, 0.00014323465, 9.0628874e-05, 5.6587336e-05, 3.4969442e-05 }, //Row:[151]. 59 items, sigma=15.1
		{ 2.2916481e-05, 3.7509221e-05, 6.076782e-05, 9.7159129e-05, 0.00015304984, 0.00023729893, 0.0003619296, 0.00054283977, 0.00080048167, 0.0011604082, 0.0016535515, 0.0023160738, 0.0031886166, 0.0043147888, 0.0057387726, 0.007502006, 0.0096390119, 0.012172587, 0.015108715, 0.018431715,
			0.022100245, 0.026044812, 0.030167404, 0.034343697, 0.038428057, 0.042261219, 0.045680174, 0.048529474, 0.050672867, 0.052004081, 0.052455545, 0.052004081, 0.050672867, 0.048529474, 0.045680174, 0.042261219, 0.038428057, 0.034343697, 0.030167404, 0.026044812,
			0.022100245, 0.018431715, 0.015108715, 0.012172587, 0.0096390119, 0.007502006, 0.0057387726, 0.0043147888, 0.0031886166, 0.0023160738, 0.0016535515, 0.0011604082, 0.00080048167, 0.00054283977, 0.0003619296, 0.00023729893, 0.00015304984, 9.7159129e-05, 6.076782e-05, 3.7509221e-05,
			2.2916481e-05 }, //Row:[152]. 61 items, sigma=15.2
		{ 2.5209619e-05, 4.0985814e-05, 6.5967634e-05, 0.00010481045, 0.00016410608, 0.00025296798, 0.0003836874, 0.00057241714, 0.00083981226, 0.0012115253, 0.0017184246, 0.0023963794, 0.0032854501, 0.0044283312, 0.0058679429, 0.0076441419, 0.0097896316, 0.01232529, 0.015255277, 0.018562415,
			0.022204436, 0.026111727, 0.030187124, 0.034308208, 0.038332252, 0.042103728, 0.045463892, 0.048261686, 0.050364918, 0.051670602, 0.05211331, 0.051670602, 0.050364918, 0.048261686, 0.045463892, 0.042103728, 0.038332252, 0.034308208, 0.030187124, 0.026111727,
			0.022204436, 0.018562415, 0.015255277, 0.01232529, 0.0097896316, 0.0076441419, 0.0058679429, 0.0044283312, 0.0032854501, 0.0023963794, 0.0017184246, 0.0012115253, 0.00083981226, 0.00057241714, 0.0003836874, 0.00025296798, 0.00016410608, 0.00010481045, 6.5967634e-05, 4.0985814e-05,
			2.5209619e-05 }, //Row:[153]. 61 items, sigma=15.3
		{ 2.7679149e-05, 4.4704769e-05, 7.1493646e-05, 0.00011289042, 0.00017571061, 0.00026931765, 0.00040626225, 0.00060293882, 0.00088018698, 0.0012637366, 0.0017843667, 0.0024776297, 0.0033829841, 0.0045421989, 0.0059969356, 0.0077854904, 0.009938789, 0.012475852, 0.015399092, 0.018689916,
			0.022305215, 0.026175315, 0.03020394, 0.034270565, 0.038235323, 0.041946333, 0.045248993, 0.047996487, 0.050060545, 0.051341347, 0.051775522, 0.051341347, 0.050060545, 0.047996487, 0.045248993, 0.041946333, 0.038235323, 0.034270565, 0.03020394, 0.026175315,
			0.022305215, 0.018689916, 0.015399092, 0.012475852, 0.009938789, 0.0077854904, 0.0059969356, 0.0045421989, 0.0033829841, 0.0024776297, 0.0017843667, 0.0012637366, 0.00088018698, 0.00060293882, 0.00040626225, 0.00026931765, 0.00017571061, 0.00011289042, 7.1493646e-05, 4.4704769e-05,
			2.7679149e-05 }, //Row:[154]. 61 items, sigma=15.4
		{ 3.0333944e-05, 4.867647e-05, 7.7357435e-05, 0.00012141114, 0.000187875, 0.00028635749, 0.00042965979, 0.00063440434, 0.00092159718, 0.0013170233, 0.0018513485, 0.0025597846, 0.0034811697, 0.0046563382, 0.0061256973, 0.0079260054, 0.010086452, 0.012624261, 0.015540172, 0.018814259,
			0.022402645, 0.026235659, 0.030217939, 0.03423085, 0.038137337, 0.041789076, 0.045035483, 0.047733852, 0.049759692, 0.051016236, 0.051442096, 0.051016236, 0.049759692, 0.047733852, 0.045035483, 0.041789076, 0.038137337, 0.03423085, 0.030217939, 0.026235659,
			0.022402645, 0.018814259, 0.015540172, 0.012624261, 0.010086452, 0.0079260054, 0.0061256973, 0.0046563382, 0.0034811697, 0.0025597846, 0.0018513485, 0.0013170233, 0.00092159718, 0.00063440434, 0.00042965979, 0.00028635749, 0.000187875, 0.00012141114, 7.7357435e-05, 4.867647e-05,
			3.0333944e-05 }, //Row:[155]. 61 items, sigma=15.5
		{ 3.3183001e-05, 5.2911341e-05, 8.3570487e-05, 0.00013038447, 0.00020061039, 0.00030409636, 0.00045388475, 0.00066681227, 0.0009640332, 0.001371366, 0.0019193399, 0.0026428038, 0.0035799586, 0.0047706965, 0.0062541766, 0.0080656428, 0.010232591, 0.012770508, 0.015678531, 0.018935482,
			0.022496789, 0.026292835, 0.030229205, 0.034189144, 0.038038358, 0.041631994, 0.044823369, 0.04747375, 0.049462301, 0.050695195, 0.051112951, 0.050695195, 0.049462301, 0.04747375, 0.044823369, 0.041631994, 0.038038358, 0.034189144, 0.030229205, 0.026292835,
			0.022496789, 0.018935482, 0.015678531, 0.012770508, 0.010232591, 0.0080656428, 0.0062541766, 0.0047706965, 0.0035799586, 0.0026428038, 0.0019193399, 0.001371366, 0.0009640332, 0.00066681227, 0.00045388475, 0.00030409636, 0.00020061039, 0.00013038447, 8.3570487e-05, 5.2911341e-05,
			3.3183001e-05 }, //Row:[156]. 61 items, sigma=15.6
		{ 2.2034739e-05, 3.5512972e-05, 5.669737e-05, 8.9421727e-05, 0.00013909952, 0.00021320494, 0.00032181998, 0.00047821857, 0.00069943774, 0.001006762, 0.0014260219, 0.0019875878, 0.0027259247, 0.0036785808, 0.0048845001, 0.0063816013, 0.0082036386, 0.010376456, 0.012913862, 0.015813465,
			0.019052905, 0.022586986, 0.0263462, 0.030237101, 0.034144803, 0.037937726, 0.0414744, 0.044611931, 0.047215434, 0.049167597, 0.050377427, 0.050787283, 0.050377427, 0.049167597, 0.047215434, 0.044611931, 0.0414744, 0.037937726, 0.034144803, 0.030237101,
			0.0263462, 0.022586986, 0.019052905, 0.015813465, 0.012913862, 0.010376456, 0.0082036386, 0.0063816013, 0.0048845001, 0.0036785808, 0.0027259247, 0.0019875878, 0.0014260219, 0.001006762, 0.00069943774, 0.00047821857, 0.00032181998, 0.00021320494, 0.00013909952, 8.9421727e-05,
			5.669737e-05, 3.5512972e-05, 2.2034739e-05 }, //Row:[157]. 63 items, sigma=15.7
		{ 2.4169292e-05, 3.8707965e-05, 6.1419907e-05, 9.6297305e-05, 0.00014894247, 0.00022704371, 0.00034091078, 0.00050403919, 0.00073365228, 0.0010511468, 0.0014823449, 0.002057436, 0.002810482, 0.0037783641, 0.0049990737, 0.0065092986, 0.0083413281, 0.010519397, 0.013055694, 0.015946365,
			0.019167944, 0.022674673, 0.026397205, 0.030243081, 0.034099277, 0.037836874, 0.041317704, 0.04440255, 0.046960251, 0.048876898, 0.050064235, 0.050466389, 0.050064235, 0.048876898, 0.046960251, 0.04440255, 0.041317704, 0.037836874, 0.034099277, 0.030243081,
			0.026397205, 0.022674673, 0.019167944, 0.015946365, 0.013055694, 0.010519397, 0.0083413281, 0.0065092986, 0.0049990737, 0.0037783641, 0.002810482, 0.002057436, 0.0014823449, 0.0010511468, 0.00073365228, 0.00050403919, 0.00034091078, 0.00022704371, 0.00014894247, 9.6297305e-05,
			6.1419907e-05, 3.8707965e-05, 2.4169292e-05 }, //Row:[158]. 63 items, sigma=15.8
		{ 2.6462972e-05, 4.2119563e-05, 6.6431683e-05, 0.00010355062, 0.00015926658, 0.00024147864, 0.00036071784, 0.0005306911, 0.00076879393, 0.0010965176, 0.0015396556, 0.0021281956, 0.002895778, 0.0038786044, 0.005113711, 0.0066365646, 0.0084780161, 0.010660734, 0.013195342, 0.016076594,
			0.019279983, 0.022759255, 0.026445266, 0.030246569, 0.034051982, 0.037735202, 0.04116128, 0.04419457, 0.046707515, 0.048589496, 0.049754891, 0.050149535, 0.049754891, 0.048589496, 0.046707515, 0.04419457, 0.04116128, 0.037735202, 0.034051982, 0.030246569,
			0.026445266, 0.022759255, 0.019279983, 0.016076594, 0.013195342, 0.010660734, 0.0084780161, 0.0066365646, 0.005113711, 0.0038786044, 0.002895778, 0.0021281956, 0.0015396556, 0.0010965176, 0.00076879393, 0.0005306911, 0.00036071784, 0.00024147864, 0.00015926658, 0.00010355062,
			6.6431683e-05, 4.2119563e-05, 2.6462972e-05 }, //Row:[159]. 63 items, sigma=15.9
		{ 2.8923552e-05, 4.5756836e-05, 7.1742811e-05, 0.00011119231, 0.00017008217, 0.00025651856, 0.00038124699, 0.00055817539, 0.00080485722, 0.0011428608, 0.0015979312, 0.0021998346, 0.0029817725, 0.0039792559, 0.0052283641, 0.0067633548, 0.0086136672, 0.010800445, 0.013332805, 0.01620417,
			0.019389064, 0.022840792, 0.026490458, 0.03024764, 0.034002988, 0.037632764, 0.041005158, 0.043987993, 0.0464572, 0.048305337, 0.049449326, 0.049836645, 0.049449326, 0.048305337, 0.0464572, 0.043987993, 0.041005158, 0.037632764, 0.034002988, 0.03024764,
			0.026490458, 0.022840792, 0.019389064, 0.01620417, 0.013332805, 0.010800445, 0.0086136672, 0.0067633548, 0.0052283641, 0.0039792559, 0.0029817725, 0.0021998346, 0.0015979312, 0.0011428608, 0.00080485722, 0.00055817539, 0.00038124699, 0.00025651856, 0.00017008217, 0.00011119231,
			7.1742811e-05, 4.5756836e-05, 2.8923552e-05 }, //Row:[160]. 63 items, sigma=16.0
		{ 3.1558914e-05, 4.9628887e-05, 7.7363339e-05, 0.00011923282, 0.00018139922, 0.00027217177, 0.00040250336, 0.0005864923, 0.00084183584, 0.0011901621, 0.0016571484, 0.0022723208, 0.0030684254, 0.0040802733, 0.0053429866, 0.0068896265, 0.0087482478, 0.010938512, 0.013468083, 0.016329117,
			0.019495229, 0.022919345, 0.026532851, 0.03024637, 0.033952365, 0.037529614, 0.040849369, 0.043782821, 0.04620928, 0.048024372, 0.049147473, 0.049527648, 0.049147473, 0.048024372, 0.04620928, 0.043782821, 0.040849369, 0.037529614, 0.033952365, 0.03024637,
			0.026532851, 0.022919345, 0.019495229, 0.016329117, 0.013468083, 0.010938512, 0.0087482478, 0.0068896265, 0.0053429866, 0.0040802733, 0.0030684254, 0.0022723208, 0.0016571484, 0.0011901621, 0.00084183584, 0.0005864923, 0.00040250336, 0.00027217177, 0.00018139922, 0.00011923282,
			7.7363339e-05, 4.9628887e-05, 3.1558914e-05 }, //Row:[161]. 63 items, sigma=16.1
		{ 2.1217811e-05, 3.3703447e-05, 5.3071261e-05, 8.2629647e-05, 0.00012700878, 0.00019255372, 0.00028777241, 0.00042381776, 0.00061496768, 0.00087904905, 0.0012377327, 0.0017166099, 0.0023449481, 0.0031550237, 0.0041809387, 0.0054568597, 0.0070146651, 0.0088810527, 0.011074244, 0.013600503,
			0.016450783, 0.019597848, 0.022994299, 0.026571844, 0.030242158, 0.033899504, 0.037425127, 0.040693266, 0.04357838, 0.045963052, 0.047745876, 0.048848594, 0.049221798, 0.048848594, 0.047745876, 0.045963052, 0.04357838, 0.040693266, 0.037425127, 0.033899504,
			0.030242158, 0.026571844, 0.022994299, 0.019597848, 0.016450783, 0.013600503, 0.011074244, 0.0088810527, 0.0070146651, 0.0054568597, 0.0041809387, 0.0031550237, 0.0023449481, 0.0017166099, 0.0012377327, 0.00087904905, 0.00061496768, 0.00042381776, 0.00028777241, 0.00019255372,
			0.00012700878, 8.2629647e-05, 5.3071261e-05, 3.3703447e-05, 2.1217811e-05 }, //Row:[162]. 65 items, sigma=16.2
		{ 2.3209652e-05, 3.6649133e-05, 5.7377015e-05, 8.8835523e-05, 0.0001358141, 0.00020483878, 0.0003046116, 0.00044647784, 0.00064488412, 0.00091777279, 0.0012868408, 0.0017775753, 0.002418968, 0.0032428119, 0.0042824926, 0.0055712237, 0.0071397151, 0.0090133364, 0.01120891, 0.013731355,
			0.016570474, 0.019698249, 0.023066998, 0.026608789, 0.030236361, 0.033845754, 0.037320638, 0.040538159, 0.043375955, 0.045719777, 0.047471084, 0.048553908, 0.04892031, 0.048553908, 0.047471084, 0.045719777, 0.043375955, 0.040538159, 0.037320638, 0.033845754,
			0.030236361, 0.026608789, 0.023066998, 0.019698249, 0.016570474, 0.013731355, 0.01120891, 0.0090133364, 0.0071397151, 0.0055712237, 0.0042824926, 0.0032428119, 0.002418968, 0.0017775753, 0.0012868408, 0.00091777279, 0.00064488412, 0.00044647784, 0.0003046116, 0.00020483878,
			0.0001358141, 8.8835523e-05, 5.7377015e-05, 3.6649133e-05, 2.3209652e-05 }, //Row:[163]. 65 items, sigma=16.3
		{ 2.5345535e-05, 3.9789168e-05, 6.1940345e-05, 9.5375783e-05, 0.00014504357, 0.00021764825, 0.00032208101, 0.00046987168, 0.00067562452, 0.00095738339, 0.0013368549, 0.0018394048, 0.002493733, 0.003331136, 0.0043842775, 0.0056854212, 0.0072641242, 0.0091444562, 0.011341882, 0.013860026,
			0.016687602, 0.019795859, 0.023136886, 0.02664314, 0.030228432, 0.033790563, 0.037215577, 0.040383458, 0.043174929, 0.045478811, 0.047199335, 0.048262738, 0.048622503, 0.048262738, 0.047199335, 0.045478811, 0.043174929, 0.040383458, 0.037215577, 0.033790563,
			0.030228432, 0.02664314, 0.023136886, 0.019795859, 0.016687602, 0.013860026, 0.011341882, 0.0091444562, 0.0072641242, 0.0056854212, 0.0043842775, 0.003331136, 0.002493733, 0.0018394048, 0.0013368549, 0.00095738339, 0.00067562452, 0.00046987168, 0.00032208101, 0.00021764825,
			0.00014504357, 9.5375783e-05, 6.1940345e-05, 3.9789168e-05, 2.5345535e-05 }, //Row:[164]. 65 items, sigma=16.4
		{ 2.7632298e-05, 4.3131505e-05, 6.6770121e-05, 0.0001022598, 0.00015470642, 0.00023099026, 0.00034018646, 0.00049400135, 0.00070718578, 0.00099787113, 0.0013877578, 0.0019020733, 0.0025692102, 0.0034199573, 0.0044862513, 0.0057994108, 0.0073878562, 0.0092743859, 0.011473149, 0.013986523,
			0.016802191, 0.019890721, 0.023204021, 0.026674962, 0.030218439, 0.033733991, 0.037109989, 0.040229186, 0.042975301, 0.045240129, 0.04693058, 0.047975022, 0.04832831, 0.047975022, 0.04693058, 0.045240129, 0.042975301, 0.040229186, 0.037109989, 0.033733991,
			0.030218439, 0.026674962, 0.023204021, 0.019890721, 0.016802191, 0.013986523, 0.011473149, 0.0092743859, 0.0073878562, 0.0057994108, 0.0044862513, 0.0034199573, 0.0025692102, 0.0019020733, 0.0013877578, 0.00099787113, 0.00070718578, 0.00049400135, 0.00034018646, 0.00023099026,
			0.00015470642, 0.0001022598, 6.6770121e-05, 4.3131505e-05, 2.7632298e-05 }, //Row:[165]. 65 items, sigma=16.5
		{ 3.0076873e-05, 4.6684132e-05, 7.1875164e-05, 0.00010949681, 0.00016481158, 0.00024487248, 0.00035893319, 0.00051886826, 0.00073956401, 0.0010392256, 0.0014395314, 0.0019655549, 0.0026453668, 0.0035092374, 0.0045883731, 0.0059131526, 0.0075108765, 0.0094031014, 0.011602699, 0.014110855,
			0.016914267, 0.01998288, 0.02326846, 0.026704323, 0.030206447, 0.033676096, 0.037003919, 0.040075367, 0.04277707, 0.045003705, 0.046664774, 0.047690702, 0.048037667, 0.047690702, 0.046664774, 0.045003705, 0.04277707, 0.040075367, 0.037003919, 0.033676096,
			0.030206447, 0.026704323, 0.02326846, 0.01998288, 0.016914267, 0.014110855, 0.011602699, 0.0094031014, 0.0075108765, 0.0059131526, 0.0045883731, 0.0035092374, 0.0026453668, 0.0019655549, 0.0014395314, 0.0010392256, 0.00073956401, 0.00051886826, 0.00035893319, 0.00024487248,
			0.00016481158, 0.00010949681, 7.1875164e-05, 4.6684132e-05, 3.0076873e-05 }, //Row:[166]. 65 items, sigma=16.6
		{ 3.2686272e-05, 5.0455058e-05, 7.7264231e-05, 0.00011709584, 0.00017536767, 0.00025930215, 0.00037832583, 0.00054447313, 0.00077275461, 0.0010814356, 0.0014921574, 0.0020298237, 0.0027221699, 0.0035989386, 0.0046906027, 0.0060266079, 0.0076331525, 0.00953058, 0.011730526, 0.014233028,
			0.017023856, 0.020072378, 0.023330259, 0.026731284, 0.030192522, 0.033616934, 0.036897406, 0.039922019, 0.042580233, 0.044769513, 0.046401872, 0.047409719, 0.04775051, 0.047409719, 0.046401872, 0.044769513, 0.042580233, 0.039922019, 0.036897406, 0.033616934,
			0.030192522, 0.026731284, 0.023330259, 0.020072378, 0.017023856, 0.014233028, 0.011730526, 0.00953058, 0.0076331525, 0.0060266079, 0.0046906027, 0.0035989386, 0.0027221699, 0.0020298237, 0.0014921574, 0.0010814356, 0.00077275461, 0.00054447313, 0.00037832583, 0.00025930215,
			0.00017536767, 0.00011709584, 7.7264231e-05, 5.0455058e-05, 3.2686272e-05 }, //Row:[167]. 65 items, sigma=16.7
		{ 2.2321807e-05, 3.4780752e-05, 5.3765476e-05, 8.2259176e-05, 0.00012437892, 0.00018569617, 0.00027359924, 0.00039768164, 0.00057012917, 0.00080606547, 0.0011238026, 0.0015449299, 0.0020941667, 0.0027989002, 0.0036883367, 0.0047922142, 0.0061390526, 0.007753966, 0.009656114, 0.011855936,
			0.014352367, 0.0171303, 0.020158573, 0.023388788, 0.026755223, 0.030176038, 0.033555874, 0.036789805, 0.039768478, 0.042384099, 0.044536842, 0.046141142, 0.04713133, 0.047466092, 0.04713133, 0.046141142, 0.044536842, 0.042384099, 0.039768478, 0.036789805,
			0.033555874, 0.030176038, 0.026755223, 0.023388788, 0.020158573, 0.0171303, 0.014352367, 0.011855936, 0.009656114, 0.007753966, 0.0061390526, 0.0047922142, 0.0036883367, 0.0027989002, 0.0020941667, 0.0015449299, 0.0011238026, 0.00080606547, 0.00057012917, 0.00039768164,
			0.00027359924, 0.00018569617, 0.00012437892, 8.2259176e-05, 5.3765476e-05, 3.4780752e-05, 2.2321807e-05 }, //Row:[168]. 67 items, sigma=16.8
		{ 2.4315565e-05, 3.7679756e-05, 5.7935703e-05, 8.8180888e-05, 0.00013266701, 0.00019711734, 0.00028908234, 0.00041831641, 0.00059714805, 0.00084080289, 0.0011676264, 0.0015991418, 0.0021598699, 0.0028768374, 0.0037787073, 0.0048944817, 0.0062517631, 0.0078745996, 0.0097809964, 0.011980234,
			0.014470194, 0.017234937, 0.020242819, 0.023445413, 0.026777512, 0.030158369, 0.03349428, 0.036682466, 0.039616073, 0.042189979, 0.044306978, 0.045883853, 0.04685679, 0.047185666, 0.04685679, 0.045883853, 0.044306978, 0.042189979, 0.039616073, 0.036682466,
			0.03349428, 0.030158369, 0.026777512, 0.023445413, 0.020242819, 0.017234937, 0.014470194, 0.011980234, 0.0097809964, 0.0078745996, 0.0062517631, 0.0048944817, 0.0037787073, 0.0028768374, 0.0021598699, 0.0015991418, 0.0011676264, 0.00084080289, 0.00059714805, 0.00041831641,
			0.00028908234, 0.00019711734, 0.00013266701, 8.8180888e-05, 5.7935703e-05, 3.7679756e-05, 2.4315565e-05 }, //Row:[169]. 67 items, sigma=16.9
		{ 2.6446166e-05, 4.0760784e-05, 6.2344044e-05, 9.4408152e-05, 0.00014133884, 0.00020900913, 0.00030512762, 0.00043960336, 0.00062489884, 0.00087633056, 0.0012122645, 0.0016541437, 0.0022262769, 0.0029553196, 0.0038693845, 0.0049967383, 0.0063640748, 0.0079943955, 0.0099045799, 0.012102789,
			0.014585892, 0.017337168, 0.02032453, 0.02349956, 0.026797583, 0.030138944, 0.033431574, 0.036574796, 0.039464193, 0.041997237, 0.044079267, 0.045629334, 0.046585417, 0.046908543, 0.046585417, 0.045629334, 0.044079267, 0.041997237, 0.039464193, 0.036574796,
			0.033431574, 0.030138944, 0.026797583, 0.02349956, 0.02032453, 0.017337168, 0.014585892, 0.012102789, 0.0099045799, 0.0079943955, 0.0063640748, 0.0049967383, 0.0038693845, 0.0029553196, 0.0022262769, 0.0016541437, 0.0012122645, 0.00087633056, 0.00062489884, 0.00043960336,
			0.00030512762, 0.00020900913, 0.00014133884, 9.4408152e-05, 6.2344044e-05, 4.0760784e-05, 2.6446166e-05 }, //Row:[170]. 67 items, sigma=17.0
		{ 2.8719732e-05, 4.4030871e-05, 6.6998271e-05, 0.00010094913, 0.00015040242, 0.00022137862, 0.00032174027, 0.00046154464, 0.00065337944, 0.00091264101, 0.0012577032, 0.0017099151, 0.0022933609, 0.0030343145, 0.0039603326, 0.0050989476, 0.0064759544, 0.0081133268, 0.010026848, 0.012223596,
			0.014699475, 0.017437021, 0.020403749, 0.023551282, 0.026815494, 0.030117821, 0.033367805, 0.03646683, 0.039312854, 0.041805871, 0.043853684, 0.045377541, 0.046317157, 0.046634666, 0.046317157, 0.045377541, 0.043853684, 0.041805871, 0.039312854, 0.03646683,
			0.033367805, 0.030117821, 0.026815494, 0.023551282, 0.020403749, 0.017437021, 0.014699475, 0.012223596, 0.010026848, 0.0081133268, 0.0064759544, 0.0050989476, 0.0039603326, 0.0030343145, 0.0022933609, 0.0017099151, 0.0012577032, 0.00091264101, 0.00065337944, 0.00046154464,
			0.00032174027, 0.00022137862, 0.00015040242, 0.00010094913, 6.6998271e-05, 4.4030871e-05, 2.8719732e-05 }, //Row:[171]. 67 items, sigma=17.1
		{ 3.1142454e-05, 4.7497075e-05, 7.1906112e-05, 0.00010781186, 0.00015986551, 0.00023423251, 0.00033892498, 0.00048414182, 0.00068258713, 0.00094972619, 0.0013039281, 0.0017664353, 0.002361095, 0.0031137901, 0.0040515166, 0.0052010743, 0.0065873696, 0.0082313683, 0.010147786, 0.012342655,
			0.014810954, 0.017534524, 0.020480519, 0.023600632, 0.026831302, 0.030095055, 0.033303022, 0.036358603, 0.039162072, 0.041615875, 0.043630204, 0.045128435, 0.046051958, 0.04636398, 0.046051958, 0.045128435, 0.043630204, 0.041615875, 0.039162072, 0.036358603,
			0.033303022, 0.030095055, 0.026831302, 0.023600632, 0.020480519, 0.017534524, 0.014810954, 0.012342655, 0.010147786, 0.0082313683, 0.0065873696, 0.0052010743, 0.0040515166, 0.0031137901, 0.002361095, 0.0017664353, 0.0013039281, 0.00094972619, 0.00068258713, 0.00048414182,
			0.00033892498, 0.00023423251, 0.00015986551, 0.00010781186, 7.1906112e-05, 4.7497075e-05, 3.1142454e-05 }, //Row:[172]. 67 items, sigma=17.2
		{ 2.1498108e-05, 3.3078849e-05, 5.0524733e-05, 7.6433497e-05, 0.00011436246, 0.00016909388, 0.00024693543, 0.00035604423, 0.00050675418, 0.00071187688, 0.00098693571, 0.0013502829, 0.0018230417, 0.0024288106, 0.003193073, 0.0041422602, 0.0053024422, 0.0066976477, 0.0083478542, 0.010266739,
			0.012459324, 0.014919704, 0.017629066, 0.02055424, 0.023647019, 0.026844422, 0.030070059, 0.033236629, 0.036249505, 0.039011221, 0.041426603, 0.043408162, 0.044881334, 0.045789128, 0.046095788, 0.045789128, 0.044881334, 0.043408162, 0.041426603, 0.039011221,
			0.036249505, 0.033236629, 0.030070059, 0.026844422, 0.023647019, 0.02055424, 0.017629066, 0.014919704, 0.012459324, 0.010266739, 0.0083478542, 0.0066976477, 0.0053024422, 0.0041422602, 0.003193073, 0.0024288106, 0.0018230417, 0.0013502829, 0.00098693571, 0.00071187688,
			0.00050675418, 0.00035604423, 0.00024693543, 0.00016909388, 0.00011436246, 7.6433497e-05, 5.0524733e-05, 3.3078849e-05, 2.1498108e-05 }, //Row:[173]. 69 items, sigma=17.3
		{ 2.3363413e-05, 3.5763013e-05, 5.4348701e-05, 8.1815809e-05, 0.00012183645, 0.00017932253, 0.00026072111, 0.00037432956, 0.00053060997, 0.00074247255, 0.0010254881, 0.0013979803, 0.0018809406, 0.0024977086, 0.0032733597, 0.0042337576, 0.0054042458, 0.0068079866, 0.0084639897, 0.010384922,
			0.012574831, 0.015026968, 0.017721903, 0.020626183, 0.023691724, 0.026856137, 0.030044114, 0.033169899, 0.036140795, 0.038861541, 0.041239277, 0.043188762, 0.044637425, 0.045529845, 0.045831265, 0.045529845, 0.044637425, 0.043188762, 0.041239277, 0.038861541,
			0.036140795, 0.033169899, 0.030044114, 0.026856137, 0.023691724, 0.020626183, 0.017721903, 0.015026968, 0.012574831, 0.010384922, 0.0084639897, 0.0068079866, 0.0054042458, 0.0042337576, 0.0032733597, 0.0024977086, 0.0018809406, 0.0013979803, 0.0010254881, 0.00074247255,
			0.00053060997, 0.00037432956, 0.00026072111, 0.00017932253, 0.00012183645, 8.1815809e-05, 5.4348701e-05, 3.5763013e-05, 2.3363413e-05 }, //Row:[174]. 69 items, sigma=17.4
		{ 2.535321e-05, 3.8611525e-05, 5.8386268e-05, 8.7470784e-05, 0.00012965158, 0.00018996866, 0.00027500538, 0.00039319446, 0.00055511931, 0.00077377992, 0.001064784, 0.0014464147, 0.0019395208, 0.0025671722, 0.0033540294, 0.0043253859, 0.005505863, 0.0069177683, 0.008579164, 0.010501735,
			0.012688589, 0.015132172, 0.017812478, 0.020695801, 0.023734206, 0.026865911, 0.030016683, 0.033102285, 0.036031915, 0.038712456, 0.041053301, 0.04297139, 0.044396081, 0.04527347, 0.04556977, 0.04527347, 0.044396081, 0.04297139, 0.041053301, 0.038712456,
			0.036031915, 0.033102285, 0.030016683, 0.026865911, 0.023734206, 0.020695801, 0.017812478, 0.015132172, 0.012688589, 0.010501735, 0.008579164, 0.0069177683, 0.005505863, 0.0043253859, 0.0033540294, 0.0025671722, 0.0019395208, 0.0014464147, 0.001064784, 0.00077377992,
			0.00055511931, 0.00039319446, 0.00027500538, 0.00018996866, 0.00012965158, 8.7470784e-05, 5.8386268e-05, 3.8611525e-05, 2.535321e-05 }, //Row:[175]. 69 items, sigma=17.5
		{ 2.7472924e-05, 4.1630604e-05, 6.2644308e-05, 9.3405674e-05, 0.00013781503, 0.00020103877, 0.00028979324, 0.00041264154, 0.00058028141, 0.00080579379, 0.0011048129, 0.0014955703, 0.0019987605, 0.0026371745, 0.0034350513, 0.0044171126, 0.0056072621, 0.0070269653, 0.0086933569, 0.010617168,
			0.012800601, 0.015235333, 0.017900819, 0.020763133, 0.023774515, 0.026873796, 0.029987814, 0.03303383, 0.035922891, 0.038563978, 0.04086867, 0.042756022, 0.044157263, 0.045019957, 0.045311251, 0.045019957, 0.044157263, 0.042756022, 0.04086867, 0.038563978,
			0.035922891, 0.03303383, 0.029987814, 0.026873796, 0.023774515, 0.020763133, 0.017900819, 0.015235333, 0.012800601, 0.010617168, 0.0086933569, 0.0070269653, 0.0056072621, 0.0044171126, 0.0034350513, 0.0026371745, 0.0019987605, 0.0014955703, 0.0011048129, 0.00080579379,
			0.00058028141, 0.00041264154, 0.00028979324, 0.00020103877, 0.00013781503, 9.3405674e-05, 6.2644308e-05, 4.1630604e-05, 2.7472924e-05 }, //Row:[176]. 69 items, sigma=17.6
		{ 2.9728043e-05, 4.4826494e-05, 6.7129661e-05, 9.9627616e-05, 0.0001463338, 0.00021253904, 0.00030508933, 0.00043267295, 0.00060609494, 0.00083850843, 0.0011455639, 0.0015454304, 0.0020586378, 0.0027076887, 0.0035163952, 0.004508906, 0.0057084128, 0.0071355515, 0.0088065499, 0.010731212,
			0.012910871, 0.015336467, 0.017986957, 0.020828223, 0.0238127, 0.026879845, 0.029957557, 0.032964573, 0.035813754, 0.038416119, 0.040685378, 0.042542635, 0.043920935, 0.044769258, 0.045055658, 0.044769258, 0.043920935, 0.042542635, 0.040685378, 0.038416119,
			0.035813754, 0.032964573, 0.029957557, 0.026879845, 0.0238127, 0.020828223, 0.017986957, 0.015336467, 0.012910871, 0.010731212, 0.0088065499, 0.0071355515, 0.0057084128, 0.004508906, 0.0035163952, 0.0027076887, 0.0020586378, 0.0015454304, 0.0011455639, 0.00083850843,
			0.00060609494, 0.00043267295, 0.00030508933, 0.00021253904, 0.0001463338, 9.9627616e-05, 6.7129661e-05, 4.4826494e-05, 2.9728043e-05 }, //Row:[177]. 69 items, sigma=17.7
		{ 2.0731945e-05, 3.1523182e-05, 4.7604525e-05, 7.1248192e-05, 0.0001055427, 0.00015461373, 0.00022387446, 0.00032029695, 0.00045268941, 0.00063195714, 0.00087131668, 0.0011864248, 0.0015953774, 0.0021185295, 0.0027780872, 0.0035974302, 0.0046001342, 0.0058086845, 0.0072429008, 0.0089181244,
			0.010843261, 0.013018802, 0.015434994, 0.018070321, 0.02089051, 0.023848208, 0.026883505, 0.029925358, 0.032893954, 0.035703927, 0.038268289, 0.040502816, 0.042330607, 0.043686459, 0.044520728, 0.044802343, 0.044520728, 0.043686459, 0.042330607, 0.040502816,
			0.038268289, 0.035703927, 0.032893954, 0.029925358, 0.026883505, 0.023848208, 0.02089051, 0.018070321, 0.015434994, 0.013018802, 0.010843261, 0.0089181244, 0.0072429008, 0.0058086845, 0.0046001342, 0.0035974302, 0.0027780872, 0.0021185295, 0.0015953774, 0.0011864248,
			0.00087131668, 0.00063195714, 0.00045268941, 0.00032029695, 0.00022387446, 0.00015461373, 0.0001055427, 7.1248192e-05, 4.7604525e-05, 3.1523182e-05, 2.0731945e-05 }, //Row:[178]. 71 items, sigma=17.8
		{ 2.2480776e-05, 3.4015092e-05, 5.1122118e-05, 7.6157802e-05, 0.00011230898, 0.00016381252, 0.00023620176, 0.0003365711, 0.00047384329, 0.0006590168, 0.00090536296, 0.001228535, 0.0016465454, 0.0021795646, 0.0028494945, 0.003679278, 0.0046919179, 0.0059092, 0.0073501406, 0.0090292154,
			0.010954458, 0.013125552, 0.015532081, 0.018152094, 0.020951185, 0.023882237, 0.026885978, 0.029892415, 0.03282316, 0.035594588, 0.038121648, 0.04032213, 0.042121064, 0.043454949, 0.044275472, 0.044552408, 0.044275472, 0.043454949, 0.042121064, 0.04032213,
			0.038121648, 0.035594588, 0.03282316, 0.029892415, 0.026885978, 0.023882237, 0.020951185, 0.018152094, 0.015532081, 0.013125552, 0.010954458, 0.0090292154, 0.0073501406, 0.0059092, 0.0046919179, 0.003679278, 0.0028494945, 0.0021795646, 0.0016465454, 0.001228535,
			0.00090536296, 0.0006590168, 0.00047384329, 0.0003365711, 0.00023620176, 0.00016381252, 0.00011230898, 7.6157802e-05, 5.1122118e-05, 3.4015092e-05, 2.2480776e-05 }, //Row:[179]. 71 items, sigma=17.9
		{ 2.4343187e-05, 3.6655872e-05, 5.4831998e-05, 8.1311632e-05, 0.00011937967, 0.000173383, 0.00024897269, 0.00035336167, 0.00049558178, 0.00068671756, 0.00094008652, 0.0012713288, 0.0016983636, 0.0022411674, 0.0029213308, 0.0037613561, 0.0047836743, 0.0060093782, 0.0074566948, 0.0091392544,
			0.011064245, 0.013230574, 0.015627195, 0.018231753, 0.021009735, 0.02391428, 0.026886758, 0.029858218, 0.032751677, 0.035485207, 0.037975653, 0.040142759, 0.041913431, 0.043225819, 0.044032895, 0.044305255, 0.044032895, 0.043225819, 0.041913431, 0.040142759,
			0.037975653, 0.035485207, 0.032751677, 0.029858218, 0.026886758, 0.02391428, 0.021009735, 0.018231753, 0.015627195, 0.013230574, 0.011064245, 0.0091392544, 0.0074566948, 0.0060093782, 0.0047836743, 0.0037613561, 0.0029213308, 0.0022411674, 0.0016983636, 0.0012713288,
			0.00094008652, 0.00068671756, 0.00049558178, 0.00035336167, 0.00024897269, 0.000173383, 0.00011937967, 8.1311632e-05, 5.4831998e-05, 3.6655872e-05, 2.4343187e-05 }, //Row:[180]. 71 items, sigma=18.0
		{ 2.6324005e-05, 3.9451044e-05, 5.8740267e-05, 8.6716138e-05, 0.00012676123, 0.00018333109, 0.00026219204, 0.00037067155, 0.00051790501, 0.00071505593, 0.00097547955, 0.0013147935, 0.0017508141, 0.0023033155, 0.0029935699, 0.0038436359, 0.0048753744, 0.0061091923, 0.0075625415, 0.0092482271,
			0.011172618, 0.013333876, 0.015720355, 0.018309327, 0.021066201, 0.023944382, 0.026885893, 0.029822812, 0.032679539, 0.035375809, 0.037830312, 0.039964695, 0.041707686, 0.042999032, 0.043792953, 0.044060838, 0.043792953, 0.042999032, 0.041707686, 0.039964695,
			0.037830312, 0.035375809, 0.032679539, 0.029822812, 0.026885893, 0.023944382, 0.021066201, 0.018309327, 0.015720355, 0.013333876, 0.011172618, 0.0092482271, 0.0075625415, 0.0061091923, 0.0048753744, 0.0038436359, 0.0029935699, 0.0023033155, 0.0017508141, 0.0013147935,
			0.00097547955, 0.00071505593, 0.00051790501, 0.00037067155, 0.00026219204, 0.00018333109, 0.00012676123, 8.6716138e-05, 5.8740267e-05, 3.9451044e-05, 2.6324005e-05 }, //Row:[181]. 71 items, sigma=18.1
		{ 2.842811e-05, 4.2406148e-05, 6.2852995e-05, 9.2377685e-05, 0.00013445993, 0.00019366252, 0.0002758643, 0.00038850323, 0.00054081266, 0.00074402798, 0.0010115338, 0.0013589161, 0.0018038791, 0.0023659864, 0.0030661858, 0.0039260894, 0.00496699, 0.0062086168, 0.0076676602, 0.0093561208,
			0.011279574, 0.013435465, 0.015811581, 0.018384849, 0.021120621, 0.023972588, 0.026883427, 0.029786238, 0.03260678, 0.035266415, 0.037685633, 0.039787932, 0.041503807, 0.042774555, 0.043555605, 0.043819111, 0.043555605, 0.042774555, 0.041503807, 0.039787932,
			0.037685633, 0.035266415, 0.03260678, 0.029786238, 0.026883427, 0.023972588, 0.021120621, 0.018384849, 0.015811581, 0.013435465, 0.011279574, 0.0093561208, 0.0076676602, 0.0062086168, 0.00496699, 0.0039260894, 0.0030661858, 0.0023659864, 0.0018038791, 0.0013589161,
			0.0010115338, 0.00074402798, 0.00054081266, 0.00038850323, 0.0002758643, 0.00019366252, 0.00013445993, 9.2377685e-05, 6.2852995e-05, 4.2406148e-05, 2.842811e-05 }, //Row:[182]. 71 items, sigma=18.2
		{ 2.0017574e-05, 3.0096555e-05, 4.4962863e-05, 6.6612348e-05, 9.7738667e-05, 0.00014191804, 0.00020381885, 0.00028942972, 0.00040629492, 0.00056374009, 0.00077306544, 0.0010476767, 0.0014031193, 0.0018569767, 0.0024285938, 0.0031385888, 0.0040081251, 0.0050579299, 0.006307063, 0.0077714674,
			0.0094623597, 0.011384547, 0.013534787, 0.015900329, 0.018457783, 0.02117247, 0.023998378, 0.026878842, 0.029747974, 0.03253287, 0.035156484, 0.037541061, 0.039611898, 0.041301209, 0.042551792, 0.043320246, 0.043579469, 0.043320246, 0.042551792, 0.041301209,
			0.039611898, 0.037541061, 0.035156484, 0.03253287, 0.029747974, 0.026878842, 0.023998378, 0.02117247, 0.018457783, 0.015900329, 0.013534787, 0.011384547, 0.0094623597, 0.0077714674, 0.006307063, 0.0050579299, 0.0040081251, 0.0031385888, 0.0024285938, 0.0018569767,
			0.0014031193, 0.0010476767, 0.00077306544, 0.00056374009, 0.00040629492, 0.00028942972, 0.00020381885, 0.00014191804, 9.7738667e-05, 6.6612348e-05, 4.4962863e-05, 3.0096555e-05, 2.0017574e-05 }, //Row:[183]. 73 items, sigma=18.3
		{ 2.166047e-05, 3.2415783e-05, 4.8208223e-05, 7.1105792e-05, 0.00010388672, 0.00015022295, 0.00021488675, 0.0003039736, 0.00042512977, 0.00058776756, 0.00080324494, 0.0010849807, 0.0014484711, 0.0019111701, 0.002492197, 0.003211835, 0.0040907975, 0.0051492491, 0.0064055885, 0.0078750262,
			0.0095680143, 0.011488617, 0.013632932, 0.015987701, 0.018529244, 0.021222868, 0.024022877, 0.026873263, 0.029709142, 0.032458922, 0.035047117, 0.037397682, 0.039437667, 0.041100951, 0.042331791, 0.043087917, 0.043342949, 0.043087917, 0.042331791, 0.041100951,
			0.039437667, 0.037397682, 0.035047117, 0.032458922, 0.029709142, 0.026873263, 0.024022877, 0.021222868, 0.018529244, 0.015987701, 0.013632932, 0.011488617, 0.0095680143, 0.0078750262, 0.0064055885, 0.0051492491, 0.0040907975, 0.003211835, 0.002492197, 0.0019111701,
			0.0014484711, 0.0010849807, 0.00080324494, 0.00058776756, 0.00042512977, 0.0003039736, 0.00021488675, 0.00015022295, 0.00010388672, 7.1105792e-05, 4.8208223e-05, 3.2415783e-05, 2.166047e-05 }, //Row:[184]. 73 items, sigma=18.4
		{ 2.3407272e-05, 3.4870276e-05, 5.1627265e-05, 7.5818746e-05, 0.00011030737, 0.00015885991, 0.00022635065, 0.00031897887, 0.00044448856, 0.00061237291, 0.0008340407, 0.0011229158, 0.0014944369, 0.0019659205, 0.0025562531, 0.0032853787, 0.0041735596, 0.0052404012, 0.0065036501, 0.0079777985,
			0.0096725545, 0.011591263, 0.013729391, 0.016073198, 0.01859874, 0.021271334, 0.024045607, 0.026866212, 0.02966926, 0.032384448, 0.034937814, 0.037254984, 0.039264711, 0.040902493, 0.042113999, 0.04285806, 0.043108991, 0.04285806, 0.042113999, 0.040902493,
			0.039264711, 0.037254984, 0.034937814, 0.032384448, 0.02966926, 0.026866212, 0.024045607, 0.021271334, 0.01859874, 0.016073198, 0.013729391, 0.011591263, 0.0096725545, 0.0079777985, 0.0065036501, 0.0052404012, 0.0041735596, 0.0032853787, 0.0025562531, 0.0019659205,
			0.0014944369, 0.0011229158, 0.0008340407, 0.00061237291, 0.00044448856, 0.00031897887, 0.00022635065, 0.00015885991, 0.00011030737, 7.5818746e-05, 5.1627265e-05, 3.4870276e-05, 2.3407272e-05 }, //Row:[185]. 73 items, sigma=18.5
		{ 2.5262287e-05, 3.746495e-05, 5.5225415e-05, 8.0756973e-05, 0.00011700642, 0.00016783434, 0.00023821507, 0.00033444856, 0.00046437207, 0.00063755397, 0.00086544693, 0.0011614722, 0.0015410024, 0.0020212093, 0.00262074, 0.003359195, 0.0042563853, 0.0053313609, 0.0066012255, 0.0080797677,
			0.0097759712, 0.011692487, 0.013824174, 0.016156841, 0.018666302, 0.021317904, 0.024066608, 0.026857731, 0.029628365, 0.032309476, 0.034828594, 0.037112972, 0.039093021, 0.040705812, 0.041898385, 0.042630636, 0.042877552, 0.042630636, 0.041898385, 0.040705812,
			0.039093021, 0.037112972, 0.034828594, 0.032309476, 0.029628365, 0.026857731, 0.024066608, 0.021317904, 0.018666302, 0.016156841, 0.013824174, 0.011692487, 0.0097759712, 0.0080797677, 0.0066012255, 0.0053313609, 0.0042563853, 0.003359195, 0.00262074, 0.0020212093,
			0.0015410024, 0.0011614722, 0.00086544693, 0.00063755397, 0.00046437207, 0.00033444856, 0.00023821507, 0.00016783434, 0.00011700642, 8.0756973e-05, 5.5225415e-05, 3.746495e-05, 2.5262287e-05 }, //Row:[186]. 73 items, sigma=18.6
		{ 2.7229869e-05, 4.0204737e-05, 5.9008085e-05, 8.5926162e-05, 0.00012398954, 0.0001771515, 0.00025048429, 0.00035038535, 0.0004847807, 0.00066330816, 0.00089745744, 0.0012006394, 0.0015881532, 0.0020770178, 0.0026856353, 0.0034332594, 0.0043392491, 0.005422104, 0.0066982938, 0.0081809184,
			0.0098782564, 0.011792288, 0.013917293, 0.01623865, 0.018731961, 0.021362615, 0.024085922, 0.026847861, 0.029586495, 0.032234037, 0.034719476, 0.036971651, 0.038922589, 0.040510889, 0.041684919, 0.042405606, 0.042648594, 0.042405606, 0.041684919, 0.040510889,
			0.038922589, 0.036971651, 0.034719476, 0.032234037, 0.029586495, 0.026847861, 0.024085922, 0.021362615, 0.018731961, 0.01623865, 0.013917293, 0.011792288, 0.0098782564, 0.0081809184, 0.0066982938, 0.005422104, 0.0043392491, 0.0034332594, 0.0026856353, 0.0020770178,
			0.0015881532, 0.0012006394, 0.00089745744, 0.00066330816, 0.0004847807, 0.00035038535, 0.00025048429, 0.0001771515, 0.00012398954, 8.5926162e-05, 5.9008085e-05, 4.0204737e-05, 2.7229869e-05 }, //Row:[187]. 73 items, sigma=18.7
		{ 1.9349985e-05, 2.8784281e-05, 4.2564451e-05, 6.2450522e-05, 9.0801791e-05, 0.00013073213, 0.00018628626, 0.00026263218, 0.00036626144, 0.00050518432, 0.00068910236, 0.00092953552, 0.0012398769, 0.0016353444, 0.0021327973, 0.0027503871, 0.0035070175, 0.0044215961, 0.0055120766, 0.0067943045,
			0.0082807055, 0.0099788729, 0.011890139, 0.014008228, 0.016318119, 0.018795215, 0.021404974, 0.024103059, 0.026836111, 0.029543155, 0.032157627, 0.034609945, 0.036830497, 0.038752878, 0.040317172, 0.041473039, 0.042182405, 0.042421547, 0.042182405, 0.041473039,
			0.040317172, 0.038752878, 0.036830497, 0.034609945, 0.032157627, 0.029543155, 0.026836111, 0.024103059, 0.021404974, 0.018795215, 0.016318119, 0.014008228, 0.011890139, 0.0099788729, 0.0082807055, 0.0067943045, 0.0055120766, 0.0044215961, 0.0035070175, 0.0027503871,
			0.0021327973, 0.0016353444, 0.0012398769, 0.00092953552, 0.00068910236, 0.00050518432, 0.00036626144, 0.00026263218, 0.00018628626, 0.00013073213, 9.0801791e-05, 6.2450522e-05, 4.2564451e-05, 2.8784281e-05, 1.9349985e-05 }, //Row:[188]. 75 items, sigma=18.8
		{ 2.0896254e-05, 3.0947868e-05, 4.5566958e-05, 6.6575986e-05, 9.6407298e-05, 0.00013825749, 0.00019626135, 0.00027568035, 0.00038309674, 0.00052660049, 0.00071595111, 0.00096269217, 0.0012801917, 0.001683579, 0.0021895469, 0.0028159914, 0.0035814633, 0.0045044199, 0.0056022737, 0.0068902562,
			0.0083801334, 0.010078832, 0.01198706, 0.014098011, 0.016396285, 0.018857114, 0.021446035, 0.024119075, 0.026823538, 0.029499397, 0.032081293, 0.034501038, 0.036690531, 0.038584897, 0.040125661, 0.041263734, 0.041962015, 0.042197391, 0.041962015, 0.041263734,
			0.040125661, 0.038584897, 0.036690531, 0.034501038, 0.032081293, 0.029499397, 0.026823538, 0.024119075, 0.021446035, 0.018857114, 0.016396285, 0.014098011, 0.01198706, 0.010078832, 0.0083801334, 0.0068902562, 0.0056022737, 0.0045044199, 0.0035814633, 0.0028159914,
			0.0021895469, 0.001683579, 0.0012801917, 0.00096269217, 0.00071595111, 0.00052660049, 0.00038309674, 0.00027568035, 0.00019626135, 0.00013825749, 9.6407298e-05, 6.6575986e-05, 4.5566958e-05, 3.0947868e-05, 2.0896254e-05 }, //Row:[189]. 75 items, sigma=18.9
		{ 2.2537822e-05, 3.3234712e-05, 4.8726831e-05, 7.0899401e-05, 0.00010225773, 0.00014608045, 0.000206591, 0.00028914186, 0.00040040255, 0.00054853808, 0.00074336028, 0.00099642972, 0.0013210822, 0.0017323514, 0.0022467576, 0.0028819361, 0.0036560831, 0.0045872064, 0.0056921829, 0.00698564,
			0.0084786989, 0.010177639, 0.012082565, 0.014186164, 0.016472681, 0.018917196, 0.021485342, 0.024133519, 0.026809689, 0.029454766, 0.032004569, 0.034392279, 0.036551267, 0.038418147, 0.039935844, 0.041056485, 0.041743909, 0.041975598, 0.041743909, 0.041056485,
			0.039935844, 0.038418147, 0.036551267, 0.034392279, 0.032004569, 0.029454766, 0.026809689, 0.024133519, 0.021485342, 0.018917196, 0.016472681, 0.014186164, 0.012082565, 0.010177639, 0.0084786989, 0.00698564, 0.0056921829, 0.0045872064, 0.0036560831, 0.0028819361,
			0.0022467576, 0.0017323514, 0.0013210822, 0.00099642972, 0.00074336028, 0.00054853808, 0.00040040255, 0.00028914186, 0.000206591, 0.00014608045, 0.00010225773, 7.0899401e-05, 4.8726831e-05, 3.3234712e-05, 2.2537822e-05 }, //Row:[190]. 75 items, sigma=19.0
		{ 2.4278544e-05, 3.5649203e-05, 5.2048917e-05, 7.5425926e-05, 0.00010835832, 0.000154206, 0.00021727947, 0.00030301976, 0.0004181801, 0.00057099591, 0.00077132566, 0.0010307405, 0.001362537, 0.0017816466, 0.0023044107, 0.0029481998, 0.0037308536, 0.0046699322, 0.0057817827, 0.0070804381,
			0.0085763898, 0.010275289, 0.012176657, 0.0142727, 0.016547328, 0.018975492, 0.021522932, 0.024146428, 0.026794602, 0.029409294, 0.031927482, 0.034283684, 0.03641271, 0.03825262, 0.039747702, 0.040851263, 0.041528053, 0.041756132, 0.041528053, 0.040851263,
			0.039747702, 0.03825262, 0.03641271, 0.034283684, 0.031927482, 0.029409294, 0.026794602, 0.024146428, 0.021522932, 0.018975492, 0.016547328, 0.0142727, 0.012176657, 0.010275289, 0.0085763898, 0.0070804381, 0.0057817827, 0.0046699322, 0.0037308536, 0.0029481998,
			0.0023044107, 0.0017816466, 0.001362537, 0.0010307405, 0.00077132566, 0.00057099591, 0.0004181801, 0.00030301976, 0.00021727947, 0.000154206, 0.00010835832, 7.5425926e-05, 5.2048917e-05, 3.5649203e-05, 2.4278544e-05 }, //Row:[191]. 75 items, sigma=19.1
		{ 2.6122318e-05, 3.8195749e-05, 5.553805e-05, 8.0160658e-05, 0.00011471419, 0.00016263892, 0.00022833079, 0.00031731681, 0.00043643029, 0.00059397242, 0.00079984268, 0.0010656163, 0.0014045444, 0.0018314489, 0.0023624872, 0.0030147612, 0.0038057522, 0.0047525749, 0.0058710523, 0.0071746338,
			0.0086731949, 0.010371777, 0.012269341, 0.014357631, 0.016620249, 0.01903203, 0.021558839, 0.02415784, 0.026778313, 0.029363012, 0.031850056, 0.034175268, 0.036274862, 0.038088307, 0.039561215, 0.040648039, 0.041314413, 0.041538957, 0.041314413, 0.040648039,
			0.039561215, 0.038088307, 0.036274862, 0.034175268, 0.031850056, 0.029363012, 0.026778313, 0.02415784, 0.021558839, 0.01903203, 0.016620249, 0.014357631, 0.012269341, 0.010371777, 0.0086731949, 0.0071746338, 0.0058710523, 0.0047525749, 0.0038057522, 0.0030147612,
			0.0023624872, 0.0018314489, 0.0014045444, 0.0010656163, 0.00079984268, 0.00059397242, 0.00043643029, 0.00031731681, 0.00022833079, 0.00016263892, 0.00011471419, 8.0160658e-05, 5.553805e-05, 3.8195749e-05, 2.6122318e-05 }, //Row:[192]. 75 items, sigma=19.2
		{ 1.8724785e-05, 2.7573753e-05, 4.0379445e-05, 5.8699715e-05, 8.4609308e-05, 0.00012083101, 0.00017088452, 0.00023924947, 0.00033153617, 0.00045465436, 0.00061696637, 0.0008284071, 0.0011005498, 0.001446593, 0.0018812435, 0.0024204693, 0.0030810997, 0.0038802573, 0.0048346131, 0.0059594724,
			0.0072677114, 0.0087686047, 0.010466601, 0.012360122, 0.014440474, 0.016690967, 0.019086341, 0.021592597, 0.024167292, 0.026760358, 0.029315453, 0.031771814, 0.034066545, 0.036137226, 0.037924701, 0.039375866, 0.040446287, 0.041102457, 0.041323538, 0.041102457,
			0.040446287, 0.039375866, 0.037924701, 0.036137226, 0.034066545, 0.031771814, 0.029315453, 0.026760358, 0.024167292, 0.021592597, 0.019086341, 0.016690967, 0.014440474, 0.012360122, 0.010466601, 0.0087686047, 0.0072677114, 0.0059594724, 0.0048346131, 0.0038802573,
			0.0030810997, 0.0024204693, 0.0018812435, 0.001446593, 0.0011005498, 0.0008284071, 0.00061696637, 0.00045465436, 0.00033153617, 0.00023924947, 0.00017088452, 0.00012083101, 8.4609308e-05, 5.8699715e-05, 4.0379445e-05, 2.7573753e-05, 1.8724785e-05 }, //Row:[193]. 77 items, sigma=19.3
		{ 2.018268e-05, 2.9596598e-05, 4.3164499e-05, 6.2498479e-05, 8.9736621e-05, 0.00012767345, 0.00017990703, 0.00025099884, 0.0003466398, 0.00047381237, 0.00064093531, 0.00085797347, 0.0011364918, 0.0014896304, 0.0019319747, 0.0024792981, 0.0031481544, 0.0039553066, 0.004916985, 0.0060479835,
			0.0073606155, 0.0088635693, 0.010560719, 0.012449968, 0.014522202, 0.016760463, 0.019139412, 0.021625199, 0.024175778, 0.02674173, 0.029267606, 0.031693741, 0.033958489, 0.036000766, 0.037762752, 0.039192594, 0.040246939, 0.040893113, 0.041110801, 0.040893113,
			0.040246939, 0.039192594, 0.037762752, 0.036000766, 0.033958489, 0.031693741, 0.029267606, 0.02674173, 0.024175778, 0.021625199, 0.019139412, 0.016760463, 0.014522202, 0.012449968, 0.010560719, 0.0088635693, 0.0073606155, 0.0060479835, 0.004916985, 0.0039553066,
			0.0031481544, 0.0024792981, 0.0019319747, 0.0014896304, 0.0011364918, 0.00085797347, 0.00064093531, 0.00047381237, 0.0003466398, 0.00025099884, 0.00017990703, 0.00012767345, 8.9736621e-05, 6.2498479e-05, 4.3164499e-05, 2.9596598e-05, 2.018268e-05 }, //Row:[194]. 77 items, sigma=19.4
		{ 2.1728191e-05, 3.1732064e-05, 4.6092561e-05, 6.6476312e-05, 9.5084702e-05, 0.00013478348, 0.00018924793, 0.00026311948, 0.00036216682, 0.00049344144, 0.00066541388, 0.00088807344, 0.001172971, 0.0015331816, 0.001983164, 0.0025384923, 0.003215442, 0.004030416, 0.004999207, 0.0061361043,
			0.0074528689, 0.008957617, 0.010653666, 0.01253842, 0.014602367, 0.016828297, 0.019190811, 0.021656217, 0.02418287, 0.026722001, 0.029219038, 0.031615394, 0.033850649, 0.035865021, 0.037601989, 0.039010919, 0.040049507, 0.040685885, 0.04090025, 0.040685885,
			0.040049507, 0.039010919, 0.037601989, 0.035865021, 0.033850649, 0.031615394, 0.029219038, 0.026722001, 0.02418287, 0.021656217, 0.019190811, 0.016828297, 0.014602367, 0.01253842, 0.010653666, 0.008957617, 0.0074528689, 0.0061361043, 0.004999207, 0.004030416,
			0.003215442, 0.0025384923, 0.001983164, 0.0015331816, 0.001172971, 0.00088807344, 0.00066541388, 0.00049344144, 0.00036216682, 0.00026311948, 0.00018924793, 0.00013478348, 9.5084702e-05, 6.6476312e-05, 4.6092561e-05, 3.1732064e-05, 2.1728191e-05 }, //Row:[195]. 77 items, sigma=19.5
		{ 2.3364778e-05, 3.3984083e-05, 4.9167968e-05, 7.0637838e-05, 0.00010065827, 0.00014216563, 0.00019891121, 0.00027561438, 0.00037811878, 0.00051354112, 0.00069039908, 0.00091870107, 0.0012099779, 0.001577234, 0.0020347956, 0.0025980333, 0.0032829419, 0.0041055644, 0.0050812589, 0.0062238167,
			0.0075444577, 0.0090507399, 0.010745442, 0.012625485, 0.014680983, 0.016894491, 0.019240565, 0.021685683, 0.024188604, 0.026701203, 0.029169777, 0.031536796, 0.033743039, 0.035729992, 0.037442404, 0.038830823, 0.039853963, 0.040480742, 0.040691851, 0.040480742,
			0.039853963, 0.038830823, 0.037442404, 0.035729992, 0.033743039, 0.031536796, 0.029169777, 0.026701203, 0.024188604, 0.021685683, 0.019240565, 0.016894491, 0.014680983, 0.012625485, 0.010745442, 0.0090507399, 0.0075444577, 0.0062238167, 0.0050812589, 0.0041055644,
			0.0032829419, 0.0025980333, 0.0020347956, 0.001577234, 0.0012099779, 0.00091870107, 0.00069039908, 0.00051354112, 0.00037811878, 0.00027561438, 0.00019891121, 0.00014216563, 0.00010065827, 7.0637838e-05, 4.9167968e-05, 3.3984083e-05, 2.3364778e-05 }, //Row:[196]. 77 items, sigma=19.6
		{ 2.509594e-05, 3.6356605e-05, 5.2395052e-05, 7.4987638e-05, 0.00010646195, 0.00014982431, 0.00020890068, 0.00028848633, 0.00039449692, 0.00053411063, 0.00071588763, 0.00094985012, 0.0012475031, 0.0016217748, 0.0020868536, 0.0026579029, 0.0033506342, 0.0041807312, 0.0051631208, 0.0063111037,
			0.0076353688, 0.0091429305, 0.010836045, 0.012711172, 0.014758065, 0.016959068, 0.019288703, 0.021713628, 0.024193011, 0.026679368, 0.02911985, 0.031457968, 0.033635668, 0.035595681, 0.037283987, 0.038652286, 0.039660282, 0.040277654, 0.040485573, 0.040277654,
			0.039660282, 0.038652286, 0.037283987, 0.035595681, 0.033635668, 0.031457968, 0.02911985, 0.026679368, 0.024193011, 0.021713628, 0.019288703, 0.016959068, 0.014758065, 0.012711172, 0.010836045, 0.0091429305, 0.0076353688, 0.0063111037, 0.0051631208, 0.0041807312,
			0.0033506342, 0.0026579029, 0.0020868536, 0.0016217748, 0.0012475031, 0.00094985012, 0.00071588763, 0.00053411063, 0.00039449692, 0.00028848633, 0.00020890068, 0.00014982431, 0.00010646195, 7.4987638e-05, 5.2395052e-05, 3.6356605e-05, 2.509594e-05 }, //Row:[197]. 77 items, sigma=19.7
		{ 1.813811e-05, 2.6454093e-05, 3.8382473e-05, 5.5307009e-05, 7.9059124e-05, 0.00011202916, 0.00015729263, 0.00021874882, 0.00030126673, 0.0004108311, 0.00055467776, 0.00074140478, 0.00098104295, 0.001285066, 0.0016663201, 0.0021388513, 0.0027176116, 0.0034180279, 0.004255425, 0.0052443024,
			0.0063974775, 0.0077251188, 0.009233711, 0.010925004, 0.012795016, 0.014833158, 0.017021579, 0.019334781, 0.021739613, 0.024195655, 0.026656056, 0.029068814, 0.031378457, 0.033528079, 0.035461618, 0.03712626, 0.038474821, 0.039467968, 0.040076119, 0.040280912,
			0.040076119, 0.039467968, 0.038474821, 0.03712626, 0.035461618, 0.033528079, 0.031378457, 0.029068814, 0.026656056, 0.024195655, 0.021739613, 0.019334781, 0.017021579, 0.014833158, 0.012795016, 0.010925004, 0.009233711, 0.0077251188, 0.0063974775, 0.0052443024,
			0.004255425, 0.0034180279, 0.0027176116, 0.0021388513, 0.0016663201, 0.001285066, 0.00098104295, 0.00074140478, 0.00055467776, 0.0004108311, 0.00030126673, 0.00021874882, 0.00015729263, 0.00011202916, 7.9059124e-05, 5.5307009e-05, 3.8382473e-05, 2.6454093e-05, 1.813811e-05 }, //Row:[198]. 79 items, sigma=19.8
		{ 1.9514974e-05, 2.8349277e-05, 4.0972139e-05, 5.8814611e-05, 8.3763244e-05, 0.00011827081, 0.0001654812, 0.00022936551, 0.00031486434, 0.0004280285, 0.00057614761, 0.00076785313, 0.0010131793, 0.001323563, 0.0017117633, 0.0021916792, 0.0027780479, 0.0034860101, 0.0043305326, 0.0053256918,
			0.0064838286, 0.0078146024, 0.0093239819, 0.011013228, 0.012877932, 0.014907184, 0.017082951, 0.019379734, 0.021764576, 0.024197473, 0.026632205, 0.029017601, 0.03129919, 0.033421187, 0.035328711, 0.036970118, 0.038299315, 0.039277902, 0.039877015, 0.040078744,
			0.039877015, 0.039277902, 0.038299315, 0.036970118, 0.035328711, 0.033421187, 0.03129919, 0.029017601, 0.026632205, 0.024197473, 0.021764576, 0.019379734, 0.017082951, 0.014907184, 0.012877932, 0.011013228, 0.0093239819, 0.0078146024, 0.0064838286, 0.0053256918,
			0.0043305326, 0.0034860101, 0.0027780479, 0.0021916792, 0.0017117633, 0.001323563, 0.0010131793, 0.00076785313, 0.00057614761, 0.0004280285, 0.00031486434, 0.00022936551, 0.0001654812, 0.00011827081, 8.3763244e-05, 5.8814611e-05, 4.0972139e-05, 2.8349277e-05, 1.9514974e-05 }, //Row:[199]. 79 items, sigma=19.9
		{ 2.0972571e-05, 3.0347628e-05, 4.3692125e-05, 6.2484677e-05, 8.8666957e-05, 0.00012475378, 0.0001739565, 0.00024031653, 0.00032884377, 0.00044565207, 0.00059808106, 0.00079479104, 0.0010458146, 0.0013625467, 0.001757654, 0.0022448841, 0.0028387564, 0.003554124, 0.0044055973, 0.0054068334,
			0.0065697045, 0.0079033713, 0.0094133004, 0.011100279, 0.012959491, 0.014979721, 0.017142771, 0.019423151, 0.02178811, 0.024198059, 0.026607406, 0.028965799, 0.031219748, 0.033314565, 0.035196523, 0.036815118, 0.038125315, 0.039089623, 0.039679876, 0.039878601,
			0.039679876, 0.039089623, 0.038125315, 0.036815118, 0.035196523, 0.033314565, 0.031219748, 0.028965799, 0.026607406, 0.024198059, 0.02178811, 0.019423151, 0.017142771, 0.014979721, 0.012959491, 0.011100279, 0.0094133004, 0.0079033713, 0.0065697045, 0.0054068334,
			0.0044055973, 0.003554124, 0.0028387564, 0.0022448841, 0.001757654, 0.0013625467, 0.0010458146, 0.00079479104, 0.00059808106, 0.00044565207, 0.00032884377, 0.00024031653, 0.0001739565, 0.00012475378, 8.8666957e-05, 6.2484677e-05, 4.3692125e-05, 3.0347628e-05, 2.0972571e-05 }, //Row:[200]. 79 items, sigma=20.0
		{ 2.251402e-05, 3.2452679e-05, 4.6546327e-05, 6.6321366e-05, 9.3774526e-05, 0.00013148221, 0.00018272225, 0.00025160479, 0.00034320674, 0.0004637019, 0.00062047608, 0.00082221395, 0.0010789415, 0.0014020067, 0.001803979, 0.0022984501, 0.0028997192, 0.0036223508, 0.0044806001, 0.0054877097,
			0.0066550905, 0.0079914152, 0.0095016618, 0.01118616, 0.013039702, 0.015050785, 0.017201061, 0.01946506, 0.021810245, 0.024197445, 0.026581688, 0.028913431, 0.031140149, 0.033208224, 0.035065054, 0.036661249, 0.037952802, 0.038903106, 0.039484672, 0.039680453,
			0.039484672, 0.038903106, 0.037952802, 0.036661249, 0.035065054, 0.033208224, 0.031140149, 0.028913431, 0.026581688, 0.024197445, 0.021810245, 0.01946506, 0.017201061, 0.015050785, 0.013039702, 0.01118616, 0.0095016618, 0.0079914152, 0.0066550905, 0.0054877097,
			0.0044806001, 0.0036223508, 0.0028997192, 0.0022984501, 0.001803979, 0.0014020067, 0.0010789415, 0.00082221395, 0.00062047608, 0.0004637019, 0.00034320674, 0.00025160479, 0.00018272225, 0.00013148221, 9.3774526e-05, 6.6321366e-05, 4.6546327e-05, 3.2452679e-05, 2.251402e-05 }, //Row:[201]. 79 items, sigma=20.1
		{ 2.4142471e-05, 3.4667978e-05, 4.9538636e-05, 7.0328801e-05, 9.9090142e-05, 0.00013846011, 0.00019178201, 0.00026323303, 0.00035795473, 0.00048217777, 0.00064333036, 0.00085011707, 0.0011125525, 0.0014419326, 0.0018507249, 0.0023523617, 0.0029609189, 0.0036906719, 0.0045555228, 0.0055683039,
			0.0067399726, 0.0080787244, 0.009589062, 0.011270873, 0.013118574, 0.015120391, 0.017257841, 0.019505487, 0.02183101, 0.02419566, 0.02655508, 0.028860524, 0.03106041, 0.033102172, 0.034934306, 0.036508502, 0.03778176, 0.038718328, 0.039291377, 0.039484272,
			0.039291377, 0.038718328, 0.03778176, 0.036508502, 0.034934306, 0.033102172, 0.03106041, 0.028860524, 0.02655508, 0.02419566, 0.02183101, 0.019505487, 0.017257841, 0.015120391, 0.013118574, 0.011270873, 0.009589062, 0.0080787244, 0.0067399726, 0.0055683039,
			0.0045555228, 0.0036906719, 0.0029609189, 0.0023523617, 0.0018507249, 0.0014419326, 0.0011125525, 0.00085011707, 0.00064333036, 0.00048217777, 0.00035795473, 0.00026323303, 0.00019178201, 0.00013846011, 9.9090142e-05, 7.0328801e-05, 4.9538636e-05, 3.4667978e-05, 2.4142471e-05 }, //Row:[202]. 79 items, sigma=20.2
		{ 2.5861104e-05, 3.6997088e-05, 5.2672931e-05, 7.4511067e-05, 0.00010461792, 0.00014569141, 0.00020113917, 0.00027520377, 0.00037308898, 0.00050107924, 0.00066664131, 0.00087849534, 0.0011466398, 0.0014823137, 0.0018978785, 0.002406603, 0.0030223379, 0.003759069, 0.0046303472, 0.0056485995,
			0.0068243375, 0.0081652898, 0.0096754973, 0.01135442, 0.013196116, 0.015188556, 0.017313135, 0.019544458, 0.021850435, 0.024192733, 0.026527609, 0.028807098, 0.030980548, 0.032996419, 0.034804277, 0.03635687, 0.037612172, 0.038535267, 0.039099963, 0.039290028,
			0.039099963, 0.038535267, 0.037612172, 0.03635687, 0.034804277, 0.032996419, 0.030980548, 0.028807098, 0.026527609, 0.024192733, 0.021850435, 0.019544458, 0.017313135, 0.015188556, 0.013196116, 0.01135442, 0.0096754973, 0.0081652898, 0.0068243375, 0.0056485995,
			0.0046303472, 0.003759069, 0.0030223379, 0.002406603, 0.0018978785, 0.0014823137, 0.0011466398, 0.00087849534, 0.00066664131, 0.00050107924, 0.00037308898, 0.00027520377, 0.00020113917, 0.00014569141, 0.00010461792, 7.4511067e-05, 5.2672931e-05, 3.6997088e-05, 2.5861104e-05 }, //Row:[203]. 79 items, sigma=20.3
		{ 1.8888932e-05, 2.7194928e-05, 3.896538e-05, 5.5474877e-05, 7.8393998e-05, 0.00010988368, 0.00015270167, 0.00021031878, 0.00028704112, 0.00038813231, 0.00051992738, 0.00068992789, 0.00090686525, 0.0011807172, 0.0015226612, 0.0019449481, 0.0024606802, 0.0030834808, 0.0038270461, 0.0047045779,
			0.0057281027, 0.0069076941, 0.0082506249, 0.0097604867, 0.011436327, 0.013271861, 0.015254817, 0.017366485, 0.019581522, 0.021868069, 0.024188216, 0.026498823, 0.028752699, 0.030900101, 0.032890493, 0.034674489, 0.036205865, 0.037443543, 0.03835342, 0.038909925,
			0.039097215, 0.038909925, 0.03835342, 0.037443543, 0.036205865, 0.034674489, 0.032890493, 0.030900101, 0.028752699, 0.026498823, 0.024188216, 0.021868069, 0.019581522, 0.017366485, 0.015254817, 0.013271861, 0.011436327, 0.0097604867, 0.0082506249, 0.0069076941,
			0.0057281027, 0.0047045779, 0.0038270461, 0.0030834808, 0.0024606802, 0.0019449481, 0.0015226612, 0.0011807172, 0.00090686525, 0.00068992789, 0.00051992738, 0.00038813231, 0.00028704112, 0.00021031878, 0.00015270167, 0.00010988368, 7.8393998e-05, 5.5474877e-05, 3.896538e-05, 2.7194928e-05,
			1.8888932e-05 }, //Row:[204]. 81 items, sigma=20.4
		{ 2.0265871e-05, 2.9068717e-05, 4.1497974e-05, 5.8869862e-05, 8.2903125e-05, 0.00011581292, 0.00016041609, 0.00022024541, 0.00029966875, 0.00040400702, 0.00053964275, 0.00071410854, 0.00093614281, 0.001215698, 0.0015638856, 0.0019928419, 0.0025154995, 0.0031452523, 0.0038955072, 0.0047791192,
			0.0058077196, 0.006990952, 0.0083356434, 0.0098449492, 0.011517519, 0.01334674, 0.015320114, 0.017418835, 0.019617625, 0.021884863, 0.024183059, 0.026469671, 0.02869827, 0.030820006, 0.032785326, 0.034545864, 0.036056399, 0.037276779, 0.038173687, 0.038722158,
			0.038906728, 0.038722158, 0.038173687, 0.037276779, 0.036056399, 0.034545864, 0.032785326, 0.030820006, 0.02869827, 0.026469671, 0.024183059, 0.021884863, 0.019617625, 0.017418835, 0.015320114, 0.01334674, 0.011517519, 0.0098449492, 0.0083356434, 0.006990952,
			0.0058077196, 0.0047791192, 0.0038955072, 0.0031452523, 0.0025154995, 0.0019928419, 0.0015638856, 0.001215698, 0.00093614281, 0.00071410854, 0.00053964275, 0.00040400702, 0.00029966875, 0.00022024541, 0.00016041609, 0.00011581292, 8.2903125e-05, 5.8869862e-05, 4.1497974e-05, 2.9068717e-05,
			2.0265871e-05 }, //Row:[205]. 81 items, sigma=20.5
		{ 2.1720178e-05, 3.1040424e-05, 4.4153149e-05, 6.2416405e-05, 8.7597076e-05, 0.00012196419, 0.0001683929, 0.00023047668, 0.00031264325, 0.00042026835, 0.00055977883, 0.00073873462, 0.00096587696, 0.0012511286, 0.0016055308, 0.0020411013, 0.0025706, 0.0032071903, 0.0039639896, 0.0048535093,
			0.0058869904, 0.0070736543, 0.0084198928, 0.0099284377, 0.011597555, 0.013420318, 0.015384016, 0.017469762, 0.019652349, 0.0219004, 0.024176843, 0.026439733, 0.028643388, 0.030739834, 0.03268048, 0.034417956, 0.03590802, 0.037111418, 0.037995602, 0.038536193,
			0.038718093, 0.038536193, 0.037995602, 0.037111418, 0.03590802, 0.034417956, 0.03268048, 0.030739834, 0.028643388, 0.026439733, 0.024176843, 0.0219004, 0.019652349, 0.017469762, 0.015384016, 0.013420318, 0.011597555, 0.0099284377, 0.0084198928, 0.0070736543,
			0.0058869904, 0.0048535093, 0.0039639896, 0.0032071903, 0.0025706, 0.0020411013, 0.0016055308, 0.0012511286, 0.00096587696, 0.00073873462, 0.00055977883, 0.00042026835, 0.00031264325, 0.00023047668, 0.0001683929, 0.00012196419, 8.7597076e-05, 6.2416405e-05, 4.4153149e-05, 3.1040424e-05,
			2.1720178e-05 }, //Row:[206]. 81 items, sigma=20.6
		{ 2.3254698e-05, 3.3113246e-05, 4.6934408e-05, 6.611823e-05, 9.2479653e-05, 0.00012834116, 0.00017663543, 0.00024101523, 0.00032596626, 0.00043691654, 0.00058033409, 0.00076380246, 0.00099606164, 0.0012870004, 0.0016475856, 0.0020897128, 0.0026259665, 0.0032692782, 0.0040324767, 0.0049277321,
			0.0059659009, 0.0071557899, 0.0085033663, 0.010010951, 0.011676439, 0.013492605, 0.01544654, 0.017519287, 0.019685719, 0.021914705, 0.024169596, 0.026409033, 0.028588072, 0.030659598, 0.032575961, 0.034290764, 0.035760717, 0.036947444, 0.037819142, 0.038352002,
			0.038531286, 0.038352002, 0.037819142, 0.036947444, 0.035760717, 0.034290764, 0.032575961, 0.030659598, 0.028588072, 0.026409033, 0.024169596, 0.021914705, 0.019685719, 0.017519287, 0.01544654, 0.013492605, 0.011676439, 0.010010951, 0.0085033663, 0.0071557899,
			0.0059659009, 0.0049277321, 0.0040324767, 0.0032692782, 0.0026259665, 0.0020897128, 0.0016475856, 0.0012870004, 0.00099606164, 0.00076380246, 0.00058033409, 0.00043691654, 0.00032596626, 0.00024101523, 0.00017663543, 0.00012834116, 9.2479653e-05, 6.611823e-05, 4.6934408e-05, 3.3113246e-05,
			2.3254698e-05 }, //Row:[207]. 81 items, sigma=20.7
		{ 2.4872302e-05, 3.5290395e-05, 4.9845248e-05, 6.9979022e-05, 9.7554589e-05, 0.00013494743, 0.00018514683, 0.00025186353, 0.00033963919, 0.00045395157, 0.00060130674, 0.00078930818, 0.0010266906, 0.0013233047, 0.0016900387, 0.0021386629, 0.0026815837, 0.0033314996, 0.0041009517, 0.0050017719,
			0.0060444373, 0.0072373484, 0.008586058, 0.010092488, 0.011754177, 0.013563613, 0.015507703, 0.017567431, 0.019717758, 0.021927806, 0.024161345, 0.026377596, 0.028532343, 0.030579313, 0.032471776, 0.034164287, 0.035614483, 0.036784842, 0.037644287, 0.038169563,
			0.038346279, 0.038169563, 0.037644287, 0.036784842, 0.035614483, 0.034164287, 0.032471776, 0.030579313, 0.028532343, 0.026377596, 0.024161345, 0.021927806, 0.019717758, 0.017567431, 0.015507703, 0.013563613, 0.011754177, 0.010092488, 0.008586058, 0.0072373484,
			0.0060444373, 0.0050017719, 0.0041009517, 0.0033314996, 0.0026815837, 0.0021386629, 0.0016900387, 0.0013233047, 0.0010266906, 0.00078930818, 0.00060130674, 0.00045395157, 0.00033963919, 0.00025186353, 0.00018514683, 0.00013494743, 9.7554589e-05, 6.9979022e-05, 4.9845248e-05, 3.5290395e-05,
			2.4872302e-05 }, //Row:[208]. 81 items, sigma=20.8
		{ 1.8300845e-05, 2.6124013e-05, 3.7123217e-05, 5.2437282e-05, 7.355056e-05, 0.00010237368, 0.0001413346, 0.0001934783, 0.000262572, 0.0003532114, 0.00047092137, 0.00062224289, 0.00081479579, 0.0010573055, 0.0013595807, 0.0017324272, 0.0021874864, 0.0027369847, 0.0033933868, 0.0041689467,
			0.0050751615, 0.0061221347, 0.0073178681, 0.0086675102, 0.010172596, 0.01183032, 0.013632901, 0.01556707, 0.017613763, 0.01974804, 0.021939277, 0.024151663, 0.026344993, 0.028475768, 0.03049854, 0.032367481, 0.034038072, 0.035468857, 0.036623144, 0.037470563,
			0.037988398, 0.038162595, 0.037988398, 0.037470563, 0.036623144, 0.035468857, 0.034038072, 0.032367481, 0.03049854, 0.028475768, 0.026344993, 0.024151663, 0.021939277, 0.01974804, 0.017613763, 0.01556707, 0.013632901, 0.01183032, 0.010172596, 0.0086675102,
			0.0073178681, 0.0061221347, 0.0050751615, 0.0041689467, 0.0033933868, 0.0027369847, 0.0021874864, 0.0017324272, 0.0013595807, 0.0010573055, 0.00081479579, 0.00062224289, 0.00047092137, 0.0003532114, 0.000262572, 0.0001934783, 0.0001413346, 0.00010237368, 7.355056e-05, 5.2437282e-05,
			3.7123217e-05, 2.6124013e-05, 1.8300845e-05 }, //Row:[209]. 83 items, sigma=20.9
		{ 1.9603605e-05, 2.7884328e-05, 3.9486518e-05, 5.5585559e-05, 7.7708031e-05, 0.0001078121, 0.00014837763, 0.0002025043, 0.00027401436, 0.00036755547, 0.00048869706, 0.00064401188, 0.00084113254, 0.0010887714, 0.0013966909, 0.0017756111, 0.0022370414, 0.0027930262, 0.0034557953, 0.0042373174,
			0.0051487579, 0.006199852, 0.0073982111, 0.0087485896, 0.010252147, 0.011905748, 0.013701351, 0.015625528, 0.017659177, 0.019777461, 0.021950016, 0.024141447, 0.02631212, 0.028419238, 0.030418165, 0.032263952, 0.033912989, 0.035324702, 0.036463206, 0.037298821,
			0.037809355, 0.037981081, 0.037809355, 0.037298821, 0.036463206, 0.035324702, 0.033912989, 0.032263952, 0.030418165, 0.028419238, 0.02631212, 0.024141447, 0.021950016, 0.019777461, 0.017659177, 0.015625528, 0.013701351, 0.011905748, 0.010252147, 0.0087485896,
			0.0073982111, 0.006199852, 0.0051487579, 0.0042373174, 0.0034557953, 0.0027930262, 0.0022370414, 0.0017756111, 0.0013966909, 0.0010887714, 0.00084113254, 0.00064401188, 0.00048869706, 0.00036755547, 0.00027401436, 0.0002025043, 0.00014837763, 0.0001078121, 7.7708031e-05, 5.5585559e-05,
			3.9486518e-05, 2.7884328e-05, 1.9603605e-05 }, //Row:[210]. 83 items, sigma=21.0
		{ 2.0977902e-05, 2.973471e-05, 4.1962053e-05, 5.8872056e-05, 8.2033526e-05, 0.00011345188, 0.00015565831, 0.00021180612, 0.00028577107, 0.00038225074, 0.00050685651, 0.00066618978, 0.00086789242, 0.0011206599, 0.0014342049, 0.0018191578, 0.0022868934, 0.0028492718, 0.0035182882, 0.0043056269,
			0.0052221251, 0.0062771556, 0.0074779472, 0.00882887, 0.01033072, 0.011980044, 0.013768553, 0.015682672, 0.017703271, 0.019805623, 0.021959626, 0.024130301, 0.026278577, 0.028362349, 0.030337778, 0.032160775, 0.033788616, 0.035181587, 0.036304593, 0.037128621,
			0.03763199, 0.037801291, 0.03763199, 0.037128621, 0.036304593, 0.035181587, 0.033788616, 0.032160775, 0.030337778, 0.028362349, 0.026278577, 0.024130301, 0.021959626, 0.019805623, 0.017703271, 0.015682672, 0.013768553, 0.011980044, 0.01033072, 0.00882887,
			0.0074779472, 0.0062771556, 0.0052221251, 0.0043056269, 0.0035182882, 0.0028492718, 0.0022868934, 0.0018191578, 0.0014342049, 0.0011206599, 0.00086789242, 0.00066618978, 0.00050685651, 0.00038225074, 0.00028577107, 0.00021180612, 0.00015565831, 0.00011345188, 8.2033526e-05, 5.8872056e-05,
			4.1962053e-05, 2.973471e-05, 2.0977902e-05 }, //Row:[211]. 83 items, sigma=21.1
		{ 2.2426308e-05, 3.1678049e-05, 4.4552985e-05, 6.230014e-05, 8.6530496e-05, 0.00011929642, 0.00016317973, 0.00022138631, 0.00029784385, 0.00039729776, 0.00052539876, 0.00068877384, 0.00089507064, 0.0011529641, 0.0014721132, 0.0018630557, 0.0023370288, 0.002905707, 0.0035808502, 0.0043738601,
			0.0052952494, 0.0063540339, 0.007557068, 0.0089083474, 0.010408316, 0.012053214, 0.01383452, 0.01573852, 0.017746066, 0.019832549, 0.021968133, 0.02411825, 0.026244387, 0.028305121, 0.030257391, 0.032057955, 0.03366495, 0.035039504, 0.036147289, 0.036959943,
			0.037456279, 0.037623201, 0.037456279, 0.036959943, 0.036147289, 0.035039504, 0.03366495, 0.032057955, 0.030257391, 0.028305121, 0.026244387, 0.02411825, 0.021968133, 0.019832549, 0.017746066, 0.01573852, 0.01383452, 0.012053214, 0.010408316, 0.0089083474,
			0.007557068, 0.0063540339, 0.0052952494, 0.0043738601, 0.0035808502, 0.002905707, 0.0023370288, 0.0018630557, 0.0014721132, 0.0011529641, 0.00089507064, 0.00068877384, 0.00052539876, 0.00039729776, 0.00029784385, 0.00022138631, 0.00016317973, 0.00011929642, 8.6530496e-05, 6.230014e-05,
			4.4552985e-05, 3.1678049e-05, 2.2426308e-05 }, //Row:[212]. 83 items, sigma=21.2
		{ 2.3951423e-05, 3.3717245e-05, 4.7262475e-05, 6.5873148e-05, 9.1202336e-05, 0.00012534899, 0.00017094485, 0.00023124726, 0.00031023422, 0.00041269687, 0.00054432268, 0.00071176112, 0.00092266221, 0.0011856767, 0.0015104066, 0.0019072934, 0.0023874345, 0.0029623172, 0.0036434662, 0.0044420023,
			0.005368117, 0.0064304757, 0.0076355656, 0.0089870181, 0.010484936, 0.012125265, 0.013899263, 0.015793088, 0.017787583, 0.019858264, 0.021975559, 0.024105316, 0.02620957, 0.028247569, 0.030177017, 0.031955497, 0.033541991, 0.034898445, 0.035991279, 0.036792766,
			0.0372822, 0.037446787, 0.0372822, 0.036792766, 0.035991279, 0.034898445, 0.033541991, 0.031955497, 0.030177017, 0.028247569, 0.02620957, 0.024105316, 0.021975559, 0.019858264, 0.017787583, 0.015793088, 0.013899263, 0.012125265, 0.010484936, 0.0089870181,
			0.0076355656, 0.0064304757, 0.005368117, 0.0044420023, 0.0036434662, 0.0029623172, 0.0023874345, 0.0019072934, 0.0015104066, 0.0011856767, 0.00092266221, 0.00071176112, 0.00054432268, 0.00041269687, 0.00031023422, 0.00023124726, 0.00017094485, 0.00012534899, 9.1202336e-05, 6.5873148e-05,
			4.7262475e-05, 3.3717245e-05, 2.3951423e-05 }, //Row:[213]. 83 items, sigma=21.3
		{ 1.7747422e-05, 2.5128218e-05, 3.542756e-05, 4.9666024e-05, 6.9166738e-05, 9.5624739e-05, 0.00013118515, 0.00017852889, 0.00024096357, 0.00032251589, 0.00042802059, 0.00056319925, 0.00073472082, 0.0009502343, 0.0012183628, 0.0015486478, 0.0019514318, 0.0024376697, 0.0030186605, 0.0037056938,
			0.0045096113, 0.0054402875, 0.0065060426, 0.0077130051, 0.0090644512, 0.010560153, 0.012195777, 0.013962365, 0.015845965, 0.017827415, 0.019882362, 0.021981503, 0.024091096, 0.02617372, 0.028189283, 0.030096239, 0.031852978, 0.033419308, 0.034757973, 0.035836122,
			0.036626644, 0.037109304, 0.037271599, 0.037109304, 0.036626644, 0.035836122, 0.034757973, 0.033419308, 0.031852978, 0.030096239, 0.028189283, 0.02617372, 0.024091096, 0.021981503, 0.019882362, 0.017827415, 0.015845965, 0.013962365, 0.012195777, 0.010560153,
			0.0090644512, 0.0077130051, 0.0065060426, 0.0054402875, 0.0045096113, 0.0037056938, 0.0030186605, 0.0024376697, 0.0019514318, 0.0015486478, 0.0012183628, 0.0009502343, 0.00073472082, 0.00056319925, 0.00042802059, 0.00032251589, 0.00024096357, 0.00017852889, 0.00013118515, 9.5624739e-05,
			6.9166738e-05, 4.9666024e-05, 3.542756e-05, 2.5128218e-05, 1.7747422e-05 }, //Row:[214]. 85 items, sigma=21.4
		{ 1.8981812e-05, 2.6784886e-05, 3.7637462e-05, 5.2592323e-05, 7.3009737e-05, 0.00010062654, 0.00013763357, 0.00018676013, 0.00025136289, 0.00033551559, 0.00044409441, 0.00058285245, 0.00075847512, 0.00097860714, 0.0012518406, 0.001587653, 0.0019962849, 0.0024885468, 0.0030755483, 0.0037683441,
			0.0045774988, 0.0055125737, 0.00658155, 0.0077902051, 0.0091414697, 0.010634797, 0.01226558, 0.014024666, 0.015897992, 0.017866407, 0.01990569, 0.021986813, 0.024076439, 0.026137683, 0.028131104, 0.030015893, 0.031751229, 0.033297726, 0.034618906, 0.035682629,
			0.036462384, 0.036938393, 0.03709844, 0.036938393, 0.036462384, 0.035682629, 0.034618906, 0.033297726, 0.031751229, 0.030015893, 0.028131104, 0.026137683, 0.024076439, 0.021986813, 0.01990569, 0.017866407, 0.015897992, 0.014024666, 0.01226558, 0.010634797,
			0.0091414697, 0.0077902051, 0.00658155, 0.0055125737, 0.0045774988, 0.0037683441, 0.0030755483, 0.0024885468, 0.0019962849, 0.001587653, 0.0012518406, 0.00097860714, 0.00075847512, 0.00058285245, 0.00044409441, 0.00033551559, 0.00025136289, 0.00018676013, 0.00013763357, 0.00010062654,
			7.3009737e-05, 5.2592323e-05, 3.7637462e-05, 2.6784886e-05, 1.8981812e-05 }, //Row:[215]. 85 items, sigma=21.5
		{ 2.0282476e-05, 2.8524584e-05, 3.995037e-05, 5.5644994e-05, 7.7005879e-05, 0.00010581144, 0.00014429777, 0.00019524168, 0.00026204768, 0.00034883482, 0.0004605186, 0.00060288103, 0.00078262101, 0.0010073757, 0.0012857028, 0.0016270129, 0.0020414418, 0.0025396535, 0.0031325672, 0.0038310033,
			0.0046452515, 0.0055845639, 0.0066565886, 0.0078667597, 0.0092176716, 0.010708468, 0.012334284, 0.014085776, 0.015948787, 0.017904179, 0.019927873, 0.021991112, 0.024060965, 0.02610108, 0.02807265, 0.029935591, 0.031649855, 0.033176842, 0.034480836, 0.035530386,
			0.036299568, 0.036769048, 0.036926887, 0.036769048, 0.036299568, 0.035530386, 0.034480836, 0.033176842, 0.031649855, 0.029935591, 0.02807265, 0.02610108, 0.024060965, 0.021991112, 0.019927873, 0.017904179, 0.015948787, 0.014085776, 0.012334284, 0.010708468,
			0.0092176716, 0.0078667597, 0.0066565886, 0.0055845639, 0.0046452515, 0.0038310033, 0.0031325672, 0.0025396535, 0.0020414418, 0.0016270129, 0.0012857028, 0.0010073757, 0.00078262101, 0.00060288103, 0.0004605186, 0.00034883482, 0.00026204768, 0.00019524168, 0.00014429777, 0.00010581144,
			7.7005879e-05, 5.5644994e-05, 3.995037e-05, 2.8524584e-05, 2.0282476e-05 }, //Row:[216]. 85 items, sigma=21.6
		{ 2.1651749e-05, 3.0349927e-05, 4.2369148e-05, 5.8827088e-05, 8.1158301e-05, 0.00011118255, 0.00015118061, 0.00020397597, 0.00027301966, 0.00036247435, 0.00047729268, 0.00062328297, 0.00080715469, 0.0010365343, 0.0013199419, 0.0016667179, 0.002086891, 0.0025909769, 0.0031897033, 0.0038936575,
			0.0047128561, 0.0056562464, 0.0067311492, 0.0079426629, 0.0092930551, 0.010781171, 0.012401896, 0.014145709, 0.015998366, 0.017940752, 0.019948932, 0.021994423, 0.024044699, 0.026063928, 0.028013936, 0.029855343, 0.03154886, 0.033056655, 0.034343753, 0.035379379,
			0.036138178, 0.036601247, 0.036756919, 0.036601247, 0.036138178, 0.035379379, 0.034343753, 0.033056655, 0.03154886, 0.029855343, 0.028013936, 0.026063928, 0.024044699, 0.021994423, 0.019948932, 0.017940752, 0.015998366, 0.014145709, 0.012401896, 0.010781171,
			0.0092930551, 0.0079426629, 0.0067311492, 0.0056562464, 0.0047128561, 0.0038936575, 0.0031897033, 0.0025909769, 0.002086891, 0.0016667179, 0.0013199419, 0.0010365343, 0.00080715469, 0.00062328297, 0.00047729268, 0.00036247435, 0.00027301966, 0.00020397597, 0.00015118061, 0.00011118255,
			8.1158301e-05, 5.8827088e-05, 4.2369148e-05, 3.0349927e-05, 2.1651749e-05 }, //Row:[217]. 85 items, sigma=21.7
		{ 2.3091989e-05, 3.2263543e-05, 4.4896655e-05, 6.2141632e-05, 8.5470098e-05, 0.00011674287, 0.00015828487, 0.00021296531, 0.00028428042, 0.00037643479, 0.00049441597, 0.00064405608, 0.00083207221, 0.001066077, 0.0013545499, 0.0017067583, 0.0021326211, 0.002642504, 0.0032469432, 0.0039562929,
			0.0047802997, 0.0057276098, 0.0068052227, 0.0080179091, 0.0093676186, 0.010852908, 0.012468424, 0.014204477, 0.016046745, 0.017976144, 0.019968889, 0.021996769, 0.02402766, 0.026026248, 0.027954975, 0.029775159, 0.031448248, 0.032937162, 0.034207651, 0.035229595,
			0.035978196, 0.03643497, 0.036588514, 0.03643497, 0.035978196, 0.035229595, 0.034207651, 0.032937162, 0.031448248, 0.029775159, 0.027954975, 0.026026248, 0.02402766, 0.021996769, 0.019968889, 0.017976144, 0.016046745, 0.014204477, 0.012468424, 0.010852908,
			0.0093676186, 0.0080179091, 0.0068052227, 0.0057276098, 0.0047802997, 0.0039562929, 0.0032469432, 0.002642504, 0.0021326211, 0.0017067583, 0.0013545499, 0.001066077, 0.00083207221, 0.00064405608, 0.00049441597, 0.00037643479, 0.00028428042, 0.00021296531, 0.00015828487, 0.00011674287,
			8.5470098e-05, 6.2141632e-05, 4.4896655e-05, 3.2263543e-05, 2.3091989e-05 }, //Row:[218]. 85 items, sigma=21.8
		{ 1.722574e-05, 2.420026e-05, 3.3862755e-05, 4.7130434e-05, 6.5186315e-05, 8.9539004e-05, 0.00012209006, 0.00016520791, 0.00022180655, 0.0002954261, 0.00039031124, 0.00051148233, 0.00066479267, 0.00085696414, 0.0010955926, 0.0013891136, 0.0017467189, 0.0021782153, 0.0026938171, 0.0033038682,
			0.004018491, 0.0048471643, 0.0057982377, 0.0068783954, 0.0080920878, 0.0094409557, 0.010923277, 0.012533469, 0.014261686, 0.016093536, 0.018009971, 0.019987359, 0.021997766, 0.024009465, 0.025987652, 0.027895379, 0.029694642, 0.031347616, 0.032817956, 0.034072115,
			0.035080615, 0.035819198, 0.036269792, 0.036421245, 0.036269792, 0.035819198, 0.035080615, 0.034072115, 0.032817956, 0.031347616, 0.029694642, 0.027895379, 0.025987652, 0.024009465, 0.021997766, 0.019987359, 0.018009971, 0.016093536, 0.014261686, 0.012533469,
			0.010923277, 0.0094409557, 0.0080920878, 0.0068783954, 0.0057982377, 0.0048471643, 0.004018491, 0.0033038682, 0.0026938171, 0.0021782153, 0.0017467189, 0.0013891136, 0.0010955926, 0.00085696414, 0.00066479267, 0.00051148233, 0.00039031124, 0.0002954261, 0.00022180655, 0.00016520791,
			0.00012209006, 8.9539004e-05, 6.5186315e-05, 4.7130434e-05, 3.3862755e-05, 2.420026e-05, 1.722574e-05 }, //Row:[219]. 87 items, sigma=21.9
		{ 1.8396978e-05, 2.5762023e-05, 3.5933272e-05, 4.9856396e-05, 6.8747177e-05, 9.4151079e-05, 0.00012801003, 0.00017273535, 0.00023128481, 0.00030724103, 0.00040488703, 0.00052927378, 0.00068627323, 0.00088260925, 0.0011258579, 0.001424408, 0.001787373, 0.0022244453, 0.0027456866, 0.0033612481,
			0.0040810216, 0.0049142207, 0.0058689027, 0.0069514425, 0.0081659775, 0.0095138486, 0.010993064, 0.012597824, 0.014318132, 0.016139537, 0.018043034, 0.020005147, 0.021998218, 0.023990916, 0.025948942, 0.027835943, 0.029614586, 0.031247753, 0.032699816, 0.033937919,
			0.034933209, 0.035661952, 0.036106474, 0.036255876, 0.036106474, 0.035661952, 0.034933209, 0.033937919, 0.032699816, 0.031247753, 0.029614586, 0.027835943, 0.025948942, 0.023990916, 0.021998218, 0.020005147, 0.018043034, 0.016139537, 0.014318132, 0.012597824,
			0.010993064, 0.0095138486, 0.0081659775, 0.0069514425, 0.0058689027, 0.0049142207, 0.0040810216, 0.0033612481, 0.0027456866, 0.0022244453, 0.001787373, 0.001424408, 0.0011258579, 0.00088260925, 0.00068627323, 0.00052927378, 0.00040488703, 0.00030724103, 0.00023128481, 0.00017273535,
			0.00012801003, 9.4151079e-05, 6.8747177e-05, 4.9856396e-05, 3.5933272e-05, 2.5762023e-05, 1.8396978e-05 }, //Row:[220]. 87 items, sigma=22.0
		{ 1.9629732e-05, 2.7400486e-05, 3.8098538e-05, 5.2698173e-05, 7.2447961e-05, 9.8930064e-05, 0.0001341264, 0.00018049046, 0.00024102281, 0.00031934714, 0.00041978304, 0.00054740991, 0.00070811585, 0.00090862391, 0.0011564875, 0.0014600459, 0.0018283315, 0.0022709207, 0.0027977209, 0.0034186909,
			0.0041434928, 0.0049810779, 0.0059392155, 0.007023977, 0.0082391947, 0.0095859178, 0.011061895, 0.012661118, 0.014373449, 0.016184387, 0.018074973, 0.020021893, 0.021997768, 0.023971655, 0.025909755, 0.027776302, 0.029534619, 0.031148281, 0.032582362, 0.033804678,
			0.034786984, 0.035506059, 0.03594462, 0.036092007, 0.03594462, 0.035506059, 0.034786984, 0.033804678, 0.032582362, 0.031148281, 0.029534619, 0.027776302, 0.025909755, 0.023971655, 0.021997768, 0.020021893, 0.018074973, 0.016184387, 0.014373449, 0.012661118,
			0.011061895, 0.0095859178, 0.0082391947, 0.007023977, 0.0059392155, 0.0049810779, 0.0041434928, 0.0034186909, 0.0027977209, 0.0022709207, 0.0018283315, 0.0014600459, 0.0011564875, 0.00090862391, 0.00070811585, 0.00054740991, 0.00041978304, 0.00031934714, 0.00024102281, 0.00018049046,
			0.0001341264, 9.8930064e-05, 7.2447961e-05, 5.2698173e-05, 3.8098538e-05, 2.7400486e-05, 1.9629732e-05 }, //Row:[221]. 87 items, sigma=22.1
		{ 2.0926124e-05, 2.9118025e-05, 4.0361152e-05, 5.5658534e-05, 7.6291524e-05, 0.00010387879, 0.00014044181, 0.00018847555, 0.00025102227, 0.00033174537, 0.00043499916, 0.00056588931, 0.00073031756, 0.00093500347, 0.0011874749, 0.001496019, 0.0018695846, 0.0023176301, 0.0028499077, 0.0034761838,
			0.0042058921, 0.0050477243, 0.0060091661, 0.0070959917, 0.0083117353, 0.0096571631, 0.011129774, 0.012723358, 0.014427649, 0.0162281, 0.018105807, 0.020037619, 0.021996435, 0.023951701, 0.025870109, 0.02771647, 0.02945475, 0.031049204, 0.032465591, 0.033672382,
			0.034641927, 0.035351505, 0.035784209, 0.035929618, 0.035784209, 0.035351505, 0.034641927, 0.033672382, 0.032465591, 0.031049204, 0.02945475, 0.02771647, 0.025870109, 0.023951701, 0.021996435, 0.020037619, 0.018105807, 0.0162281, 0.014427649, 0.012723358,
			0.011129774, 0.0096571631, 0.0083117353, 0.0070959917, 0.0060091661, 0.0050477243, 0.0042058921, 0.0034761838, 0.0028499077, 0.0023176301, 0.0018695846, 0.001496019, 0.0011874749, 0.00093500347, 0.00073031756, 0.00056588931, 0.00043499916, 0.00033174537, 0.00025102227, 0.00018847555,
			0.00014044181, 0.00010387879, 7.6291524e-05, 5.5658534e-05, 4.0361152e-05, 2.9118025e-05, 2.0926124e-05 }, //Row:[222]. 87 items, sigma=22.2
		{ 2.2288297e-05, 3.0917025e-05, 4.2723709e-05, 5.874023e-05, 8.0280689e-05, 0.00010900004, 0.00014695886, 0.00019669282, 0.00026128482, 0.00034443649, 0.00045053509, 0.00058471036, 0.00075287526, 0.00096174315, 0.0012188137, 0.001532319, 0.0019111225, 0.0023645623, 0.0029022349, 0.0035337142,
			0.0042682071, 0.0051141487, 0.0060787453, 0.0071674798, 0.0083835958, 0.0097275848, 0.011196704, 0.012784553, 0.014480745, 0.016270692, 0.018135554, 0.020052345, 0.021994241, 0.023931074, 0.025830019, 0.027656459, 0.029374987, 0.030950525, 0.0323495, 0.033541024,
			0.034498025, 0.035198271, 0.035625223, 0.035768688, 0.035625223, 0.035198271, 0.034498025, 0.033541024, 0.0323495, 0.030950525, 0.029374987, 0.027656459, 0.025830019, 0.023931074, 0.021994241, 0.020052345, 0.018135554, 0.016270692, 0.014480745, 0.012784553,
			0.011196704, 0.0097275848, 0.0083835958, 0.0071674798, 0.0060787453, 0.0051141487, 0.0042682071, 0.0035337142, 0.0029022349, 0.0023645623, 0.0019111225, 0.001532319, 0.0012188137, 0.00096174315, 0.00075287526, 0.00058471036, 0.00045053509, 0.00034443649, 0.00026128482, 0.00019669282,
			0.00014695886, 0.00010900004, 8.0280689e-05, 5.874023e-05, 4.2723709e-05, 3.0917025e-05, 2.2288297e-05 }, //Row:[223]. 87 items, sigma=22.3
		{ 1.6733188e-05, 2.3333741e-05, 3.2415207e-05, 4.4804129e-05, 6.1561321e-05, 8.4033567e-05, 0.00011391186, 0.00015329535, 0.00020475968, 0.00027142724, 0.00035703646, 0.0004660057, 0.00060348665, 0.00077540102, 0.00098845337, 0.0012501124, 0.001568553, 0.0019525504, 0.0024113215, 0.0029543058,
			0.0035908852, 0.0043300413, 0.0051799554, 0.0061475593, 0.0072380502, 0.0084543883, 0.0097967987, 0.011262306, 0.012844328, 0.014532364, 0.016311796, 0.018163848, 0.020065705, 0.02199082, 0.023909406, 0.025789119, 0.027595897, 0.029294953, 0.030851861, 0.032233701,
			0.03341021, 0.034354882, 0.035045958, 0.035467259, 0.035608815, 0.035467259, 0.035045958, 0.034354882, 0.03341021, 0.032233701, 0.030851861, 0.029294953, 0.027595897, 0.025789119, 0.023909406, 0.02199082, 0.020065705, 0.018163848, 0.016311796, 0.014532364,
			0.012844328, 0.011262306, 0.0097967987, 0.0084543883, 0.0072380502, 0.0061475593, 0.0051799554, 0.0043300413, 0.0035908852, 0.0029543058, 0.0024113215, 0.0019525504, 0.001568553, 0.0012501124, 0.00098845337, 0.00077540102, 0.00060348665, 0.0004660057, 0.00035703646, 0.00027142724,
			0.00020475968, 0.00015329535, 0.00011391186, 8.4033567e-05, 6.1561321e-05, 4.4804129e-05, 3.2415207e-05, 2.3333741e-05, 1.6733188e-05 }, //Row:[224]. 89 items, sigma=22.4
		{ 1.7845979e-05, 2.4808395e-05, 3.4358733e-05, 4.7348759e-05, 6.4868274e-05, 8.8296658e-05, 0.00011936064, 0.00016019746, 0.00021342187, 0.00028219465, 0.00037028953, 0.00048215415, 0.00062296, 0.00079863517, 0.0010158728, 0.0012821083, 0.0016054563, 0.0019946024, 0.0024586404, 0.0030068524,
			0.0036484285, 0.0043921268, 0.0052458778, 0.0062163432, 0.0073084406, 0.0085248538, 0.0098655495, 0.011327328, 0.012903435, 0.014583262, 0.016352169, 0.018191451, 0.020078463, 0.021986935, 0.023887461, 0.025748166, 0.02753554, 0.029215401, 0.030753959, 0.032118935,
			0.033280677, 0.034213229, 0.034895293, 0.035311042, 0.035450723, 0.035311042, 0.034895293, 0.034213229, 0.033280677, 0.032118935, 0.030753959, 0.029215401, 0.02753554, 0.025748166, 0.023887461, 0.021986935, 0.020078463, 0.018191451, 0.016352169, 0.014583262,
			0.012903435, 0.011327328, 0.0098655495, 0.0085248538, 0.0073084406, 0.0062163432, 0.0052458778, 0.0043921268, 0.0036484285, 0.0030068524, 0.0024586404, 0.0019946024, 0.0016054563, 0.0012821083, 0.0010158728, 0.00079863517, 0.00062296, 0.00048215415, 0.00037028953, 0.00028219465,
			0.00021342187, 0.00016019746, 0.00011936064, 8.8296658e-05, 6.4868274e-05, 4.7348759e-05, 3.4358733e-05, 2.4808395e-05, 1.7845979e-05 }, //Row:[225]. 89 items, sigma=22.5
		{ 1.9015969e-05, 2.6354045e-05, 3.6389601e-05, 4.9999769e-05, 6.8303367e-05, 9.271225e-05, 0.00012498858, 0.0001673071, 0.00022232085, 0.00029322784, 0.00038383561, 0.00049861926, 0.00064276795, 0.00082221374, 0.001043636, 0.0013144339, 0.0016426602, 0.0020369083, 0.0025061477, 0.0030595026,
			0.0037059719, 0.0044540917, 0.0053115452, 0.0062847283, 0.0073782849, 0.0085946293, 0.009933478, 0.011391416, 0.012961522, 0.014633093, 0.016391469, 0.01821802, 0.020090277, 0.021982246, 0.023864895, 0.025706816, 0.027475039, 0.029135975, 0.030656461, 0.032004839,
			0.033152056, 0.034072692, 0.034745901, 0.035156194, 0.035294033, 0.035156194, 0.034745901, 0.034072692, 0.033152056, 0.032004839, 0.030656461, 0.029135975, 0.027475039, 0.025706816, 0.023864895, 0.021982246, 0.020090277, 0.01821802, 0.016391469, 0.014633093,
			0.012961522, 0.011391416, 0.009933478, 0.0085946293, 0.0073782849, 0.0062847283, 0.0053115452, 0.0044540917, 0.0037059719, 0.0030595026, 0.0025061477, 0.0020369083, 0.0016426602, 0.0013144339, 0.001043636, 0.00082221374, 0.00064276795, 0.00049861926, 0.00038383561, 0.00029322784,
			0.00022232085, 0.0001673071, 0.00012498858, 9.271225e-05, 6.8303367e-05, 4.9999769e-05, 3.6389601e-05, 2.6354045e-05, 1.9015969e-05 }, //Row:[226]. 89 items, sigma=22.6
		{ 2.0245095e-05, 2.7972851e-05, 3.8510174e-05, 5.2759678e-05, 7.1869209e-05, 9.7282946e-05, 0.00013079813, 0.00017462645, 0.00023145834, 0.00030452786, 0.00039767486, 0.00051540006, 0.00066290821, 0.00084613301, 0.0010717375, 0.0013470824, 0.001680156, 0.0020794581, 0.0025538324, 0.0031122448,
			0.0037635036, 0.004515925, 0.0053769478, 0.0063527067, 0.0074475777, 0.0086637127, 0.010000586, 0.011454573, 0.013018599, 0.014681868, 0.016429709, 0.018243573, 0.020101166, 0.02197677, 0.023841726, 0.025665085, 0.027414405, 0.029056685, 0.030559368, 0.031891411,
			0.033024339, 0.03393326, 0.034597765, 0.035002698, 0.035138726, 0.035002698, 0.034597765, 0.03393326, 0.033024339, 0.031891411, 0.030559368, 0.029056685, 0.027414405, 0.025665085, 0.023841726, 0.02197677, 0.020101166, 0.018243573, 0.016429709, 0.014681868,
			0.013018599, 0.011454573, 0.010000586, 0.0086637127, 0.0074475777, 0.0063527067, 0.0053769478, 0.004515925, 0.0037635036, 0.0031122448, 0.0025538324, 0.0020794581, 0.001680156, 0.0013470824, 0.0010717375, 0.00084613301, 0.00066290821, 0.00051540006, 0.00039767486, 0.00030452786,
			0.00023145834, 0.00017462645, 0.00013079813, 9.7282946e-05, 7.1869209e-05, 5.2759678e-05, 3.8510174e-05, 2.7972851e-05, 2.0245095e-05 }, //Row:[227]. 89 items, sigma=22.7
		{ 2.1535308e-05, 2.9666983e-05, 4.0722812e-05, 5.5630991e-05, 7.5568376e-05, 0.0001020113, 0.00013679171, 0.0001821576, 0.00024083592, 0.0003160956, 0.00041180726, 0.00053249545, 0.00068337836, 0.00087038907, 0.0011001719, 0.0013800468, 0.0017179355, 0.002122242, 0.0026016837, 0.0031650676,
			0.0038210123, 0.0045776159, 0.005442076, 0.0064202707, 0.007516314, 0.0087321021, 0.010066874, 0.011516807, 0.013074675, 0.0147296, 0.016466906, 0.018268127, 0.020111148, 0.021970527, 0.023817972, 0.025622985, 0.027353651, 0.028977535, 0.030462683, 0.031778646,
			0.03289752, 0.03379492, 0.03445087, 0.034850536, 0.034984786, 0.034850536, 0.03445087, 0.03379492, 0.03289752, 0.031778646, 0.030462683, 0.028977535, 0.027353651, 0.025622985, 0.023817972, 0.021970527, 0.020111148, 0.018268127, 0.016466906, 0.0147296,
			0.013074675, 0.011516807, 0.010066874, 0.0087321021, 0.007516314, 0.0064202707, 0.005442076, 0.0045776159, 0.0038210123, 0.0031650676, 0.0026016837, 0.002122242, 0.0017179355, 0.0013800468, 0.0011001719, 0.00087038907, 0.00068337836, 0.00053249545, 0.00041180726, 0.0003160956,
			0.00024083592, 0.0001821576, 0.00013679171, 0.0001020113, 7.5568376e-05, 5.5630991e-05, 4.0722812e-05, 2.9666983e-05, 2.1535308e-05 }, //Row:[228]. 89 items, sigma=22.8
		{ 1.6267433e-05, 2.2523018e-05, 3.1073058e-05, 4.2664312e-05, 5.8250636e-05, 7.9037852e-05, 0.00010653424, 0.00014260606, 0.00018953697, 0.00025008953, 0.00032756628, 0.00042586712, 0.00054953859, 0.00070381028, 0.00089461237, 0.0011285681, 0.0014129545, 0.0017556244, 0.0021648848, 0.0026493254,
			0.0032175944, 0.0038781213, 0.0046387882, 0.005506555, 0.0064870476, 0.0075841235, 0.0087994304, 0.010131979, 0.011577756, 0.013129393, 0.014775936, 0.01650271, 0.018291335, 0.020119877, 0.021963168, 0.023793282, 0.025580167, 0.02729242, 0.028898168, 0.030366042,
			0.031666177, 0.032771223, 0.033657296, 0.034304837, 0.034699326, 0.034831828, 0.034699326, 0.034304837, 0.033657296, 0.032771223, 0.031666177, 0.030366042, 0.028898168, 0.02729242, 0.025580167, 0.023793282, 0.021963168, 0.020119877, 0.018291335, 0.01650271,
			0.014775936, 0.013129393, 0.011577756, 0.010131979, 0.0087994304, 0.0075841235, 0.0064870476, 0.005506555, 0.0046387882, 0.0038781213, 0.0032175944, 0.0026493254, 0.0021648848, 0.0017556244, 0.0014129545, 0.0011285681, 0.00089461237, 0.00070381028, 0.00054953859, 0.00042586712,
			0.00032756628, 0.00025008953, 0.00018953697, 0.00014260606, 0.00010653424, 7.9037852e-05, 5.8250636e-05, 4.2664312e-05, 3.1073058e-05, 2.2523018e-05, 1.6267433e-05 }, //Row:[229]. 91 items, sigma=22.9
		{ 1.7326025e-05, 2.3917539e-05, 3.2900593e-05, 4.504436e-05, 6.1328415e-05, 8.2987479e-05, 0.00011156155, 0.00014895078, 0.00019747381, 0.00025992785, 0.00033964788, 0.00044056148, 0.00056723541, 0.0007249086, 0.00091950611, 0.0011576279, 0.0014465056, 0.0017939216, 0.0022080841, 0.0026974543,
			0.0032705214, 0.0039355269, 0.0047001391, 0.0055710832, 0.0065537378, 0.0076517094, 0.0088664038, 0.010196612, 0.011638134, 0.013183471, 0.014821596, 0.016537842, 0.01831392, 0.020128077, 0.021955419, 0.023768381, 0.025537351, 0.02723143, 0.028819297, 0.030270153,
			0.031554707, 0.032646149, 0.033521083, 0.034160357, 0.034549758, 0.034680542, 0.034549758, 0.034160357, 0.033521083, 0.032646149, 0.031554707, 0.030270153, 0.028819297, 0.02723143, 0.025537351, 0.023768381, 0.021955419, 0.020128077, 0.01831392, 0.016537842,
			0.014821596, 0.013183471, 0.011638134, 0.010196612, 0.0088664038, 0.0076517094, 0.0065537378, 0.0055710832, 0.0047001391, 0.0039355269, 0.0032705214, 0.0026974543, 0.0022080841, 0.0017939216, 0.0014465056, 0.0011576279, 0.00091950611, 0.0007249086, 0.00056723541, 0.00044056148,
		0.00033964788, 0.00025992785, 0.00019747381, 0.00014895078, 0.00011156155, 8.2987479e-05, 6.1328415e-05, 4.504436e-05, 3.2900593e-05, 2.3917539e-05, 1.7326025e-05 }, //Row:[230]. 91 items, sigma=23.0
		{ 1.8437895e-05, 2.5377901e-05, 3.4808808e-05, 4.7522332e-05, 6.4523804e-05, 8.7076755e-05, 0.00011675265, 0.00015548506, 0.00020562693, 0.00027000919, 0.00035199793, 0.00045554696, 0.0005852414, 0.00074632751, 0.00094472308, 0.0011870025, 0.00148035, 0.0018324755, 0.0022514873, 0.0027457168,
			0.0033234948, 0.0039928756, 0.0047613157, 0.0056353092, 0.0066199916, 0.0077187247, 0.0089326782, 0.01026043, 0.011697604, 0.013236574, 0.014866249, 0.016571975, 0.018335556, 0.020135423, 0.021946955, 0.023742942, 0.025494208, 0.02717035, 0.028740584, 0.030174676,
			0.03144389, 0.032521948, 0.033385927, 0.034017073, 0.034401473, 0.03453057, 0.034401473, 0.034017073, 0.033385927, 0.032521948, 0.03144389, 0.030174676, 0.028740584, 0.02717035, 0.025494208, 0.023742942, 0.021946955, 0.020135423, 0.018335556, 0.016571975,
			0.014866249, 0.013236574, 0.011697604, 0.01026043, 0.0089326782, 0.0077187247, 0.0066199916, 0.0056353092, 0.0047613157, 0.0039928756, 0.0033234948, 0.0027457168, 0.0022514873, 0.0018324755, 0.00148035, 0.0011870025, 0.00094472308, 0.00074632751, 0.0005852414, 0.00045554696,
			0.00035199793, 0.00027000919, 0.00020562693, 0.00015548506, 0.00011675265, 8.7076755e-05, 6.4523804e-05, 4.7522332e-05, 3.4808808e-05, 2.5377901e-05, 1.8437895e-05 }, //Row:[231]. 91 items, sigma=23.1
		{ 1.9604812e-05, 2.6906072e-05, 3.6799853e-05, 5.0100525e-05, 6.7839189e-05, 9.1308072e-05, 0.00012210983, 0.00016221097, 0.00021399802, 0.00028033462, 0.00036461678, 0.00047082294, 0.00060355483, 0.00076806401, 0.00097025886, 0.001216686, 0.0015144804, 0.0018712777, 0.0022950846, 0.0027941027,
			0.0033765039, 0.0040501568, 0.0048223082, 0.0056992246, 0.006685803, 0.0077851656, 0.0089982531, 0.010323437, 0.011756172, 0.013288711, 0.014909908, 0.016605124, 0.01835626, 0.020141933, 0.021937791, 0.02371698, 0.025450751, 0.027109187, 0.028662036, 0.030079611,
			0.031333723, 0.032398611, 0.033251816, 0.033874971, 0.034254456, 0.034381893, 0.034254456, 0.033874971, 0.033251816, 0.032398611, 0.031333723, 0.030079611, 0.028662036, 0.027109187, 0.025450751, 0.02371698, 0.021937791, 0.020141933, 0.01835626, 0.016605124,
			0.014909908, 0.013288711, 0.011756172, 0.010323437, 0.0089982531, 0.0077851656, 0.006685803, 0.0056992246, 0.0048223082, 0.0040501568, 0.0033765039, 0.0027941027, 0.0022950846, 0.0018712777, 0.0015144804, 0.001216686, 0.00097025886, 0.00076806401, 0.00060355483, 0.00047082294,
			0.00036461678, 0.00028033462, 0.00021399802, 0.00016221097, 0.00012210983, 9.1308072e-05, 6.7839189e-05, 5.0100525e-05, 3.6799853e-05, 2.6906072e-05, 1.9604812e-05 }, //Row:[232]. 91 items, sigma=23.2
		{ 2.0828557e-05, 2.850403e-05, 3.887588e-05, 5.2781225e-05, 7.1276928e-05, 9.5683776e-05, 0.00012763531, 0.00016913047, 0.00022258863, 0.00029090515, 0.00037750465, 0.0004863887, 0.00062217384, 0.00079011496, 0.00099610897, 0.0012466727, 0.0015488897, 0.0019103196, 0.0023388666, 0.0028426017,
			0.0034295382, 0.0041073602, 0.0048831073, 0.0057628214, 0.0067511658, 0.0078510287, 0.009063128, 0.010385636, 0.011813844, 0.013339894, 0.014952586, 0.016637303, 0.018376048, 0.020147622, 0.021927945, 0.023690511, 0.025406993, 0.027047953, 0.028583658, 0.02998496,
			0.031224203, 0.032276131, 0.033118739, 0.033734037, 0.034108689, 0.034234496, 0.034108689, 0.033734037, 0.033118739, 0.032276131, 0.031224203, 0.02998496, 0.028583658, 0.027047953, 0.025406993, 0.023690511, 0.021927945, 0.020147622, 0.018376048, 0.016637303,
			0.014952586, 0.013339894, 0.011813844, 0.010385636, 0.009063128, 0.0078510287, 0.0067511658, 0.0057628214, 0.0048831073, 0.0041073602, 0.0034295382, 0.0028426017, 0.0023388666, 0.0019103196, 0.0015488897, 0.0012466727, 0.00099610897, 0.00079011496, 0.00062217384, 0.0004863887,
			0.00037750465, 0.00029090515, 0.00022258863, 0.00016913047, 0.00012763531, 9.5683776e-05, 7.1276928e-05, 5.2781225e-05, 3.887588e-05, 2.850403e-05, 2.0828557e-05 }, //Row:[233]. 91 items, sigma=23.3
		{ 1.582638e-05, 2.1763096e-05, 2.9825929e-05, 4.0691207e-05, 5.5218869e-05, 7.4491516e-05, 9.9858338e-05, 0.00013298343, 0.00017589763, 0.00023105241, 0.00030137381, 0.0003903138, 0.00050189554, 0.0006407486, 0.00081212929, 0.001021921, 0.0012766086, 0.0015832225, 0.0019492448, 0.0023824759,
			0.0028908558, 0.0034822395, 0.004164128, 0.0049433558, 0.0058257441, 0.0068157265, 0.0079159631, 0.009126955, 0.010446682, 0.01187028, 0.013389782, 0.014993946, 0.016668179, 0.018394589, 0.020152161, 0.021917085, 0.023663201, 0.025362597, 0.026986308, 0.028505108,
			0.029890375, 0.031114979, 0.032154154, 0.032986338, 0.033593909, 0.03396381, 0.034088014, 0.03396381, 0.033593909, 0.032986338, 0.032154154, 0.031114979, 0.029890375, 0.028505108, 0.026986308, 0.025362597, 0.023663201, 0.021917085, 0.020152161, 0.018394589,
			0.016668179, 0.014993946, 0.013389782, 0.01187028, 0.010446682, 0.009126955, 0.0079159631, 0.0068157265, 0.0058257441, 0.0049433558, 0.004164128, 0.0034822395, 0.0028908558, 0.0023824759, 0.0019492448, 0.0015832225, 0.0012766086, 0.001021921, 0.00081212929, 0.0006407486,
			0.00050189554, 0.0003903138, 0.00030137381, 0.00023105241, 0.00017589763, 0.00013298343, 9.9858338e-05, 7.4491516e-05, 5.5218869e-05, 4.0691207e-05, 2.9825929e-05, 2.1763096e-05, 1.582638e-05 }, //Row:[234]. 93 items, sigma=23.4
		{ 1.6834621e-05, 2.3083747e-05, 3.1547266e-05, 4.2921485e-05, 5.8089219e-05, 7.8158761e-05, 0.00010450752, 0.00013882979, 0.00018318777, 0.00024006422, 0.00031241486, 0.0004037177, 0.00051801597, 0.00065995051, 0.00083477713, 0.0010483636, 0.0013071613, 0.0016181451, 0.0019887184, 0.0024265767,
			0.0029395285, 0.0035352714, 0.004221124, 0.0050037186, 0.0058886588, 0.0068801535, 0.0079806395, 0.0091904076, 0.010507251, 0.011926158, 0.01343906, 0.015034676, 0.016698441, 0.018412571, 0.02015624, 0.0219059, 0.023635739, 0.025318251, 0.026924934, 0.028427063,
			0.029796532, 0.03100672, 0.032033344, 0.032855274, 0.033455247, 0.033820478, 0.033943105, 0.033820478, 0.033455247, 0.032855274, 0.032033344, 0.03100672, 0.029796532, 0.028427063, 0.026924934, 0.025318251, 0.023635739, 0.0219059, 0.02015624, 0.018412571,
			0.016698441, 0.015034676, 0.01343906, 0.011926158, 0.010507251, 0.0091904076, 0.0079806395, 0.0068801535, 0.0058886588, 0.0050037186, 0.004221124, 0.0035352714, 0.0029395285, 0.0024265767, 0.0019887184, 0.0016181451, 0.0013071613, 0.0010483636, 0.00083477713, 0.00065995051,
			0.00051801597, 0.0004037177, 0.00031241486, 0.00024006422, 0.00018318777, 0.00013882979, 0.00010450752, 7.8158761e-05, 5.8089219e-05, 4.2921485e-05, 3.1547266e-05, 2.3083747e-05, 1.6834621e-05 }, //Row:[235]. 93 items, sigma=23.5
		{ 1.7892571e-05, 2.4465565e-05, 3.3343273e-05, 4.5242089e-05, 6.1067746e-05, 8.1954163e-05, 0.00010930676, 0.00014484967, 0.00019067585, 0.00024929857, 0.00032370217, 0.00041738944, 0.0005344221, 0.00067945057, 0.00085772825, 0.0010751055, 0.0013379977, 0.0016533233, 0.002028405, 0.0024708328,
			0.0029882833, 0.0035882969, 0.0042780119, 0.0050638603, 0.0059512317, 0.0069441149, 0.0080447286, 0.0092531595, 0.010567021, 0.011981159, 0.013487411, 0.015074459, 0.016727776, 0.018429685, 0.020159546, 0.02189408, 0.023607811, 0.025273639, 0.026863514, 0.028349203,
			0.029703105, 0.030899096, 0.031913369, 0.03272521, 0.033317712, 0.033678349, 0.033799427, 0.033678349, 0.033317712, 0.03272521, 0.031913369, 0.030899096, 0.029703105, 0.028349203, 0.026863514, 0.025273639, 0.023607811, 0.02189408, 0.020159546, 0.018429685,
			0.016727776, 0.015074459, 0.013487411, 0.011981159, 0.010567021, 0.0092531595, 0.0080447286, 0.0069441149, 0.0059512317, 0.0050638603, 0.0042780119, 0.0035882969, 0.0029882833, 0.0024708328, 0.002028405, 0.0016533233, 0.0013379977, 0.0010751055, 0.00085772825, 0.00067945057,
			0.0005344221, 0.00041738944, 0.00032370217, 0.00024929857, 0.00019067585, 0.00014484967, 0.00010930676, 8.1954163e-05, 6.1067746e-05, 4.5242089e-05, 3.3343273e-05, 2.4465565e-05, 1.7892571e-05 }, //Row:[236]. 93 items, sigma=23.6
		{ 1.9001845e-05, 2.5910351e-05, 3.5215913e-05, 4.7655117e-05, 6.4156634e-05, 8.5879919e-05, 0.00011425818, 0.00015104499, 0.00019836348, 0.0002587566, 0.00033523626, 0.0004313287, 0.00055111265, 0.00069924637, 0.00088097901, 0.0011021415, 0.0013691118, 0.0016887497, 0.0020682964, 0.002515235,
			0.0030371104, 0.0036413064, 0.0043347822, 0.0051237724, 0.006013456, 0.0070076058, 0.0081082282, 0.0093152113, 0.010625996, 0.012035289, 0.013534843, 0.015113309, 0.016756198, 0.018445944, 0.020162097, 0.02188164, 0.023579433, 0.025228772, 0.026802056, 0.028271532,
			0.029610093, 0.030792105, 0.03179422, 0.032596137, 0.033181289, 0.03353741, 0.033656965, 0.03353741, 0.033181289, 0.032596137, 0.03179422, 0.030792105, 0.029610093, 0.028271532, 0.026802056, 0.025228772, 0.023579433, 0.02188164, 0.020162097, 0.018445944,
			0.016756198, 0.015113309, 0.013534843, 0.012035289, 0.010625996, 0.0093152113, 0.0081082282, 0.0070076058, 0.006013456, 0.0051237724, 0.0043347822, 0.0036413064, 0.0030371104, 0.002515235, 0.0020682964, 0.0016887497, 0.0013691118, 0.0011021415, 0.00088097901, 0.00069924637,
			0.00055111265, 0.0004313287, 0.00033523626, 0.0002587566, 0.00019836348, 0.00015104499, 0.00011425818, 8.5879919e-05, 6.4156634e-05, 4.7655117e-05, 3.5215913e-05, 2.5910351e-05, 1.9001845e-05 }, //Row:[237]. 93 items, sigma=23.7
		{ 2.0164074e-05, 2.7419911e-05, 3.7167152e-05, 5.0162657e-05, 6.7358041e-05, 8.993819e-05, 0.00011936385, 0.00015741762, 0.00020625218, 0.00026843934, 0.00034701749, 0.00044553505, 0.00056808623, 0.00071933541, 0.0009045257, 0.0011294668, 0.0014004973, 0.001724417, 0.0021083839, 0.002559774,
			0.0030860002, 0.0036942903, 0.004391426, 0.0051834469, 0.0060753252, 0.0070706219, 0.0081711363, 0.0093765638, 0.010684178, 0.012088556, 0.013581366, 0.015151238, 0.016783722, 0.018461366, 0.020163909, 0.021868596, 0.023550617, 0.025183662, 0.026740569, 0.028194054,
			0.029517497, 0.030685743, 0.031675891, 0.032468043, 0.033045966, 0.033397646, 0.033515702, 0.033397646, 0.033045966, 0.032468043, 0.031675891, 0.030685743, 0.029517497, 0.028194054, 0.026740569, 0.025183662, 0.023550617, 0.021868596, 0.020163909, 0.018461366,
			0.016783722, 0.015151238, 0.013581366, 0.012088556, 0.010684178, 0.0093765638, 0.0081711363, 0.0070706219, 0.0060753252, 0.0051834469, 0.004391426, 0.0036942903, 0.0030860002, 0.002559774, 0.0021083839, 0.001724417, 0.0014004973, 0.0011294668, 0.0009045257, 0.00071933541,
			0.00056808623, 0.00044553505, 0.00034701749, 0.00026843934, 0.00020625218, 0.00015741762, 0.00011936385, 8.993819e-05, 6.7358041e-05, 5.0162657e-05, 3.7167152e-05, 2.7419911e-05, 2.0164074e-05 }, //Row:[238]. 93 items, sigma=23.8
		{ 2.1380903e-05, 2.899606e-05, 3.9198951e-05, 5.2766786e-05, 7.0674105e-05, 9.41311e-05, 0.00012462579, 0.00016396935, 0.0002143434, 0.00027834772, 0.00035904615, 0.00046000795, 0.0005853413, 0.00073971507, 0.00092836451, 0.0011570763, 0.0014321479, 0.0017603176, 0.0021486592, 0.0026044409,
			0.0031349433, 0.0037472391, 0.0044479344, 0.0052428761, 0.0061368329, 0.0071331588, 0.0082334511, 0.009437218, 0.010741572, 0.012140965, 0.013626991, 0.015188257, 0.01681036, 0.018475964, 0.020164995, 0.02185496, 0.023521377, 0.02513832, 0.026679059, 0.028116774,
			0.029425319, 0.030580006, 0.031558374, 0.032340917, 0.03291173, 0.033259042, 0.033375626, 0.033259042, 0.03291173, 0.032340917, 0.031558374, 0.030580006, 0.029425319, 0.028116774, 0.026679059, 0.02513832, 0.023521377, 0.02185496, 0.020164995, 0.018475964,
			0.01681036, 0.015188257, 0.013626991, 0.012140965, 0.010741572, 0.009437218, 0.0082334511, 0.0071331588, 0.0061368329, 0.0052428761, 0.0044479344, 0.0037472391, 0.0031349433, 0.0026044409, 0.0021486592, 0.0017603176, 0.0014321479, 0.0011570763, 0.00092836451, 0.00073971507,
			0.0005853413, 0.00046000795, 0.00035904615, 0.00027834772, 0.0002143434, 0.00016396935, 0.00012462579, 9.41311e-05, 7.0674105e-05, 5.2766786e-05, 3.9198951e-05, 2.899606e-05, 2.1380903e-05 }, //Row:[239]. 93 items, sigma=23.9
		{ 1.636953e-05, 2.2301956e-05, 3.0288586e-05, 4.0961241e-05, 5.5117534e-05, 7.3754903e-05, 9.8108699e-05, 0.00012969393, 0.00017034988, 0.00022228645, 0.00028813055, 0.00037097036, 0.00047439469, 0.00060252421, 0.00076003059, 0.00095213949, 0.0011846128, 0.0014637053, 0.0017960921, 0.002188762,
			0.0026488745, 0.0031835783, 0.0037997917, 0.0045039467, 0.0053017003, 0.0061976213, 0.0071948607, 0.0082948191, 0.0094968231, 0.01079783, 0.012192174, 0.013671375, 0.015224027, 0.016835776, 0.018489402, 0.02016502, 0.021840397, 0.023491373, 0.025092404, 0.026617183,
			0.028039344, 0.029333206, 0.030474538, 0.03144131, 0.032214399, 0.032778216, 0.033121233, 0.033236367, 0.033121233, 0.032778216, 0.032214399, 0.03144131, 0.030474538, 0.029333206, 0.028039344, 0.026617183, 0.025092404, 0.023491373, 0.021840397, 0.02016502,
			0.018489402, 0.016835776, 0.015224027, 0.013671375, 0.012192174, 0.01079783, 0.0094968231, 0.0082948191, 0.0071948607, 0.0061976213, 0.0053017003, 0.0045039467, 0.0037997917, 0.0031835783, 0.0026488745, 0.002188762, 0.0017960921, 0.0014637053, 0.0011846128, 0.00095213949,
			0.00076003059, 0.00060252421, 0.00047439469, 0.00037097036, 0.00028813055, 0.00022228645, 0.00017034988, 0.00012969393, 9.8108699e-05, 7.3754903e-05, 5.5117534e-05, 4.0961241e-05, 3.0288586e-05, 2.2301956e-05, 1.636953e-05 }, //Row:[240]. 95 items, sigma=24.0
		{ 1.7377364e-05, 2.3611294e-05, 3.1981706e-05, 4.313837e-05, 5.7899344e-05, 7.7284911e-05, 0.00010255542, 0.00013525255, 0.00017724324, 0.00023076498, 0.00029847095, 0.00038347257, 0.0004893769, 0.00062031559, 0.00078096151, 0.00097652903, 0.0012127535, 0.0014958454, 0.0018324156, 0.0022293664,
			0.0026937484, 0.0032325784, 0.0038526215, 0.0045601369, 0.0053605946, 0.0062583668, 0.0072564063, 0.0083559214, 0.009556063, 0.010853638, 0.012242869, 0.01371521, 0.015259243, 0.016860664, 0.018502376, 0.020164681, 0.021825603, 0.023461301, 0.025046607, 0.02655563,
			0.02796245, 0.029241841, 0.030370019, 0.031325375, 0.032089159, 0.032646096, 0.032984886, 0.033098596, 0.032984886, 0.032646096, 0.032089159, 0.031325375, 0.030370019, 0.029241841, 0.02796245, 0.02655563, 0.025046607, 0.023461301, 0.021825603, 0.020164681,
			0.018502376, 0.016860664, 0.015259243, 0.01371521, 0.012242869, 0.010853638, 0.009556063, 0.0083559214, 0.0072564063, 0.0062583668, 0.0053605946, 0.0045601369, 0.0038526215, 0.0032325784, 0.0026937484, 0.0022293664, 0.0018324156, 0.0014958454, 0.0012127535, 0.00097652903,
			0.00078096151, 0.00062031559, 0.0004893769, 0.00038347257, 0.00029847095, 0.00023076498, 0.00017724324, 0.00013525255, 0.00010255542, 7.7284911e-05, 5.7899344e-05, 4.313837e-05, 3.1981706e-05, 2.3611294e-05, 1.7377364e-05 }, //Row:[241]. 95 items, sigma=24.1
		{ 1.8433128e-05, 2.4979204e-05, 3.3745857e-05, 4.5400899e-05, 6.0782856e-05, 8.0934793e-05, 0.00010714187, 0.00014097212, 0.00018431962, 0.0002394488, 0.00030903818, 0.00039622133, 0.00050462229, 0.0006383822, 0.00080217353, 0.0010011977, 0.0012411618, 0.0015282306, 0.0018689491, 0.0022701327,
			0.0027387224, 0.0032816034, 0.0039053881, 0.0046161655, 0.0054192206, 0.0063187329, 0.0073174608, 0.0084164257, 0.009614608, 0.010908671, 0.012292729, 0.013758174, 0.015293583, 0.016884708, 0.018514569, 0.020163661, 0.02181026, 0.023430842, 0.025000607, 0.026494076,
			0.027885764, 0.029150892, 0.030266114, 0.03121023, 0.031964858, 0.032515024, 0.032849658, 0.032961966, 0.032849658, 0.032515024, 0.031964858, 0.03121023, 0.030266114, 0.029150892, 0.027885764, 0.026494076, 0.025000607, 0.023430842, 0.02181026, 0.020163661,
			0.018514569, 0.016884708, 0.015293583, 0.013758174, 0.012292729, 0.010908671, 0.009614608, 0.0084164257, 0.0073174608, 0.0063187329, 0.0054192206, 0.0046161655, 0.0039053881, 0.0032816034, 0.0027387224, 0.0022701327, 0.0018689491, 0.0015282306, 0.0012411618, 0.0010011977,
			0.00080217353, 0.0006383822, 0.00050462229, 0.00039622133, 0.00030903818, 0.0002394488, 0.00018431962, 0.00014097212, 0.00010714187, 8.0934793e-05, 6.0782856e-05, 4.5400899e-05, 3.3745857e-05, 2.4979204e-05, 1.8433128e-05 }, //Row:[242]. 95 items, sigma=24.2
		{ 1.9538317e-05, 2.6407341e-05, 3.5582838e-05, 4.775074e-05, 6.3770053e-05, 8.4706541e-05, 0.00011186997, 0.00014685439, 0.0001915805, 0.00024833895, 0.00031983274, 0.00040921646, 0.00052012983, 0.00065672205, 0.00082366358, 0.0010261412, 0.0012698324, 0.0015608544, 0.0019056854, 0.0023110529,
			0.0027837878, 0.0033306442, 0.0039580831, 0.0046720245, 0.0054775715, 0.0063787142, 0.007378021, 0.0084763312, 0.0096724599, 0.010962932, 0.012341759, 0.013800278, 0.015327062, 0.01690792, 0.018525997, 0.020161974, 0.021794381, 0.023400008, 0.024954416, 0.026432529,
			0.027809291, 0.02906036, 0.030162818, 0.031095869, 0.031841484, 0.032384989, 0.032715534, 0.032826464, 0.032715534, 0.032384989, 0.031841484, 0.031095869, 0.030162818, 0.02906036, 0.027809291, 0.026432529, 0.024954416, 0.023400008, 0.021794381, 0.020161974,
			0.018525997, 0.01690792, 0.015327062, 0.013800278, 0.012341759, 0.010962932, 0.0096724599, 0.0084763312, 0.007378021, 0.0063787142, 0.0054775715, 0.0046720245, 0.0039580831, 0.0033306442, 0.0027837878, 0.0023110529, 0.0019056854, 0.0015608544, 0.0012698324, 0.0010261412,
			0.00082366358, 0.00065672205, 0.00052012983, 0.00040921646, 0.00031983274, 0.00024833895, 0.0001915805, 0.00014685439, 0.00011186997, 8.4706541e-05, 6.3770053e-05, 4.775074e-05, 3.5582838e-05, 2.6407341e-05, 1.9538317e-05 }, //Row:[243]. 95 items, sigma=24.3
		{ 2.0694439e-05, 2.7897366e-05, 3.7494445e-05, 5.0189796e-05, 6.6862897e-05, 8.8602114e-05, 0.00011674161, 0.00015290105, 0.00019902727, 0.00025743642, 0.00033085503, 0.00042245766, 0.00053589838, 0.00067533303, 0.0008454285, 0.0010513554, 0.0012987598, 0.0015937104, 0.0019426171, 0.0023521186,
			0.002828936, 0.0033796923, 0.0040106979, 0.0047277061, 0.0055356407, 0.0064383057, 0.007438084, 0.0085356372, 0.0097296207, 0.011016426, 0.012389967, 0.01384153, 0.015359689, 0.016930313, 0.018536672, 0.020159635, 0.021777981, 0.02336881, 0.024908041, 0.026370994,
			0.027733033, 0.028970245, 0.03006013, 0.030982284, 0.031719029, 0.032255979, 0.032582502, 0.032692075, 0.032582502, 0.032255979, 0.031719029, 0.030982284, 0.03006013, 0.028970245, 0.027733033, 0.026370994, 0.024908041, 0.02336881, 0.021777981, 0.020159635,
			0.018536672, 0.016930313, 0.015359689, 0.01384153, 0.012389967, 0.011016426, 0.0097296207, 0.0085356372, 0.007438084, 0.0064383057, 0.0055356407, 0.0047277061, 0.0040106979, 0.0033796923, 0.002828936, 0.0023521186, 0.0019426171, 0.0015937104, 0.0012987598, 0.0010513554,
			0.0008454285, 0.00067533303, 0.00053589838, 0.00042245766, 0.00033085503, 0.00025743642, 0.00019902727, 0.00015290105, 0.00011674161, 8.8602114e-05, 6.6862897e-05, 5.0189796e-05, 3.7494445e-05, 2.7897366e-05, 2.0694439e-05 }, //Row:[244]. 95 items, sigma=24.4
		{ 1.5928735e-05, 2.1567671e-05, 2.9115604e-05, 3.9147132e-05, 5.2384616e-05, 6.9727987e-05, 9.2288094e-05, 0.00012142325, 0.0001587784, 0.00020632592, 0.00026640673, 0.00034177003, 0.00043560918, 0.00055159137, 0.0006938776, 0.00086712972, 0.0010765005, 0.0013276033, 0.0016264567, 0.0019794014,
			0.0023929867, 0.0028738233, 0.0034284036, 0.004062889, 0.0047828676, 0.0055930866, 0.0064971675, 0.0074973117, 0.0085940082, 0.0097857572, 0.011068822, 0.012437026, 0.013881606, 0.015391141, 0.016951566, 0.018546274, 0.020156321, 0.021760737, 0.023336925, 0.024861158,
			0.026309144, 0.02765666, 0.02888021, 0.029957709, 0.030869134, 0.031597148, 0.032127647, 0.032450212, 0.032558452, 0.032450212, 0.032127647, 0.031597148, 0.030869134, 0.029957709, 0.02888021, 0.02765666, 0.026309144, 0.024861158, 0.023336925, 0.021760737,
			0.020156321, 0.018546274, 0.016951566, 0.015391141, 0.013881606, 0.012437026, 0.011068822, 0.0097857572, 0.0085940082, 0.0074973117, 0.0064971675, 0.0055930866, 0.0047828676, 0.004062889, 0.0034284036, 0.0028738233, 0.0023929867, 0.0019794014, 0.0016264567, 0.0013276033,
			0.0010765005, 0.00086712972, 0.0006938776, 0.00055159137, 0.00043560918, 0.00034177003, 0.00026640673, 0.00020632592, 0.0001587784, 0.00012142325, 9.2288094e-05, 6.9727987e-05, 5.2384616e-05, 3.9147132e-05, 2.9115604e-05, 2.1567671e-05, 1.5928735e-05 }, //Row:[245]. 97 items, sigma=24.5
		{ 1.6889911e-05, 2.2809989e-05, 3.0714174e-05, 4.119314e-05, 5.4987527e-05, 7.3017688e-05, 9.641682e-05, 0.00012656714, 0.00016513845, 0.00021412813, 0.00027590115, 0.0003532284, 0.00044932098, 0.0005678579, 0.00071300393, 0.00088941434, 0.0011022227, 0.0013570079, 0.0016597373, 0.0020166815,
			0.0024342994, 0.0029190916, 0.0034774203, 0.0041152988, 0.004838152, 0.0056505535, 0.0065559456, 0.0075563521, 0.0086520943, 0.0098415222, 0.011120776, 0.012483592, 0.013921165, 0.015422082, 0.016972342, 0.018555466, 0.020152697, 0.021743312, 0.023305015, 0.024814425,
			0.026247634, 0.027580823, 0.028790908, 0.029856202, 0.030757061, 0.031476481, 0.032000632, 0.032319303, 0.032426231, 0.032319303, 0.032000632, 0.031476481, 0.030757061, 0.029856202, 0.028790908, 0.027580823, 0.026247634, 0.024814425, 0.023305015, 0.021743312,
			0.020152697, 0.018555466, 0.016972342, 0.015422082, 0.013921165, 0.012483592, 0.011120776, 0.0098415222, 0.0086520943, 0.0075563521, 0.0065559456, 0.0056505535, 0.004838152, 0.0041152988, 0.0034774203, 0.0029190916, 0.0024342994, 0.0020166815, 0.0016597373, 0.0013570079,
			0.0011022227, 0.00088941434, 0.00071300393, 0.0005678579, 0.00044932098, 0.0003532284, 0.00027590115, 0.00021412813, 0.00016513845, 0.00012656714, 9.641682e-05, 7.3017688e-05, 5.4987527e-05, 4.119314e-05, 3.0714174e-05, 2.2809989e-05, 1.6889911e-05 }, //Row:[246]. 97 items, sigma=24.6
		{ 1.7895916e-05, 2.4106883e-05, 3.2378702e-05, 4.3318206e-05, 5.7684345e-05, 7.6417841e-05, 0.00010067409, 0.00013185895, 0.00017166666, 0.00022211903, 0.00028560433, 0.00036491421, 0.0004632764, 0.00058438047, 0.00073239356, 0.00091196292, 0.0011282013, 0.0013866521, 0.0016932298, 0.002054134,
			0.0024757329, 0.0029644167, 0.003526418, 0.0041676033, 0.0048932363, 0.0057077195, 0.0066143196, 0.0076148871, 0.0087095797, 0.0098966023, 0.011171977, 0.012529358, 0.0139599, 0.015452204, 0.016992337, 0.018563945, 0.02014846, 0.021725403, 0.023272774, 0.024767536,
			0.026186156, 0.02750521, 0.02870202, 0.029755291, 0.030645745, 0.031356703, 0.031874607, 0.032189446, 0.032295082, 0.032189446, 0.031874607, 0.031356703, 0.030645745, 0.029755291, 0.02870202, 0.02750521, 0.026186156, 0.024767536, 0.023272774, 0.021725403,
			0.02014846, 0.018563945, 0.016992337, 0.015452204, 0.0139599, 0.012529358, 0.011171977, 0.0098966023, 0.0087095797, 0.0076148871, 0.0066143196, 0.0057077195, 0.0048932363, 0.0041676033, 0.003526418, 0.0029644167, 0.0024757329, 0.002054134, 0.0016932298, 0.0013866521,
			0.0011282013, 0.00091196292, 0.00073239356, 0.00058438047, 0.0004632764, 0.00036491421, 0.00028560433, 0.00022211903, 0.00017166666, 0.00013185895, 0.00010067409, 7.6417841e-05, 5.7684345e-05, 4.3318206e-05, 3.2378702e-05, 2.4106883e-05, 1.7895916e-05 }, //Row:[247]. 97 items, sigma=24.7
		{ 1.8948121e-05, 2.5459869e-05, 3.4110834e-05, 4.5524084e-05, 6.0476891e-05, 7.9930284e-05, 0.0001050617, 0.00013730033, 0.00017836445, 0.00023029969, 0.00029551686, 0.00037682745, 0.00047747472, 0.00060115751, 0.000752044, 0.00093477194, 0.0011544319, 0.0014165302, 0.0017269276, 0.0020917516,
			0.0025172794, 0.0030097905, 0.0035753887, 0.0042197948, 0.0049481136, 0.005764579, 0.0066722856, 0.0076729145, 0.0087664644, 0.009951, 0.01122243, 0.012574331, 0.013997823, 0.015481521, 0.017011564, 0.018571725, 0.020143624, 0.021707021, 0.023240214, 0.024720499,
			0.026124714, 0.027429825, 0.028613547, 0.029654971, 0.030535177, 0.031237806, 0.031749561, 0.032060629, 0.032164995, 0.032060629, 0.031749561, 0.031237806, 0.030535177, 0.029654971, 0.028613547, 0.027429825, 0.026124714, 0.024720499, 0.023240214, 0.021707021,
			0.020143624, 0.018571725, 0.017011564, 0.015481521, 0.013997823, 0.012574331, 0.01122243, 0.009951, 0.0087664644, 0.0076729145, 0.0066722856, 0.005764579, 0.0049481136, 0.0042197948, 0.0035753887, 0.0030097905, 0.0025172794, 0.0020917516, 0.0017269276, 0.0014165302,
			0.0011544319, 0.00093477194, 0.000752044, 0.00060115751, 0.00047747472, 0.00037682745, 0.00029551686, 0.00023029969, 0.00017836445, 0.00013730033, 0.0001050617, 7.9930284e-05, 6.0476891e-05, 4.5524084e-05, 3.4110834e-05, 2.5459869e-05, 1.8948121e-05 }, //Row:[248]. 97 items, sigma=24.8
		{ 2.0047912e-05, 2.687047e-05, 3.5912219e-05, 4.7812519e-05, 6.3366969e-05, 8.3556825e-05, 0.00010958138, 0.00014289287, 0.00018523315, 0.00023867109, 0.00030563925, 0.00038896804, 0.0004919151, 0.00061818736, 0.00077195264, 0.00095783782, 0.0011809099, 0.0014466368, 0.0017608243, 0.0021295272,
			0.002558931, 0.0030552049, 0.0036243243, 0.0042718659, 0.0050027772, 0.0058211266, 0.0067298397, 0.0077304325, 0.008822749, 0.010004718, 0.01127214, 0.012618518, 0.014034941, 0.015510044, 0.017030036, 0.01857882, 0.0201382, 0.02168818, 0.023207344, 0.024673323,
			0.026063315, 0.02735467, 0.028525488, 0.029555238, 0.030425352, 0.031119779, 0.031625482, 0.03193284, 0.032035955, 0.03193284, 0.031625482, 0.031119779, 0.030425352, 0.029555238, 0.028525488, 0.02735467, 0.026063315, 0.024673323, 0.023207344, 0.02168818,
			0.0201382, 0.01857882, 0.017030036, 0.015510044, 0.014034941, 0.012618518, 0.01127214, 0.010004718, 0.008822749, 0.0077304325, 0.0067298397, 0.0058211266, 0.0050027772, 0.0042718659, 0.0036243243, 0.0030552049, 0.002558931, 0.0021295272, 0.0017608243, 0.0014466368,
			0.0011809099, 0.00095783782, 0.00077195264, 0.00061818736, 0.0004919151, 0.00038896804, 0.00030563925, 0.00023867109, 0.00018523315, 0.00014289287, 0.00010958138, 8.3556825e-05, 6.3366969e-05, 4.7812519e-05, 3.5912219e-05, 2.687047e-05, 2.0047912e-05 }, //Row:[249]. 97 items, sigma=24.9
		{ 1.5510419e-05, 2.087688e-05, 2.8020413e-05, 3.74647e-05, 4.9865445e-05, 6.6036563e-05, 8.6979443e-05, 0.00011391504, 0.00014831833, 0.00019195426, 0.00024691435, 0.00031565213, 0.00040101597, 0.00050627683, 0.00063514845, 0.000791797, 0.00098083708, 0.0012073108, 0.0014766463, 0.0017945936,
			0.0021671338, 0.0026003603, 0.0031003322, 0.0036728973, 0.0043234894, 0.0050569009, 0.0058770372, 0.0067866587, 0.0077871198, 0.0088781142, 0.01005744, 0.011320793, 0.012661607, 0.014070945, 0.015537463, 0.017047444, 0.018584921, 0.020131883, 0.021668571, 0.023173856,
			0.024625695, 0.026001644, 0.027279427, 0.028437523, 0.02945577, 0.030315943, 0.031002295, 0.03150204, 0.031805746, 0.031907631, 0.031805746, 0.03150204, 0.031002295, 0.030315943, 0.02945577, 0.028437523, 0.027279427, 0.026001644, 0.024625695, 0.023173856,
			0.021668571, 0.020131883, 0.018584921, 0.017047444, 0.015537463, 0.014070945, 0.012661607, 0.011320793, 0.01005744, 0.0088781142, 0.0077871198, 0.0067866587, 0.0058770372, 0.0050569009, 0.0043234894, 0.0036728973, 0.0031003322, 0.0026003603, 0.0021671338, 0.0017945936,
			0.0014766463, 0.0012073108, 0.00098083708, 0.000791797, 0.00063514845, 0.00050627683, 0.00040101597, 0.00031565213, 0.00024691435, 0.00019195426, 0.00014831833, 0.00011391504, 8.6979443e-05, 6.6036563e-05, 4.9865445e-05, 3.74647e-05, 2.8020413e-05, 2.087688e-05, 1.5510419e-05 } //Row:[250]. 99 items, sigma=25.0
	};

	static const float gaussianOffsets[99] = { -49.0f, -48.0f, -47.0f, -46.0f, -45.0f, -44.0f, -43.0f, -42.0f, -41.0f, -40.0f, -39.0f, -38.0f, -37.0f, -36.0f, -35.0f, -34.0f, -33.0f, -32.0f, -31.0f, -30.0f, -29.0f, -28.0f, -27.0f, -26.0f, -25.0f,
		-24.0f, -23.0f, -22.0f, -21.0f, -20.0f, -19.0f, -18.0f, -17.0f, -16.0f, -15.0f, -14.0f, -13.0f, -12.0f, -11.0f, -10.0f, -9.0f, -8.0f, -7.0f, -6.0f, -5.0f, -4.0f, -3.0f, -2.0f, -1.0f, 0.0f,
		1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f,
		26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f, 36.0f, 37.0f, 38.0f, 39.0f, 40.0f, 41.0f, 42.0f, 43.0f, 44.0f, 45.0f, 46.0f, 47.0f, 48.0f, 49.0f
	};



	int row = int(sigma * 10.0f);
	if (row > 250) { row = 250; }
	if (row < 0) { row = 0; }

	int n = numberOfValuesPerRow[row];
	int offsetFromStart = (99 - n) / 2;

	//copies to the array used to send to GPU
	for (int i = 0; i < n; i++) {
		offsets[i] = gaussianOffsets[offsetFromStart + i];
		weights[i] = gaussianMatrix[row][i];
	}

	return n;
}


float HeatmapLayer::ReadPixelsAndFindMax(int width, int height)
{
	float* data = (float*)malloc(sizeof(float) * width * height);
	glReadPixels(0, 0, width, height, GL_RED, GL_FLOAT, data);

	float maxVal = 0.0;
	for (int i = 0; i < width * height; i++)
	{
		if (data[i] > maxVal) maxVal = data[i];
	}
	free(data);
	return maxVal;

}

float HeatmapLayer::FindMaxValueWithReductionShader(int width, int height, int reductionFactor)	//this method currently loses some of the edge due to rounding
{
	float returnMaxVal=500.0;
	
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
		smallerWidth = smallerWidth / reductionFactor;
		smallerHeight = smallerHeight / reductionFactor;

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

		returnMaxVal = ReadPixelsAndFindMax(smallerWidth, smallerHeight);
		//printf("Max val from pass %i: %f.\n", passno, returnMaxVal);

	}
	returnMaxVal = ReadPixelsAndFindMax(smallerWidth, smallerHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return returnMaxVal;
}
