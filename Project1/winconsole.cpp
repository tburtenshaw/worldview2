// Project1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define IMGUI_IMPL_OPENGL_LOADER_GLEW


#include <iostream>
#include <tchar.h>
#include <Windows.h>
#include <vector>
#include <thread>


#define GLEW_STATIC
#include <glew.h>
#include <GLFW/glfw3.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include "shaders.h"
#include "header.h"
#include "nswe.h"
#include "loadjson.h"
//#include "shaders.h"
#include "input.h"
#include "heatmap.h"
#include "regions.h"

#define STB_IMAGE_IMPLEMENTATION
#include <STB/stb_image.h>

using namespace std;

WORLDCOORD longlatMouse;
NSWE targetNSWE;
movingTarget viewNSWE;
RECTDIMENSION windowDimensions;

LocationHistory lh;
Region* testRegion;


BackgroundInfo bgInfo;
MapPathInfo pathInfo;
MapPointsInfo pointsInfo;


GlobalOptions globalOptions;

int OpenAndReadJSON()
{
	HANDLE jsonfile;
	JSON_READER_STATE jrs;


	jsonfile = CreateFile(_T("d:/location history.json"), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

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
		result = ProcessJsonBuffer(buffer, readbytes, &jrs, lh.locations);

		if (result) {
			readbytes = 0;	//trick the loading loop into ending
		}
	}


	delete[] buffer;
	CloseHandle(jsonfile);
	return 0;
}


extern "C" {
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

int OpenGLTrial3()
{

	// start GL context and O/S window using the GLFW helper library
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return 1;
	}

	globalOptions.showPaths = false;


	windowDimensions.width = 900;
	windowDimensions.height = 900;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(windowDimensions.width, windowDimensions.height, "Hello World", NULL, NULL);
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

	glViewport(0, 0, windowDimensions.width, windowDimensions.height);

	glfwSetKeyCallback(window, key_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glBlendEquation(GL_FUNC_ADD);
	int params = -1;

	//Set some app parameters
	viewNSWE.target.setvalues(-36.83, -37.11, 174.677-1, 174.961-1);
	viewNSWE.movetowards(1000000000000);
	globalOptions.linewidth = 2;
	globalOptions.cycle = 3600;
	globalOptions.pointradius = 20;


	//try out regions
	//Region * testRegion =new Region(30,-30,-180,180);

	testRegion = new Region(-37.01, -37.072, 174.88, 174.943);

	testRegion->Populate(&lh);

	
	//set up the background and heatmap
	SetupBackgroundVertices(&bgInfo);
	LoadBackgroundImageToTexture(&bgInfo.worldTexture);
	LoadHeatmapToTexture(&viewNSWE, &bgInfo.heatmapTexture);
	SetupBackgroundShaders(&bgInfo);

	//this does the lines of the map
	SetupPathsBufferDataAndVertexAttribArrays(&pathInfo);
	SetupPathsShaders(&pathInfo);

	//and I'll do one that does points (this will be round points, that can be selectable and maybe deletable
	SetupPointsBufferDataAndVertexAttribArrays(&pointsInfo);
	SetupPointsShaders(&pointsInfo);

	while (!glfwWindowShouldClose(window)) {

		globalOptions.seconds = glfwGetTime();
				
		//get the view moving towards the target
		viewNSWE.movetowards(globalOptions.seconds);

		if (viewNSWE.isDirty()) {
			testRegion->SetNSWE(&viewNSWE.target);
			testRegion->Populate(&lh);
			//printf("d");
		}
		if (!viewNSWE.isMoving() && viewNSWE.isDirty()) {
			NSWE expanded;
			expanded = viewNSWE.target.createExpandedBy(2);
			UpdateHeatmapTexture(&expanded, &bgInfo);
		}


		// wipe the drawing surface clear
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		//This draws the heatmap
		DrawBackgroundAndHeatmap(&bgInfo);
	
		if (globalOptions.showPaths) {
			DrawPaths(&pathInfo);
		}

		if (globalOptions.showPoints) {
			DrawPoints(&pointsInfo);
		}

		// update other events like input handling 


		MakeGUI();	//make the ImGui stuff

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		
		glfwPollEvents();
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

void MakeGUI()
{
	ImGui::Begin("Map information");
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::Text("Long: %.3f, Lat: %.3f", longlatMouse.longitude, longlatMouse.latitude);
	ImGui::Text("Number of points: %i", lh.locations.size());
	ImGui::Text("N:%.2f, S:%.3f, W:%f, E:%f", viewNSWE.north, viewNSWE.south, viewNSWE.west, viewNSWE.east);
	
	{
		float maxhour = 0;
		float fhours[24];
		for (int i = 0; i < 24; i++) {
			fhours[i] = testRegion->hours[i];
			if (fhours[i] > maxhour) {
				maxhour = fhours[i];
			}
		}
		ImGui::PlotHistogram("", fhours, 24, 0, "Time spent each hour", 0, maxhour, ImVec2(0, 80), sizeof(float));
	}

	{
		float maxday = 0;
		float fdays[7];
		for (int i = 0; i < 7; i++) {
			fdays[i] = testRegion->dayofweeks[i];
			if (fdays[i] > maxday) {
				maxday = fdays[i];
			}
		}
		ImGui::PlotHistogram("", fdays, 7, 0, "Time spent each day", 0, maxday, ImVec2(0, 80), sizeof(float));
	}
	

	ImGui::End();

	ImGui::Begin("Path drawing");
	ImGui::Checkbox("Show paths", &globalOptions.showPaths);
	ImGui::SliderFloat("Line thickness", &globalOptions.linewidth, 0.0f, 20.0f, "%.2f");
	ImGui::SliderFloat("Cycle", &globalOptions.cycle, 1.0f, 3600.0f * 7.0 * 24, "%.0f");
	ImGui::End();

	ImGui::Begin("Heatmap");

	ImGui::End();

	ImGui::Begin("Selected");
	ImGui::Checkbox("Show points", &globalOptions.showPoints);
	ImGui::SliderFloat("Point radius", &globalOptions.pointradius,0,100,"%.1f");
	ImGui::End();

	return;
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

void LoadHeatmapToTexture(NSWE *nswe, unsigned int* texture)
{	
	lh.CreateHeatmap(nswe, 0);
	
	glGenTextures(1, texture);
	glBindTexture(GL_TEXTURE_2D, *texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, lh.heatmap->width, lh.heatmap->height, 0, GL_RED, GL_FLOAT, lh.heatmap->pixel);


	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);

	return;
}

void UpdateHeatmapTexture(NSWE* nswe, BackgroundInfo* backgroundInfo)
{
	//printf("Creating heatmap... ");
	lh.CreateHeatmap(nswe, 0);
	//printf("Done.\n");

	glBindTexture(GL_TEXTURE_2D, backgroundInfo->heatmapTexture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, lh.heatmap->width, lh.heatmap->height, GL_RED, GL_FLOAT, lh.heatmap->pixel);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

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
	backgroundInfo->heatmapTextureLocation = glGetUniformLocation(backgroundInfo->shader->program, "heatmapTexture");

	// Then bind the uniform samplers to texture units:
	backgroundInfo->shader->UseMe();
	glUniform1i(backgroundInfo->worldTextureLocation, 0);
	glUniform1i(backgroundInfo->heatmapTextureLocation, 1);
	
	return;
}


void DrawBackgroundAndHeatmap(BackgroundInfo * backgroundInfo)
{
	glBindBuffer(GL_ARRAY_BUFFER, backgroundInfo->vbo);

	backgroundInfo->shader->UseMe();
	//backgroundInfo->shader.SetUniformFromFloats("seconds", seconds);
	backgroundInfo->shader->SetUniformFromFloats("resolution", windowDimensions.width, windowDimensions.height);
	backgroundInfo->shader->SetUniformFromFloats("nswe", viewNSWE.north, viewNSWE.south, viewNSWE.west, viewNSWE.east);
	backgroundInfo->shader->SetUniformFromFloats("heatmapnswe", lh.heatmap->nswe->north, lh.heatmap->nswe->south, lh.heatmap->nswe->west, lh.heatmap->nswe->east);

	backgroundInfo->shader->SetUniformFromFloats("maxheatmapvalue", lh.heatmap->maxPixel);


	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, backgroundInfo->worldTexture);
	glActiveTexture(GL_TEXTURE0 + 1);
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
	glBufferData(GL_ARRAY_BUFFER, lh.locations.size() * sizeof(LOCATION), &lh.locations.front(), GL_STATIC_DRAW);

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
	//update uniform shader variables
	mapPathInfo->shader->UseMe();
	mapPathInfo->shader->SetUniformFromFloats("nswe", viewNSWE.north, viewNSWE.south, viewNSWE.west, viewNSWE.east);
	mapPathInfo->shader->SetUniformFromFloats("seconds", globalOptions.seconds);
	mapPathInfo->shader->SetUniformFromFloats("resolution", windowDimensions.width, windowDimensions.height);
	mapPathInfo->shader->SetUniformFromFloats("linewidth", globalOptions.linewidth);
	mapPathInfo->shader->SetUniformFromFloats("cycle", globalOptions.cycle);

	glBindBuffer(GL_ARRAY_BUFFER, mapPathInfo->vbo);
	glBindVertexArray(mapPathInfo->vao);
	glDrawArrays(GL_LINE_STRIP, 0, lh.locations.size());
	
	return;
}

void SetupPointsBufferDataAndVertexAttribArrays(MapPointsInfo* mapPointsInfo) //currently just straight copy from paths, but i should do new array
{
	glGenBuffers(1, &mapPointsInfo->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mapPointsInfo->vbo);
	glBufferData(GL_ARRAY_BUFFER, lh.locations.size() * sizeof(LOCATION), &lh.locations.front(), GL_STATIC_DRAW);

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
	mapPointsInfo->shader->SetUniformFromFloats("nswe", viewNSWE.north, viewNSWE.south, viewNSWE.west, viewNSWE.east);
	mapPointsInfo->shader->SetUniformFromFloats("resolution", windowDimensions.width, windowDimensions.height);
	mapPointsInfo->shader->SetUniformFromFloats("pointradius", globalOptions.pointradius);


	glBindBuffer(GL_ARRAY_BUFFER, mapPointsInfo->vbo);
	glBindVertexArray(mapPointsInfo->vao);
	glDrawArrays(GL_POINTS, 0, lh.locations.size());

	return;
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




int main(int argc, char** argv)
{
    std::cout << "Hello World!\n";

	OpenAndReadJSON();
	OptimiseDetail(lh.locations);

	OpenGLTrial3();


	return 0;
}

