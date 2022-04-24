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

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <filesystem>
#include <string>

//using namespace std;

LocationHistory* pLocationHistory;

int OpenAndReadLocationFile(LocationHistory* lh)
{
	HANDLE hLocationFile;

	if (lh->isLoadingFile) {	//we don't want to load if we're already loading something
		return 2;
	}

	lh->isLoadingFile = true;
	lh->isFileChosen = true;
	lh->isInitialised = false;

	hLocationFile = CreateFile(lh->filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if (hLocationFile == INVALID_HANDLE_VALUE) {
		lh->isFileChosen = false;
		lh->isLoadingFile = false;
		lh->isFullyLoaded = false;
		return 1;
	}

	if (!lh->locations.empty()) {
		lh->locations.clear();
	}

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

	
	CalculateEarliestAndLatest(lh);
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
	//glfwWindowHint(GLFW_SAMPLES, 4);
	//glEnable(GL_MULTISAMPLE);

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
	//	ImFont* pFont2 = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 24);

	//	ImFont* pFont3 = io.Fonts->AddFontFromFileTTF("C:\\Users\\GGPC\\AppData\\Local\\Microsoft\\Windows\\Fonts\\OpenSans-Light.ttf", 18);
	ImFont* pFont4 = io.Fonts->AddFontFromFileTTF("C:\\Users\\GGPC\\AppData\\Local\\Microsoft\\Windows\\Fonts\\OpenSans-Regular.ttf", 16);

	//glEnable(GL_DEPTH_TEST); // enable depth-testing (don't do this, as breask alpha)
	//glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"

	glViewport(0, 0, lh->windowDimensions.width, lh->windowDimensions.height);
	glfwSetKeyCallback(window, key_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetFramebufferSizeCallback(window, size_callback);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glBlendEquation(GL_FUNC_ADD);

	//set up the background
	SetupBackgroundVertices(lh->bgInfo);
	LoadBackgroundImageToTexture(&lh->bgInfo->worldTexture);
	MakeHighresImageTexture(&lh->highres->highresTexture);
	MakeHeatmapTexture(lh->viewNSWE, &lh->bgInfo->heatmapTexture);

	//set up and compile the shaders
	SetupBackgroundShaders(lh->bgInfo);
	SetupPathsShaders(lh->pathInfo);
	SetupPointsShaders(lh->pointsInfo);

	//Set up the FBO that we draw onto
	lh->fboInfo = new FrameBufferObjectInfo();
	SetupFrameBufferObject(lh->fboInfo, lh->windowDimensions.width, lh->windowDimensions.height);

	//initial values
	lh->viewNSWE->target.setvalues(-36.83, -37.11, 174.677 - 0.0, 174.961 - 0.0);
	lh->viewNSWE->target.makeratio((float)lh->windowDimensions.height / (float)lh->windowDimensions.width);
	lh->viewNSWE->setvalues(lh->viewNSWE->target);
	lh->viewNSWE->movetowards(1000000000000.0);

	//make a new region, which is the viewport
	lh->regions.push_back(new Region());
	lh->regionsInfo->displayRegions.resize(1);

	SetupRegionsBufferDataAndVertexAttribArrays(lh->regionsInfo);
	SetupRegionsShaders(lh->regionsInfo);

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
			SetupPathsBufferDataAndVertexAttribArrays(lh->pathInfo);
			SetupPointsBufferDataAndVertexAttribArrays(lh->pointsInfo);
			lh->heatmap->MakeDirty();
			lh->isInitialised = true;
		}

		glClear(GL_COLOR_BUFFER_BIT);

		//Set the FBO as the draw surface
		glBindFramebuffer(GL_FRAMEBUFFER, lh->fboInfo->fbo);

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
				UpdateHeatmapTexture(&lh->viewNSWE->target, lh->bgInfo);
				lh->heatmap->MakeClean();
			}

			if (options->showPaths) {
				if (options->regenPathColours) {
					printf("regen pathplot\n");
					ColourPathPlot(lh);
					glBindBuffer(GL_ARRAY_BUFFER, lh->pathInfo->vbo);
					glBufferSubData(GL_ARRAY_BUFFER, 0, pLocationHistory->pathPlotLocations.size() * sizeof(PathPlotLocation), &pLocationHistory->pathPlotLocations.front());
				}
				DrawPaths(lh->pathInfo);
			}

			if (options->showPoints) {
				DrawPoints(lh->pointsInfo);
			}

			UpdateDisplayRegions(lh->regionsInfo);
			DrawRegions(lh->regionsInfo);	//this draws the selection box, and the rectangle where regions are
		}

		//		glBindFramebuffer(GL_FRAMEBUFFER, 0);	//Get out of the FBO

				//Start the ImGui Frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		//draw the FBO onto the screen
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		DrawFrameBuffer(lh);

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

void SetupBackgroundVertices(BackgroundInfo* backgroundInfo)
{
	//This is just a square that covers the viewpoint.
	static const GLfloat g_vertex_buffer_data[] = {
	-1.0f, -1.0f,
	1.0f, -1.0f,
	-1.0f, 1.0f,
	1.0f, 1.0f
	};

	glGenVertexArrays(1, &backgroundInfo->vao);
	glBindVertexArray(backgroundInfo->vao);

	glGenBuffers(1, &backgroundInfo->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, backgroundInfo->vbo);

	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(0);

	return;
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

void SetupBackgroundShaders(BackgroundInfo* backgroundInfo)
{
	//Create the shader program
	backgroundInfo->shader->LoadShaderFromFile("backgroundVS.glsl", GL_VERTEX_SHADER);
	backgroundInfo->shader->LoadShaderFromFile("backgroundFS.glsl", GL_FRAGMENT_SHADER);
	backgroundInfo->shader->CreateProgram();

	//unsigned int worldTextureLocation, heatmapTextureLocation;
	backgroundInfo->worldTextureLocation = glGetUniformLocation(backgroundInfo->shader->program, "worldTexture");
	backgroundInfo->highresTextureLocation = glGetUniformLocation(backgroundInfo->shader->program, "highresTexture");
	backgroundInfo->heatmapTextureLocation = glGetUniformLocation(backgroundInfo->shader->program, "heatmapTexture");

	// Then bind the uniform samplers to texture units:
	backgroundInfo->shader->UseMe();
	glUniform1i(backgroundInfo->worldTextureLocation, 0);
	glUniform1i(backgroundInfo->highresTextureLocation, 1);
	glUniform1i(backgroundInfo->heatmapTextureLocation, 2);

	return;
}

void SetupFrameBufferObject(FrameBufferObjectInfo* fboInfo, int width, int height)
{
	glGenFramebuffers(1, &fboInfo->fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fboInfo->fbo);
	glGenTextures(1, &fboInfo->fboTexture);
	glBindTexture(GL_TEXTURE_2D, fboInfo->fboTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboInfo->fboTexture, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		printf("Error: Framebuffer not completed\n");
	else printf("Fbo generation finished\n");

	//this sets up the vertices and shaders for the FBO
	SetupBackgroundVertices(&fboInfo->fboBGInfo);
	fboInfo->fboBGInfo.shader->LoadShaderFromFile("fboVS.glsl", GL_VERTEX_SHADER);
	fboInfo->fboBGInfo.shader->LoadShaderFromFile("fboFS.glsl", GL_FRAGMENT_SHADER);
	fboInfo->fboBGInfo.shader->CreateProgram();
	//printf("After create program. glGetError %i\n", glGetError());

	fboInfo->fboBGInfo.shader->UseMe();
	int uniloc = glGetUniformLocation(fboInfo->fboBGInfo.shader->program, "screenTexture");
	//printf("FBO: After getuniloc. glGetError %i\n", glGetError());
	glUniform1i(uniloc, 4);
	//printf("FBO: After uniform set. glGetError %i\n", glGetError());

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	printf("After FBO all done. glGetError %i\n", glGetError());
}

void DrawFrameBuffer(LocationHistory* lh)
{
	lh->fboInfo->fboBGInfo.shader->UseMe();
	lh->fboInfo->fboBGInfo.shader->SetUniformFromFloats("resolution", (float)lh->windowDimensions.width, (float)lh->windowDimensions.height);

	glActiveTexture(GL_TEXTURE0 + 4);
	glBindTexture(GL_TEXTURE_2D, lh->fboInfo->fboTexture);
	glDisable(GL_DEPTH_TEST);
	glBindVertexArray(lh->fboInfo->fboBGInfo.vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	return;
}

void DrawBackgroundAndHeatmap(LocationHistory* lh)
{
	NSWE* viewnswe;
	NSWE* heatmapnswe;
	BackgroundInfo* backgroundInfo;
	HighResManager* highres;
	NSWE* highresnswe;

	backgroundInfo = lh->bgInfo;
	viewnswe = pLocationHistory->viewNSWE;
	heatmapnswe = pLocationHistory->heatmap->nswe;
	highres = lh->highres;

	highres->DecideBestTex(lh->windowDimensions, lh->viewNSWE);
	highresnswe = highres->GetBestNSWE();

	glBindBuffer(GL_ARRAY_BUFFER, backgroundInfo->vbo);

	backgroundInfo->shader->UseMe();
	//backgroundInfo->shader.SetUniformFromFloats("seconds", seconds);
	backgroundInfo->shader->SetUniformFromFloats("resolution", (float)pLocationHistory->windowDimensions.width, (float)pLocationHistory->windowDimensions.height);
	backgroundInfo->shader->SetUniformFromFloats("nswe", viewnswe->north, viewnswe->south, viewnswe->west, viewnswe->east);
	backgroundInfo->shader->SetUniformFromNSWE("highresnswe", highresnswe);
	backgroundInfo->shader->SetUniformFromFloats("highresscale", (float)highres->width / 8192.0f, (float)highres->height / 8192.0f); //as we're just loading the
	backgroundInfo->shader->SetUniformFromNSWE("heatmapnswe", heatmapnswe);

	backgroundInfo->shader->SetUniformFromFloats("maxheatmapvalue", pLocationHistory->heatmap->maxPixel);
	backgroundInfo->shader->SetUniformFromInts("palette", pLocationHistory->globalOptions->palette);

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

void SetupPathsBufferDataAndVertexAttribArrays(MapPathInfo* mapPathInfo)
{
	glGenBuffers(1, &mapPathInfo->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mapPathInfo->vbo);
	glBufferData(GL_ARRAY_BUFFER, pLocationHistory->pathPlotLocations.size() * sizeof(PathPlotLocation), &pLocationHistory->pathPlotLocations.front(), GL_STATIC_DRAW);

	glGenVertexArrays(1, &mapPathInfo->vao);
	glBindVertexArray(mapPathInfo->vao);

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

	//for (auto& element : pLocationHistory->pathPlotLocations) {
//		printf("%f ",element.detaillevel);
	//}

	return;
}

void SetupPathsShaders(MapPathInfo* mapPathInfo)
{
	mapPathInfo->shader->LoadShaderFromFile("mappathsVS.glsl", GL_VERTEX_SHADER);
	mapPathInfo->shader->LoadShaderFromFile("mappathsFS.glsl", GL_FRAGMENT_SHADER);
	mapPathInfo->shader->LoadShaderFromFile("mappathsGS.glsl", GL_GEOMETRY_SHADER);
	mapPathInfo->shader->CreateProgram();
}

void DrawPaths(MapPathInfo* mapPathInfo)
{
	GlobalOptions* options;
	options = pLocationHistory->globalOptions;

	//update uniform shader variables
	mapPathInfo->shader->UseMe();
	mapPathInfo->shader->SetUniformFromNSWE("nswe", pLocationHistory->viewNSWE);
	mapPathInfo->shader->SetUniformFromFloats("seconds", options->seconds * 20.0f);
	mapPathInfo->shader->SetUniformFromFloats("resolution", (float)pLocationHistory->windowDimensions.width, (float)pLocationHistory->windowDimensions.height);
	mapPathInfo->shader->SetUniformFromFloats("linewidth", options->linewidth);
	mapPathInfo->shader->SetUniformFromFloats("cycle", options->cycleSeconds);

	glBindBuffer(GL_ARRAY_BUFFER, mapPathInfo->vbo);
	glBindVertexArray(mapPathInfo->vao);
	glDrawArrays(GL_LINE_STRIP, 0, pLocationHistory->pathPlotLocations.size());

	return;
}

void SetupPointsBufferDataAndVertexAttribArrays(MapPointsInfo* mapPointsInfo) //currently just straight copy from paths, but i should do new array
{
	glGenBuffers(1, &mapPointsInfo->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mapPointsInfo->vbo);
	glBufferData(GL_ARRAY_BUFFER, pLocationHistory->pathPlotLocations.size() * sizeof(PathPlotLocation), &pLocationHistory->pathPlotLocations.front(), GL_STATIC_DRAW);

	glGenVertexArrays(1, &mapPointsInfo->vao);
	glBindVertexArray(mapPointsInfo->vao);

	//lat,long
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(PathPlotLocation), (void*)offsetof(PathPlotLocation, longitude));
	glEnableVertexAttribArray(0);

	//rgba colour
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(PathPlotLocation), (void*)offsetof(PathPlotLocation, rgba));
	glEnableVertexAttribArray(1);

	//timestamp
	glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(PathPlotLocation), (void*)offsetof(PathPlotLocation, timestamp));
	glEnableVertexAttribArray(2);

	return;
}

void SetupPointsShaders(MapPointsInfo* mapPointsInfo)
{
	mapPointsInfo->shader->LoadShaderFromFile("pointsVS.glsl", GL_VERTEX_SHADER);
	mapPointsInfo->shader->LoadShaderFromFile("pointsGS.glsl", GL_GEOMETRY_SHADER);
	mapPointsInfo->shader->LoadShaderFromFile("pointsFS.glsl", GL_FRAGMENT_SHADER);
	mapPointsInfo->shader->CreateProgram();

	mapPointsInfo->shader->LoadUniformLocation(&mapPointsInfo->uniformNswe, "nswe");
	mapPointsInfo->shader->LoadUniformLocation(&mapPointsInfo->uniformResolution, "resolution");
	mapPointsInfo->shader->LoadUniformLocation(&mapPointsInfo->uniformPointRadius, "pointradius");
	mapPointsInfo->shader->LoadUniformLocation(&mapPointsInfo->uniformPointAlpha, "alpha");
	mapPointsInfo->shader->LoadUniformLocation(&mapPointsInfo->uniformSeconds, "seconds");
	mapPointsInfo->shader->LoadUniformLocation(&mapPointsInfo->uniformCycleSeconds, "cycleSeconds");

	//Which times to show
	mapPointsInfo->shader->LoadUniformLocation(&mapPointsInfo->uniformEarliestTimeToShow, "earliesttimetoshow");
	mapPointsInfo->shader->LoadUniformLocation(&mapPointsInfo->uniformLatestTimeToShow, "latesttimetoshow");

	//A highlight is used to give an indication of travel speed, travels through as the peak of a sine wave
	mapPointsInfo->shader->LoadUniformLocation(&mapPointsInfo->uniformShowHighlights, "showhighlights");
	mapPointsInfo->shader->LoadUniformLocation(&mapPointsInfo->uniformSecondsBetweenHighlights, "secondsbetweenhighlights");
	mapPointsInfo->shader->LoadUniformLocation(&mapPointsInfo->uniformTravelTimeBetweenHighlights, "traveltimebetweenhighlights");

	mapPointsInfo->shader->LoadUniformLocation(&mapPointsInfo->uniformPalette, "palette");
	mapPointsInfo->shader->LoadUniformLocation(&mapPointsInfo->uniformColourBy, "colourby");
}

void DrawPoints(MapPointsInfo* mapPointsInfo)
{
	//update uniform shader variables
	mapPointsInfo->shader->UseMe();
	mapPointsInfo->shader->SetUniform(mapPointsInfo->uniformNswe, pLocationHistory->viewNSWE);
	mapPointsInfo->shader->SetUniform(mapPointsInfo->uniformResolution, (float)pLocationHistory->windowDimensions.width, (float)pLocationHistory->windowDimensions.height);
	mapPointsInfo->shader->SetUniform(mapPointsInfo->uniformPointRadius, pLocationHistory->globalOptions->pointdiameter / 2.0f);
	mapPointsInfo->shader->SetUniform(mapPointsInfo->uniformPointAlpha, pLocationHistory->globalOptions->pointalpha);
	mapPointsInfo->shader->SetUniform(mapPointsInfo->uniformSeconds, pLocationHistory->globalOptions->seconds);
	mapPointsInfo->shader->SetUniform(mapPointsInfo->uniformCycleSeconds, pLocationHistory->globalOptions->cycleSeconds);

	mapPointsInfo->shader->SetUniform(mapPointsInfo->uniformEarliestTimeToShow, pLocationHistory->globalOptions->earliestTimeToShow);
	mapPointsInfo->shader->SetUniform(mapPointsInfo->uniformLatestTimeToShow, pLocationHistory->globalOptions->latestTimeToShow);

	mapPointsInfo->shader->SetUniform(mapPointsInfo->uniformShowHighlights, pLocationHistory->globalOptions->showHighlights);
	mapPointsInfo->shader->SetUniform(mapPointsInfo->uniformSecondsBetweenHighlights, pLocationHistory->globalOptions->secondsbetweenhighlights);
	mapPointsInfo->shader->SetUniform(mapPointsInfo->uniformTravelTimeBetweenHighlights, pLocationHistory->globalOptions->minutestravelbetweenhighlights * 60.0f);

	mapPointsInfo->shader->SetUniform(mapPointsInfo->uniformColourBy, pLocationHistory->globalOptions->colourby);

	switch (pLocationHistory->globalOptions->colourby) {
	case 1:
		Palette_Handler::FillShaderPalette(mapPointsInfo->palette, 24, pLocationHistory->globalOptions->indexPaletteHour);
		mapPointsInfo->shader->SetUniform(mapPointsInfo->uniformPalette, 24, &mapPointsInfo->palette[0][0]);
		break;
	case 4:
		Palette_Handler::FillShaderPalette(mapPointsInfo->palette, 24, pLocationHistory->globalOptions->indexPaletteYear);
		mapPointsInfo->shader->SetUniform(mapPointsInfo->uniformPalette, 24, &mapPointsInfo->palette[0][0]);
		break;
	default:
		Palette_Handler::FillShaderPalette(mapPointsInfo->palette, 24, pLocationHistory->globalOptions->indexPaletteWeekday);
		mapPointsInfo->shader->SetUniform(mapPointsInfo->uniformPalette, 24, &mapPointsInfo->palette[0][0]);
		break;
	}

	glBindBuffer(GL_ARRAY_BUFFER, mapPointsInfo->vbo);
	glBindVertexArray(mapPointsInfo->vao);
	glDrawArrays(GL_POINTS, 0, pLocationHistory->pathPlotLocations.size());

	return;
}


void SetupRegionsBufferDataAndVertexAttribArrays(MapRegionsInfo* mapRegionsInfo)
{
	glGenVertexArrays(1, &mapRegionsInfo->vao);
	glBindVertexArray(mapRegionsInfo->vao);

	glGenBuffers(1, &mapRegionsInfo->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mapRegionsInfo->vbo);

	glBufferData(GL_ARRAY_BUFFER, mapRegionsInfo->displayRegions.size() * sizeof(DisplayRegion), &mapRegionsInfo->displayRegions.front(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(DisplayRegion), 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(DisplayRegion), (void*)offsetof(DisplayRegion, east));
	glEnableVertexAttribArray(1);


	glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(DisplayRegion), (void*)offsetof(DisplayRegion, colour));
	glEnableVertexAttribArray(2);

	return;
}

void SetupRegionsShaders(MapRegionsInfo* mapRegionsInfo)
{
	mapRegionsInfo->shader->LoadShaderFromFile("regionsVS.glsl", GL_VERTEX_SHADER);
	mapRegionsInfo->shader->LoadShaderFromFile("regionsFS.glsl", GL_FRAGMENT_SHADER);
	mapRegionsInfo->shader->LoadShaderFromFile("regionsGS.glsl", GL_GEOMETRY_SHADER);
	mapRegionsInfo->shader->CreateProgram();

	return;
}

void UpdateDisplayRegions(MapRegionsInfo* mapRegionsInfo)
{
	int sizeOfRegionVector = pLocationHistory->regions.size();

	//We'll just do a global redraw if any region has changed
	bool needsRedraw = false;
	for (std::size_t i = 0; (i < pLocationHistory->regions.size()) && (!needsRedraw); i++) {
		if (pLocationHistory->regions[i]->needsRedraw) {
			needsRedraw = true;
			pLocationHistory->regions[i]->needsRedraw = false;
		}
	}

	if (!needsRedraw) {
		return;
	}

	if (mapRegionsInfo->displayRegions.size() != sizeOfRegionVector - 1) {
		mapRegionsInfo->displayRegions.resize(sizeOfRegionVector - 1);
		//printf("changing displayregions size to: %i\n",sizeOfRegionVector-1);
	}

	for (int r = 1; r < sizeOfRegionVector; r++) {
		//printf("r:%i of %i.\t", r, sizeOfRegionVector);

		mapRegionsInfo->displayRegions[r - 1].west = pLocationHistory->regions[r]->nswe.west;
		mapRegionsInfo->displayRegions[r - 1].north = pLocationHistory->regions[r]->nswe.north;
		mapRegionsInfo->displayRegions[r - 1].east = pLocationHistory->regions[r]->nswe.east;
		mapRegionsInfo->displayRegions[r - 1].south = pLocationHistory->regions[r]->nswe.south;
		
		mapRegionsInfo->displayRegions[r - 1].colour = pLocationHistory->regions[r]->colour;
		

		//printf("%f %f %f %f\n", mapRegionsInfo->displayRegions[r - 1].f[0], mapRegionsInfo->displayRegions[r - 1].f[1], mapRegionsInfo->displayRegions[r - 1].f[2], mapRegionsInfo->displayRegions[r - 1].f[3]);
		//printf("%i - %i = %i\n", &mapRegionsInfo->displayRegions[1].f, &mapRegionsInfo->displayRegions[0].f, ((long)&mapRegionsInfo->displayRegions[1].f) - ((long)&mapRegionsInfo->displayRegions[0].f));
	}

	glBindBuffer(GL_ARRAY_BUFFER, mapRegionsInfo->vbo);
	glBufferData(GL_ARRAY_BUFFER, mapRegionsInfo->displayRegions.size() * sizeof(DisplayRegion), &mapRegionsInfo->displayRegions.front(), GL_STATIC_DRAW);
	
	return;
}

void DrawRegions(MapRegionsInfo* mapRegionsInfo)
{
	glBindBuffer(GL_ARRAY_BUFFER, mapRegionsInfo->vbo);
	mapRegionsInfo->shader->UseMe();
	mapRegionsInfo->shader->SetUniformFromFloats("resolution", (float)pLocationHistory->windowDimensions.width, (float)pLocationHistory->windowDimensions.height);
	mapRegionsInfo->shader->SetUniformFromNSWE("nswe", pLocationHistory->viewNSWE);
	glBindBuffer(GL_ARRAY_BUFFER, mapRegionsInfo->vbo);

	glBindVertexArray(mapRegionsInfo->vao);
	glDrawArrays(GL_POINTS, 0, mapRegionsInfo->displayRegions.size());//needs to be the number of vertices (not lines)
	glBindVertexArray(0);
	DisplayIfGLError("After DrawRegions.", false);

	return;
}

MapRegionsInfo::MapRegionsInfo()
{
	vao = 0;
	vbo = 0;

	shader = new Shader;

	displayRegions = {};
}

MapRegionsInfo::~MapRegionsInfo()
{
	delete shader;
}

BackgroundInfo::BackgroundInfo()
{
	vao = 0;
	vbo = 0;

	shader = new Shader;

	worldTexture = 0;
	heatmapTexture = 0;

	worldTextureLocation = 0;
	heatmapTextureLocation = 0;
	highresTextureLocation = 0;
}

BackgroundInfo::~BackgroundInfo()
{
	delete shader;
}

MapPathInfo::MapPathInfo()
{
	vao = 0;
	vbo = 0;

	shader = new Shader;
}
MapPathInfo::~MapPathInfo()
{
	delete shader;
}

MapPointsInfo::MapPointsInfo()
{
	vao = 0;
	vbo = 0;

	shader = new Shader;
}
MapPointsInfo::~MapPointsInfo()
{
	delete shader;
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