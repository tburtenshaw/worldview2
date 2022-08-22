// Project1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define IMGUI_IMPL_OPENGL_LOADER_GLEW
//#define IMGUI_IMPL_OPENGL_LOADER_GLAD

#include <iostream>
#include <tchar.h>
#include <Windows.h>
#include <vector>
#include <thread>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "shaders.h"
#include "header.h"
#include "nswe.h"
#include "loadjson.h"
#include "processlocations.h"
#include "input.h"
#include "heatmap.h"
#include "regions.h"
#include "gui.h"
#include "highresmanager.h"
#include "palettes.h"
#include "mygl.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <filesystem>
#include <string>

LocationHistory* pLocationHistory;


FrameBufferObjectInfo fboInfo;
BackgroundInfo backgroundLayer;
MapPathInfo pathInfo;
MapPointsInfo pointsInfo;
DisplayRegionsLayer regionsInfo;



int CloseLocationFile(LocationHistory* lh)	//closes the file, empties the arrays, resets names/counts
{
	//this should probably be method of LocationHistory
	if (lh->isLoadingFile) {
		return 2;
	}

	lh->isFileChosen = false;
	lh->isFullyLoaded = true;	//we're loaded with nothing
	lh->isInitialised = false;
	lh->isLoadingFile = false;

	if (!lh->locations.empty()) {
		lh->locations.clear();
	}
	if (!lh->pathPlotLocations.empty()) {
		lh->pathPlotLocations.clear();
	}

	//lh->filename = L"";	//can't do this, as it's where the file to load is stored.
	lh->filesize = 0;

	lh->heatmap->MakeDirty();
	
	return 0;
}

int OpenAndReadLocationFile(LocationHistory* lh)
{
	HANDLE hLocationFile;

	if (lh->isLoadingFile) {	//we don't want to load if we're already loading something
		return 2;
	}

	hLocationFile = CreateFile(lh->filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if (hLocationFile == INVALID_HANDLE_VALUE) {
		lh->isFileChosen = false;
		lh->isLoadingFile = false;
		lh->isFullyLoaded = false;
		return 1;
	}

	CloseLocationFile(lh);	//close any existing file
	lh->isLoadingFile = true;
	lh->isFileChosen = true;
	lh->isInitialised = false;


	LARGE_INTEGER filesize;
	GetFileSizeEx(hLocationFile, &filesize);
	lh->filesize = filesize.QuadPart;
	printf("File size: %i\n", lh->filesize);


	std::string extension = std::filesystem::path(lh->filename).extension().string();
	if (extension == ".json") {
		LoadJsonFile(lh, hLocationFile);
	}
	else if (extension == ".wvf") {
		LoadWVFormat(lh, hLocationFile);
	}
	CloseHandle(hLocationFile);

	SortAndCalculateEarliestAndLatest(lh);

	CreatePathPlotLocations(lh);

	lh->isLoadingFile = false;
	lh->isFullyLoaded = true;
	lh->isInitialised = false;
	lh->isFileChosen = false;
	return 0;
}





int SaveWVFormat(LocationHistory* lh, std::wstring filename)
{
	HANDLE hFile;
	DWORD numberOfLocations;
	DWORD bytesWritten;

	static const char* const magic = "WVF1";

	//copy whole thing at once.
	numberOfLocations = lh->locations.size();
	WVFormat* flatArray = new WVFormat[numberOfLocations];

	for (DWORD i = 0; i < numberOfLocations; i++) {
		flatArray[i].timestamp = lh->locations[i].timestamp;
		flatArray[i].lon = lh->locations[i].longitude;
		flatArray[i].lat = lh->locations[i].latitude;
	}

	hFile = CreateFile(filename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	WriteFile(hFile, magic, 4, &bytesWritten, NULL);
	WriteFile(hFile, &numberOfLocations, sizeof(numberOfLocations), &bytesWritten, NULL);
	WriteFile(hFile, flatArray, numberOfLocations * sizeof(WVFormat), &bytesWritten, NULL);

	delete[] flatArray;
	CloseHandle(hFile);

	return 0;
}



int StartGLProgram(LocationHistory* lh)
{
	GlobalOptions* options;
	options = lh->globalOptions;

	// start GL context and O/S window using the GLFW helper library
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return 1;
	}

	lh->windowDimensions = { 1200,800 };
	
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(lh->windowDimensions.width, lh->windowDimensions.height, "World Tracker", NULL, NULL);
	if (!window) {
		fprintf(stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return 1;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// get version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString(GL_VERSION); // version as a string
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	const char* glsl_version = "#version 330";
	ImGui_ImplOpenGL3_Init(glsl_version);
	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	//	ImFont* pFont1 = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 16);
	ImFont* pFont4 = io.Fonts->AddFontFromFileTTF("C:\\Users\\GGPC\\AppData\\Local\\Microsoft\\Windows\\Fonts\\OpenSans-Regular.ttf", 16);



	glViewport(0, 0, lh->windowDimensions.width, lh->windowDimensions.height);
	glfwSetKeyCallback(window, key_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetFramebufferSizeCallback(window, size_callback);

	glDisable(GL_DEPTH_TEST); //we're not doing 3d
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glBlendEquation(GL_FUNC_ADD);
	

	//set up the background
	backgroundLayer.SetupSquareVertices();
	LoadBackgroundImageToTexture(&backgroundLayer.worldTexture);
	MakeHighresImageTexture(&lh->highres->highresTexture);
	MakeHeatmapTexture(lh->viewNSWE, &backgroundLayer.heatmapTexture);

	//set up and compile the shaders
	backgroundLayer.SetupShaders();
	pathInfo.SetupShaders();
	pointsInfo.SetupShaders();

	//Set up the FBO that we draw onto
	fboInfo.SetupFrameBufferObject(lh->windowDimensions.width, lh->windowDimensions.height);

	//initial values
	lh->viewNSWE->target.setvalues(-36.83, -37.11, 174.677 - 0.0, 174.961 - 0.0);
	lh->viewNSWE->target.makeratio((float)lh->windowDimensions.height / (float)lh->windowDimensions.width);
	lh->viewNSWE->setvalues(lh->viewNSWE->target);
	lh->viewNSWE->movetowards(1000000000000.0);

	//make a new region, which is the viewport
	lh->regions.push_back(new Region());

	regionsInfo.displayRegions.resize(1);
	regionsInfo.SetupVertices();
	regionsInfo.SetupShaders();
	
	//Trying to do better, GPU based, heatmap
	printf("Start: glGetError %i\n", glGetError());
	
	unsigned int HeatmapFBO;
	glGenFramebuffers(1, &HeatmapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, HeatmapFBO);

	unsigned int NewHeatmapTexture;
	glGenTextures(1, &NewHeatmapTexture);
	glBindTexture(GL_TEXTURE_2D, NewHeatmapTexture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, lh->windowDimensions.width, lh->windowDimensions.height, 0, GL_RED, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, NewHeatmapTexture, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	printf("After unbind: glGetError %i\n", glGetError());


	//MAIN LOOP
	while (!glfwWindowShouldClose(window)) {
		if (!io.WantCaptureMouse) {	//if Imgui doesn't want the mouse
			ManageMouseMoveClickAndDrag(window, lh);
		}

		options->seconds = (float)glfwGetTime();

		//get the view moving towards the target
		lh->viewNSWE->movetowards(lh->globalOptions->seconds);

		if (pLocationHistory->isFileChosen && !pLocationHistory->isLoadingFile) {
			std::thread loadingthread(OpenAndReadLocationFile, pLocationHistory);
			loadingthread.detach();
		}

		if ((lh->isInitialised == false) && (lh->isFullyLoaded) && (lh->isLoadingFile == false)) {
			printf("Initialising things that need file to be fully loaded\n");
			pathInfo.SetupVertices(lh->pathPlotLocations);
			pointsInfo.SetupVertices(lh->pathPlotLocations);
			lh->heatmap->MakeDirty();
			lh->isInitialised = true;
		}

		glClear(GL_COLOR_BUFFER_BIT);


		//try to do NewHeatmap rendering
		glBlendFunc(GL_ONE, GL_ONE);

	//	glBindFramebuffer(GL_FRAMEBUFFER, fboInfo.fbo);
	//	glDrawArrays(GL_LINE_STRIP, 0, pLocationHistory->pathPlotLocations.size());
	//	glBindFramebuffer(GL_FRAMEBUFFER, 0);



		//now back to normal
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glBlendEquation(GL_FUNC_ADD);


		//Set the FBO as the draw surface
		fboInfo.BindToDrawTo();
		DrawBackgroundAndHeatmap(lh);

		//We only draw the points if everything is loaded and initialised.
		if (lh->isInitialised && lh->isFullyLoaded) {
			if (lh->viewNSWE->isDirty()) {
				lh->regions[0]->SetNSWE(&lh->viewNSWE->target);
				lh->regions[0]->Populate(lh);
				lh->heatmap->MakeDirty();
			}

			if (!lh->viewNSWE->isMoving() && lh->heatmap->IsDirty() && lh->globalOptions->showHeatmap) {
				//NSWE expanded;
				//expanded = lh->viewNSWE->target.createExpandedBy(1.0);
				UpdateHeatmapTexture(&lh->viewNSWE->target, &backgroundLayer);
				lh->heatmap->MakeClean();
			}

			if (options->showPaths) {
				if (options->regenPathColours) {
					printf("regen pathplot\n");
					ColourPathPlot(lh);
					glBindBuffer(GL_ARRAY_BUFFER, pathInfo.vbo);
					glBufferSubData(GL_ARRAY_BUFFER, 0, pLocationHistory->pathPlotLocations.size() * sizeof(PathPlotLocation), &pLocationHistory->pathPlotLocations.front());
				}
				pathInfo.Draw(pLocationHistory->pathPlotLocations, lh->windowDimensions.width, lh->windowDimensions.height, lh->viewNSWE, options->linewidth, options->seconds, options->cycleSeconds);
			}

			if (options->showPoints) {
				DrawPoints(&pointsInfo);
			}

			regionsInfo.UpdateFromRegionsData(lh->regions);
			regionsInfo.Draw(lh->windowDimensions.width, lh->windowDimensions.height, lh->viewNSWE);

		}

				//Start the ImGui Frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		//draw the FBO onto the screen
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		fboInfo.Draw(lh->windowDimensions.width, lh->windowDimensions.height);

		if (lh->isLoadingFile == false) {
			Gui::MakeGUI(lh);	//make the ImGui stuff
		}
		else if (lh->isLoadingFile == true) {
			Gui::ShowLoadingWindow(lh);
		}

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (!lh->viewNSWE->isMoving())	glfwWaitEventsTimeout(1.0f / 60.0f);	//this runs constantly, but max of ~60fps
		else {
			glfwPollEvents();
		}

		glfwSwapBuffers(window);
	}

	//ImGui close
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	// close GL context and any other GLFW resources
	glfwTerminate();
	return 0;
}


void LoadBackgroundImageToTexture(unsigned int* texture)
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

	glGenTextures(1, texture);
	glBindTexture(GL_TEXTURE_2D, *texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	stbi_image_free(data);

	return;
}

void MakeHighresImageTexture(unsigned int* texture)
{
	GLint maxtexturesize;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxtexturesize);
	printf("Max texture size: %i\n", maxtexturesize);

	glGenTextures(1, texture);
	glBindTexture(GL_TEXTURE_2D, *texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 8192, 8192, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glGenerateMipmap(GL_TEXTURE_2D);

	return;
}

void MakeHeatmapTexture(NSWE* nswe, unsigned int* texture)
{
	glGenTextures(1, texture);
	glBindTexture(GL_TEXTURE_2D, *texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, pLocationHistory->heatmap->width, pLocationHistory->heatmap->height, 0, GL_RED, GL_FLOAT, NULL);

	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);	//we don't wrap this at the moment, as funny things happen when zooming
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);	//this is the poles
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return;
}

void UpdateHeatmapTexture(NSWE* nswe, BackgroundInfo* backgroundInfo)
{
	pLocationHistory->heatmap->CreateHeatmap(nswe, 0);

	//printf("Updated texture\n");
	glBindTexture(GL_TEXTURE_2D, backgroundInfo->heatmapTexture);
	//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1800, 1800, GL_RED, GL_FLOAT, pLocationHistory->heatmap->pixel);
	//**FIX
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, pLocationHistory->heatmap->width, pLocationHistory->heatmap->height, 0, GL_RED, GL_FLOAT, NULL);	//only need to do this if size changed
	//we can probably just adjust the zooming in the shader, rather than resizing the texture

	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, pLocationHistory->heatmap->width, pLocationHistory->heatmap->height, GL_RED, GL_FLOAT, pLocationHistory->heatmap->pixel);
	glGenerateMipmap(GL_TEXTURE_2D);

	return;
}

void DrawBackgroundAndHeatmap(LocationHistory* lh)
{
	NSWE* viewnswe;
	NSWE* heatmapnswe;
	BackgroundInfo* backgroundInfo;
	HighResManager* highres;
	NSWE* highresnswe;

	backgroundInfo = &backgroundLayer;
	viewnswe = pLocationHistory->viewNSWE;
	heatmapnswe = pLocationHistory->heatmap->nswe;
	highres = lh->highres;

	highres->DecideBestTex(lh->windowDimensions, lh->viewNSWE);
	highresnswe = highres->GetBestNSWE();

	glBindBuffer(GL_ARRAY_BUFFER, backgroundInfo->vbo);

	backgroundInfo->shader.UseMe();
	//backgroundInfo->shader.SetUniformFromFloats("seconds", seconds);
	backgroundInfo->shader.SetUniformFromFloats("resolution", (float)pLocationHistory->windowDimensions.width, (float)pLocationHistory->windowDimensions.height);
	backgroundInfo->shader.SetUniformFromFloats("nswe", viewnswe->north, viewnswe->south, viewnswe->west, viewnswe->east);
	backgroundInfo->shader.SetUniformFromNSWE("highresnswe", highresnswe);
	backgroundInfo->shader.SetUniformFromFloats("highresscale", (float)highres->width / 8192.0f, (float)highres->height / 8192.0f); //as we're just loading the
	backgroundInfo->shader.SetUniformFromNSWE("heatmapnswe", heatmapnswe);

	backgroundInfo->shader.SetUniformFromFloats("maxheatmapvalue", pLocationHistory->heatmap->maxPixel);
	backgroundInfo->shader.SetUniformFromInts("palette", pLocationHistory->globalOptions->palette);

	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, backgroundInfo->worldTexture);

	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, highres->highresTexture);

	glActiveTexture(GL_TEXTURE0 + 2);
	glBindTexture(GL_TEXTURE_2D, backgroundInfo->heatmapTexture);

	glBindVertexArray(backgroundInfo->vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindVertexArray(0);
	return;
}

void DrawPoints(MapPointsInfo* mapPointsInfo)
{
	//update uniform shader variables
	mapPointsInfo->shader.UseMe();
	mapPointsInfo->shader.SetUniform(mapPointsInfo->uniformNswe, pLocationHistory->viewNSWE);
	mapPointsInfo->shader.SetUniform(mapPointsInfo->uniformResolution, (float)pLocationHistory->windowDimensions.width, (float)pLocationHistory->windowDimensions.height);
	mapPointsInfo->shader.SetUniform(mapPointsInfo->uniformPointRadius, pLocationHistory->globalOptions->pointdiameter / 2.0f);
	mapPointsInfo->shader.SetUniform(mapPointsInfo->uniformPointAlpha, pLocationHistory->globalOptions->pointalpha);
	mapPointsInfo->shader.SetUniform(mapPointsInfo->uniformSeconds, pLocationHistory->globalOptions->seconds);
	mapPointsInfo->shader.SetUniform(mapPointsInfo->uniformCycleSeconds, pLocationHistory->globalOptions->cycleSeconds);

	mapPointsInfo->shader.SetUniform(mapPointsInfo->uniformEarliestTimeToShow, pLocationHistory->globalOptions->earliestTimeToShow);
	mapPointsInfo->shader.SetUniform(mapPointsInfo->uniformLatestTimeToShow, pLocationHistory->globalOptions->latestTimeToShow);

	mapPointsInfo->shader.SetUniform(mapPointsInfo->uniformShowHighlights, pLocationHistory->globalOptions->showHighlights);
	mapPointsInfo->shader.SetUniform(mapPointsInfo->uniformSecondsBetweenHighlights, pLocationHistory->globalOptions->secondsbetweenhighlights);
	mapPointsInfo->shader.SetUniform(mapPointsInfo->uniformTravelTimeBetweenHighlights, pLocationHistory->globalOptions->minutestravelbetweenhighlights * 60.0f);

	mapPointsInfo->shader.SetUniform(mapPointsInfo->uniformColourBy, pLocationHistory->globalOptions->colourby);

	switch (pLocationHistory->globalOptions->colourby) {
	case 1:
		Palette_Handler::FillShaderPalette(mapPointsInfo->palette, 24, pLocationHistory->globalOptions->indexPaletteHour);
		mapPointsInfo->shader.SetUniform(mapPointsInfo->uniformPalette, 24, &mapPointsInfo->palette[0][0]);
		break;
	case 4:
		Palette_Handler::FillShaderPalette(mapPointsInfo->palette, 24, pLocationHistory->globalOptions->indexPaletteYear);
		mapPointsInfo->shader.SetUniform(mapPointsInfo->uniformPalette, 24, &mapPointsInfo->palette[0][0]);
		break;
	default:
		Palette_Handler::FillShaderPalette(mapPointsInfo->palette, 24, pLocationHistory->globalOptions->indexPaletteWeekday);
		mapPointsInfo->shader.SetUniform(mapPointsInfo->uniformPalette, 24, &mapPointsInfo->palette[0][0]);
		break;
	}

	glBindBuffer(GL_ARRAY_BUFFER, mapPointsInfo->vbo);
	glBindVertexArray(mapPointsInfo->vao);
	glDrawArrays(GL_POINTS, 0, pLocationHistory->pathPlotLocations.size());

	return;
}


void DisplayIfGLError(const char* message, bool alwaysshow)
{
	int e = glGetError();
	if (e || alwaysshow) {
		printf("%s glGetError %i\n", message, e);
	}

	return;
}

int main(int argc, char** argv)
{
	LocationHistory locationHistory;
	pLocationHistory = &locationHistory;

	pLocationHistory->isFileChosen = true;
	if (pLocationHistory->isFileChosen && !pLocationHistory->isLoadingFile) {
		std::thread loadingthread(OpenAndReadLocationFile, pLocationHistory);
		loadingthread.detach();
	}

	StartGLProgram(pLocationHistory);

	return 0;
}