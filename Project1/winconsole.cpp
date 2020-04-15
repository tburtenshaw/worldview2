// Project1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define IMGUI_IMPL_OPENGL_LOADER_GLEW
//#define IMGUI_IMPL_OPENGL_LOADER_GLAD

#include <iostream>
#include <tchar.h>
#include <Windows.h>
#include <vector>
#include <thread>


#define GLEW_STATIC
#include <glew.h>
//#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include "shaders.h"
#include "header.h"
#include "nswe.h"
#include "loadjson.h"
#include "input.h"
#include "heatmap.h"
#include "regions.h"
#include "gui.h"
#include "highresmanager.h"


#define STB_IMAGE_IMPLEMENTATION
#include <STB/stb_image.h>

using namespace std;

LocationHistory* pLocationHistory;

std::thread* pthread;

int OpenAndReadJSON(LocationHistory * lh)
{
	HANDLE jsonfile;
	JSON_READER_STATE jrs;
	
	if (lh->isLoadingFile) {	//we don't want to load if we're already loading something
		return 2;
	}

	lh->isLoadingFile = true;
	lh->isFileChosen = true;

	//jsonfile = CreateFile(_T("d:/lizzie.json"), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	jsonfile = CreateFile(lh->filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	
	if (jsonfile == INVALID_HANDLE_VALUE) {
		Beep(750, 300);
		lh->isFileChosen = false;
		lh->isLoadingFile = false;
		return 1;
	}

	if (!lh->locations.empty()) {
		lh->locations.clear();
	}

	LARGE_INTEGER filesize;
	GetFileSizeEx(jsonfile, &filesize);
	lh->filesize = filesize.QuadPart;
	printf("filesize: %i\n", lh->filesize);


	memset(&jrs, 0, sizeof(jrs));
	char* buffer;

	buffer = new char[READ_BUFFER_SIZE];
	unsigned long readbytes;
	BOOL rf;
	int result;	//zero if no problems.

	readbytes = 1;
	while (readbytes) {
		rf = ReadFile(jsonfile, buffer, READ_BUFFER_SIZE - 1, &readbytes, NULL);
		if (rf == false) {
			printf("failed reading the file");
			return 1;
		}
		result = ProcessJsonBuffer(buffer, readbytes, &jrs, lh->locations, lh);
		lh->totalbytesread += readbytes;
		if (result) {
			readbytes = 0;	//trick the loading loop into ending
		}
	}


	delete[] buffer;
	CloseHandle(jsonfile);

	OptimiseDetail(lh->locations);

	lh->isLoadingFile=false;
	lh->isFullyLoaded = true;
	lh->isInitialised = false;

	return 0;
}

int StartGLProgram(LocationHistory * lh)
{
	GlobalOptions *options;
	options = lh->globalOptions;

	HighResManager *highres = new HighResManager;

	// start GL context and O/S window using the GLFW helper library
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return 1;
	}

	lh->windowDimensions->width = 900;
	lh->windowDimensions->height = 900;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_SAMPLES, 4);
	//glEnable(GL_MULTISAMPLE);

	GLFWwindow* window = glfwCreateWindow(lh->windowDimensions->width, lh->windowDimensions->height, "Hello World", NULL, NULL);
	if (!window) {
		fprintf(stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return 1;
	}
	glfwMakeContextCurrent(window);

	// start GLEW extension handler
	glewExperimental = GL_TRUE;
	glewInit();


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
	
	//glEnable(GL_DEPTH_TEST); // enable depth-testing (don't do this, as breask alpha)
	//glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"

	glViewport(0, 0, lh->windowDimensions->width, lh->windowDimensions->height);
	glfwSetKeyCallback(window, key_callback);
	glfwSetScrollCallback(window, scroll_callback);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glBlendEquation(GL_FUNC_ADD);

	//set up the background
	SetupBackgroundVertices(lh->bgInfo);
	LoadBackgroundImageToTexture(&lh->bgInfo->worldTexture);
	MakeHighresImageTexture(&lh->highres->highresTexture);
	MakeHeatmapTexture(lh->viewNSWE, &lh->bgInfo->heatmapTexture);
	SetupBackgroundShaders(lh->bgInfo);


	lh->viewNSWE->target.setvalues(-36.83, -37.11, 174.677 - 0.0, 174.961 - 0.0);
	lh->viewNSWE->movetowards(1000000000000);

	while (!glfwWindowShouldClose(window)) {

		if (!io.WantCaptureMouse) {
			ManageMouseMoveClickAndDrag(window, lh);
		}

		options->seconds = (float)glfwGetTime();

		//get the view moving towards the target
		lh->viewNSWE->movetowards(lh->globalOptions->seconds);


		if ((lh->isInitialised == false) && (lh->isFullyLoaded) && (lh->isLoadingFile == false)) {
			printf("Initialising things that need file to be fully loaded\n");
			pthread->join();

			lh->regions.push_back(new Region());

			//this does the lines of the map
			SetupPathsBufferDataAndVertexAttribArrays(lh->pathInfo);
			SetupPathsShaders(lh->pathInfo);

			//and I'll do one that does points (this will be round points, that can be selectable and maybe deletable
			SetupPointsBufferDataAndVertexAttribArrays(lh->pointsInfo);
			SetupPointsShaders(lh->pointsInfo);
			lh->isInitialised = true;

		}

		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		DrawBackgroundAndHeatmap(lh);

		if (lh->isInitialised && lh->isFullyLoaded) {

			if (lh->viewNSWE->isDirty()) {
				lh->regions[0]->SetNSWE(&lh->viewNSWE->target);
				lh->regions[0]->Populate(lh);
				lh->heatmap->MakeDirty();
			}

			if (!lh->viewNSWE->isMoving() && lh->heatmap->IsDirty() && lh->globalOptions->showHeatmap) {
				NSWE expanded;
				expanded = lh->viewNSWE->target.createExpandedBy(1);
				UpdateHeatmapTexture(&expanded, lh->bgInfo);
				lh->heatmap->MakeClean();
			}

			//DrawBackgroundAndHeatmap(lh);

			if (options->showPaths) {
				DrawPaths(lh->pathInfo);
			}

			if (options->showPoints) {
				DrawPoints(lh->pointsInfo);
			}

			Gui::MakeGUI(lh);	//make the ImGui stuff
		}

		

		if (lh->isLoadingFile == true) {
			Gui::ShowLoadingWindow(lh);
		}
		

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		
		//glfwPollEvents();
		glfwWaitEventsTimeout(0.016);
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
	glBindBuffer(GL_ARRAY_BUFFER, backgroundInfo->vao);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, backgroundInfo->vbo);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(0);

	return;
}


void LoadBackgroundImageToTexture(unsigned int* texture)
{
	int width, height, nrChannels;
	unsigned char* data = stbi_load("D:/world.200409.3x4096x2048.png", &width, &height, &nrChannels, 0);

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


void MakeHeatmapTexture(NSWE *nswe, unsigned int* texture)
{	
	glGenTextures(1, texture);
	glBindTexture(GL_TEXTURE_2D, *texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, pLocationHistory->heatmap->width, pLocationHistory->heatmap->height, 0, GL_RED, GL_FLOAT, NULL);

	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);	//this is the poles
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return;
}

void UpdateHeatmapTexture(NSWE* nswe, BackgroundInfo* backgroundInfo)
{
	
	
	pLocationHistory->heatmap->CreateHeatmap(nswe, 0);

	glBindTexture(GL_TEXTURE_2D, backgroundInfo->heatmapTexture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, pLocationHistory->heatmap->width, pLocationHistory->heatmap->height, GL_RED, GL_FLOAT, pLocationHistory->heatmap->pixel);
	glGenerateMipmap(GL_TEXTURE_2D);

	return;
}

void SetupBackgroundShaders(BackgroundInfo* backgroundInfo)
{
	//Create the shader program
	backgroundInfo->shader->LoadShaderFromFile("C:/Users/Tristan/source/repos/Project1/Project1/backgroundVS.glsl", GL_VERTEX_SHADER);
	backgroundInfo->shader->LoadShaderFromFile("C:/Users/Tristan/source/repos/Project1/Project1/backgroundFS.glsl", GL_FRAGMENT_SHADER);
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
	
	printf("After background shaders. glGetError %i\n", glGetError());

	return;
}


void DrawBackgroundAndHeatmap(LocationHistory * lh)
{
	NSWE* viewnswe;
	NSWE *heatmapnswe;
	BackgroundInfo* backgroundInfo;
	HighResManager* highres;
	NSWE* highresnswe;

	backgroundInfo = lh->bgInfo;
	viewnswe = pLocationHistory->viewNSWE;
	heatmapnswe = pLocationHistory->heatmap->nswe;
	highres = lh->highres;

	highres->DecideBestTex(*lh->windowDimensions, lh->viewNSWE);
	highresnswe = highres->GetBestNSWE();


	glBindBuffer(GL_ARRAY_BUFFER, backgroundInfo->vbo);

	backgroundInfo->shader->UseMe();
	//backgroundInfo->shader.SetUniformFromFloats("seconds", seconds);
	backgroundInfo->shader->SetUniformFromFloats("resolution", (float)pLocationHistory->windowDimensions->width, (float)pLocationHistory->windowDimensions->height);
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
	glBufferData(GL_ARRAY_BUFFER, pLocationHistory->locations.size() * sizeof(LOCATION), &pLocationHistory->locations.front(), GL_STATIC_DRAW);

	glGenVertexArrays(1, &mapPathInfo->vao);
	glBindVertexArray(mapPathInfo->vao);

	//lat,long
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(LOCATION), (void*)offsetof(LOCATION, longitude));
	glEnableVertexAttribArray(0);

	//timestamp (maybe replace the whole array with a smaller copy, and let this be a colour)
	glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, sizeof(LOCATION), (void*)offsetof(LOCATION, timestamp));
	glEnableVertexAttribArray(1);

	//detail level
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(LOCATION), (void*)offsetof(LOCATION, detaillevel));
	glEnableVertexAttribArray(2);
	
	return;
}

void SetupPathsShaders(MapPathInfo* mapPathInfo)
{
	mapPathInfo->shader->LoadShaderFromFile("C:/Users/Tristan/source/repos/Project1/Project1/mappathsVS.glsl", GL_VERTEX_SHADER);
	mapPathInfo->shader->LoadShaderFromFile("C:/Users/Tristan/source/repos/Project1/Project1/mappathsFS.glsl", GL_FRAGMENT_SHADER);
	mapPathInfo->shader->LoadShaderFromFile("C:/Users/Tristan/source/repos/Project1/Project1/mappathsGS.glsl", GL_GEOMETRY_SHADER);
	mapPathInfo->shader->CreateProgram();
}

void DrawPaths(MapPathInfo* mapPathInfo)
{
	GlobalOptions* options;
	options = pLocationHistory->globalOptions;
	
	//update uniform shader variables
	mapPathInfo->shader->UseMe();
	mapPathInfo->shader->SetUniformFromNSWE("nswe", pLocationHistory->viewNSWE);
	mapPathInfo->shader->SetUniformFromFloats("seconds", options->seconds*20.0f);
	mapPathInfo->shader->SetUniformFromFloats("resolution", (float)pLocationHistory->windowDimensions->width, (float)pLocationHistory->windowDimensions->height);
	mapPathInfo->shader->SetUniformFromFloats("linewidth", options->linewidth);
	mapPathInfo->shader->SetUniformFromFloats("cycle", options->cycle);

	glBindBuffer(GL_ARRAY_BUFFER, mapPathInfo->vbo);
	glBindVertexArray(mapPathInfo->vao);
	glDrawArrays(GL_LINE_STRIP, 0, pLocationHistory->locations.size());
	
	return;
}

void SetupPointsBufferDataAndVertexAttribArrays(MapPointsInfo* mapPointsInfo) //currently just straight copy from paths, but i should do new array
{
	glGenBuffers(1, &mapPointsInfo->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mapPointsInfo->vbo);
	glBufferData(GL_ARRAY_BUFFER, pLocationHistory->locations.size() * sizeof(LOCATION), &pLocationHistory->locations.front(), GL_STATIC_DRAW);

	glGenVertexArrays(1, &mapPointsInfo->vao);
	glBindVertexArray(mapPointsInfo->vao);

	//lat,long
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(LOCATION), (void*)offsetof(LOCATION, longitude));
	glEnableVertexAttribArray(0);

	//timestamp (maybe replace the whole array with a smaller copy, and let this be a colour)
	glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, sizeof(LOCATION), (void*)offsetof(LOCATION, timestamp));
	glEnableVertexAttribArray(1);

	//detail level
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(LOCATION), (void*)offsetof(LOCATION, detaillevel));
	glEnableVertexAttribArray(2);

	return;
}

void SetupPointsShaders(MapPointsInfo* mapPointsInfo)
{
	mapPointsInfo->shader->LoadShaderFromFile("C:/Users/Tristan/source/repos/Project1/Project1/pointsVS.glsl", GL_VERTEX_SHADER);
	mapPointsInfo->shader->LoadShaderFromFile("C:/Users/Tristan/source/repos/Project1/Project1/pointsGS.glsl", GL_GEOMETRY_SHADER);
	mapPointsInfo->shader->LoadShaderFromFile("C:/Users/Tristan/source/repos/Project1/Project1/pointsFS.glsl", GL_FRAGMENT_SHADER);
	mapPointsInfo->shader->CreateProgram();
}

void DrawPoints(MapPointsInfo* mapPointsInfo)
{
	//update uniform shader variables
	mapPointsInfo->shader->UseMe();
	mapPointsInfo->shader->SetUniformFromFloats("nswe", pLocationHistory->viewNSWE->north, pLocationHistory->viewNSWE->south, pLocationHistory->viewNSWE->west, pLocationHistory->viewNSWE->east);
	mapPointsInfo->shader->SetUniformFromFloats("resolution", pLocationHistory->windowDimensions->width, pLocationHistory->windowDimensions->height);
	mapPointsInfo->shader->SetUniformFromFloats("pointradius", pLocationHistory->globalOptions->pointradius);


	glBindBuffer(GL_ARRAY_BUFFER, mapPointsInfo->vbo);
	glBindVertexArray(mapPointsInfo->vao);
	glDrawArrays(GL_POINTS, 0, pLocationHistory->locations.size());

	return;
}





int main(int argc, char** argv)
{
    std::cout << "Hello World!\n";
	pLocationHistory = new LocationHistory;

	std::thread loadingthread(OpenAndReadJSON,pLocationHistory);
	
	pthread = &loadingthread;

	StartGLProgram(pLocationHistory);


	return 0;
}

