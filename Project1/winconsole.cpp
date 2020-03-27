// Project1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define IMGUI_IMPL_OPENGL_LOADER_GLEW


#include <iostream>
#include <tchar.h>
#include <Windows.h>
#include <vector>


#define GLEW_STATIC
#include <glew.h>
#include <GLFW/glfw3.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include "header.h"
#include "nswe.h"
#include "loadjson.h"
#include "shaders.h"
#include "input.h"
#include "heatmap.h"

#define STB_IMAGE_IMPLEMENTATION
#include <STB/stb_image.h>

using namespace std;

//vector<LOCATION> locations;
WORLDCOORD longlatMouse;
NSWE targetNSWE;
movingTarget viewNSWE;
RECTDIMENSION windowDimensions;

//options
float linewidth;
float cycle;
bool showPaths;

LocationHistory lh;



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

	showPaths = true;


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
	const char* glsl_version = "#version 130";
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
	//viewNSWE.target.setvalues(90, -90, -180, 180);
	viewNSWE.target.setvalues(-36.5, -37.5, 174, 175);
	viewNSWE.movetowards(1000000000000);
	linewidth = 2;
	cycle = 3600;

	
	//trying to do a background
	static const GLfloat g_vertex_buffer_data[] = {
   -1.0f, -1.0f, 
   1.0f, -1.0f, 
   -1.0f, 1.0f,
   1.0f, 1.0f
	};
	
	GLuint backgroundVao, backgroundVbo;
	glGenVertexArrays(1, &backgroundVao);
	glBindVertexArray(backgroundVao);

	glGenBuffers(1, &backgroundVbo);
	glBindBuffer(GL_ARRAY_BUFFER, backgroundVbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, backgroundVbo);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0	);
	glEnableVertexAttribArray(0);

	Shader bg;

	bg.LoadShaderFromFile("C:/Users/Tristan/source/repos/Project1/Project1/backgroundVS.glsl", GL_VERTEX_SHADER);
	bg.LoadShaderFromFile("C:/Users/Tristan/source/repos/Project1/Project1/backgroundFS.glsl", GL_FRAGMENT_SHADER);
	bg.CreateProgram();
	
	//Load background texture
	int width, height, nrChannels;
	unsigned char* data = stbi_load("D:/world.200409.3x4096x2048.png", &width, &height, &nrChannels, 0);
	unsigned int worldtexture;
	glGenTextures(1, &worldtexture);
	glBindTexture(GL_TEXTURE_2D, worldtexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	stbi_image_free(data);

	//try to load another
	lh.CreateHeatmap(&viewNSWE, 0);
	unsigned int heatmaptexture;
	glGenTextures(1, &heatmaptexture);
	glBindTexture(GL_TEXTURE_2D, heatmaptexture);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, lh.heatmap->width, lh.heatmap->height, 0, GL_RED, GL_FLOAT, lh.heatmap->pixel);
	

	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);



	//this does the lines of the map
	GLuint vao, vbo;

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, lh.locations.size() * sizeof(LOCATION), &lh.locations.front(), GL_STATIC_DRAW);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(LOCATION), (void*)offsetof(LOCATION, longitude));
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 1, GL_UNSIGNED_INT, GL_FALSE, sizeof(LOCATION), (void*)offsetof(LOCATION, timestamp));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(LOCATION), (void*)offsetof(LOCATION, detaillevel));
	glEnableVertexAttribArray(2);


	Shader mappoints;

	mappoints.LoadShaderFromFile("C:/Users/Tristan/source/repos/Project1/Project1/mappointsVS.glsl", GL_VERTEX_SHADER);
	mappoints.LoadShaderFromFile("C:/Users/Tristan/source/repos/Project1/Project1/mappointsFS.glsl", GL_FRAGMENT_SHADER);
	mappoints.LoadShaderFromFile("C:/Users/Tristan/source/repos/Project1/Project1/mappointsGS.glsl", GL_GEOMETRY_SHADER);
	mappoints.CreateProgram();



	unsigned int worldTextureLocation, heatmapTextureLocation;
	worldTextureLocation = glGetUniformLocation(bg.program, "worldTexture");
	heatmapTextureLocation = glGetUniformLocation(bg.program, "heatmapTexture");

	// Then bind the uniform samplers to texture units:
	bg.UseMe();
	glUniform1i(worldTextureLocation, 0);
	glUniform1i(heatmapTextureLocation, 1);

	float c;
	c = 1;

	while (!glfwWindowShouldClose(window)) {

		float seconds = glfwGetTime();

		viewNSWE.movetowards(seconds);

		// wipe the drawing surface clear
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		glBindBuffer(GL_ARRAY_BUFFER, backgroundVbo);
		
		bg.UseMe();
		bg.SetUniformFromFloats("seconds", seconds);
		bg.SetUniformFromFloats("resolution", windowDimensions.width, windowDimensions.height);
		bg.SetUniformFromFloats("nswe", viewNSWE.north, viewNSWE.south, viewNSWE.west, viewNSWE.east);
		bg.SetUniformFromFloats("heatmapnswe", lh.heatmap->nswe->north, lh.heatmap->nswe->south, lh.heatmap->nswe->west, lh.heatmap->nswe->east);
		
		bg.SetUniformFromFloats("maxheatmapvalue", lh.heatmap->maxPixel);
		bg.SetUniformFromFloats("linewidth", linewidth);
		bg.SetUniformFromFloats("c", c);


		glActiveTexture(GL_TEXTURE0 + 0);
		glBindTexture(GL_TEXTURE_2D, worldtexture);
		glActiveTexture(GL_TEXTURE0 + 1);
		glBindTexture(GL_TEXTURE_2D, heatmaptexture);

		glBindVertexArray(backgroundVao);		
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		
	
		if (showPaths) {
			//update uniform shader variables
			mappoints.UseMe();
			mappoints.SetUniformFromFloats("nswe", viewNSWE.north, viewNSWE.south, viewNSWE.west, viewNSWE.east);
			mappoints.SetUniformFromFloats("seconds", seconds);
			mappoints.SetUniformFromFloats("resolution", windowDimensions.width, windowDimensions.height);
			mappoints.SetUniformFromFloats("linewidth", linewidth);
			mappoints.SetUniformFromFloats("cycle", cycle);

			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBindVertexArray(vao);
			glDrawArrays(GL_LINE_STRIP, 0, lh.locations.size());
		}

		// update other events like input handling 

			// render your GUI
		ImGui::Begin("Map information");
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::Text("Long: %.3f, Lat: %.3f", longlatMouse.longitude, longlatMouse.latitude);
		ImGui::Text("Number of points: %i", lh.locations.size());

		ImGui::Text("N:%.2f, S:%.3f, W:%f, E:%f", viewNSWE.north, viewNSWE.south, viewNSWE.west, viewNSWE.east);
		ImGui::End();
		
		ImGui::Begin("Path drawing");
		

		ImGui::Checkbox("Show paths", &showPaths);

		ImGui::SliderFloat("Line thickness", &linewidth, 0.0f, 20.0f, "%.2f");

		ImGui::SliderFloat("Cycle", &cycle, 1.0f, 3600.0f * 7.0 * 24, "%.0f");
		
		ImGui::End();

		ImGui::Begin("Heatmap");
		
		ImGui::End();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		
		glfwPollEvents();
		glfwSwapBuffers(window);
	}

	//ImGui stuff
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	// close GL context and any other GLFW resources
	glfwTerminate();
	return 0;
}


int main(int argc, char** argv)
{
    std::cout << "Hello World!\n";

	OpenAndReadJSON();
	OptimiseDetail(lh.locations);
	//PrintfSomePoints();

	OpenGLTrial3();


	return 0;
}
