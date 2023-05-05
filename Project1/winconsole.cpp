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

#include "options.h"
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
#include "mytimezone.h"
#include "guiatlas.h"
#include "filemanager.h"

LocationHistory locationHistory;

MainViewport mainView = { 1200,800 };

FrameBufferObjectInfo fboInfo;	//this is what everything is drawn to first.

//Different things drawn to the map
BackgroundLayer backgroundLayer;
PathLayer pathLayer;
PointsLayer pointsLayer;
RegionsLayer regionsLayer;
HeatmapLayer heatmapLayer;

//GUI stuff
GuiAtlas guiAtlas = { 512,512 };

//Global instance of Options which is accessible to everything including "options.h"
GlobalOptions globalOptions;

int LocationHistory::EmptyLocationInfo()	//closes the file, empties the arrays, resets names/counts
{

	fileLoader.CloseFile();
	

	if (!locations.empty()) {
		locations.clear();
	}
	if (!lodInfo.pathPlotLocations.empty()) {
		lodInfo.pathPlotLocations.clear();
	}

	initialised = false;
	
	return 0;
}

bool LocationHistory::IsInitialised() const
{
	return initialised;
}

void LocationHistory::SetInitialised(bool tf)
{
	initialised = tf;
}

bool LocationHistory::IsLoadingFile() const
{
	return fileLoader.IsLoadingFile();
}

bool LocationHistory::LoadedNotInitialised() const
{
	if ((!IsInitialised()) && (fileLoader.IsFullyLoaded()) && (!fileLoader.IsLoadingFile())) { return true; }
	return false;
}

bool LocationHistory::IsFullyLoaded() const
{
	return fileLoader.IsFullyLoaded();
}

std::string LocationHistory::GetFilename() const
{
	return fileLoader.GetFilename();
}

unsigned long LocationHistory::GetNumberOfLocations() const
{
	return locations.size();
}

unsigned long LocationHistory::GetFileSize() const
{
	return fileLoader.GetFileSize();;
}

unsigned long LocationHistory::GetFileSizeMB() const
{
	return fileLoader.GetFileSize()/0x10'0000;
}

unsigned long LocationHistory::GetTotalBytesRead() const
{
	return fileLoader.GetTotalBytesRead();
}

float LocationHistory::GetSecondsToLoad() const
{
	return fileLoader.GetSecondsToLoad();
}

void LocationHistory::CreateLoadingThread(std::wstring filename)
{
	loaderThread = std::thread(&LocationHistory::OpenAndReadLocationFile, this, filename);
	//loaderThread.detach();
}

void LocationHistory::JoinLoaderThread()
{
	if (loaderThread.joinable()) {
		loaderThread.join();
	}
}

int LocationHistory::OpenAndReadLocationFile(std::wstring filename)
{
	if (!fileLoader.OpenFile(*this,filename))	return 0;


	//sort by GMT timestamp
	std::sort(locations.begin(), locations.end());	//stable_sort might be better, as doesn't muck around if the same size

	//generate local timestamps
	for (auto& loc : locations) {
		loc.correctedTimestamp = MyTimeZone::AsLocalTime(loc.timestamp);
	}

	//copy the array to a more compact one for sending to GPU
	GenerateLocationLODs();

	//Statistics, including count, earliest/latest etc.
	stats.GenerateStatsOnLoad(locations);

	//adjust view, options based on loaded.
	globalOptions.earliestTimeToShow = stats.GetEarliestTimestamp();
	globalOptions.latestTimeToShow = stats.GetLatestTimestamp();

	mainView.viewNSWE.target=FindBestView();
	mainView.viewNSWE.target.makeratio((double)mainView.windowDimensions.height / (double)mainView.windowDimensions.width);

	initialised = false;
	fileLoader.SetFullyLoaded(true);

	printf("loaded, not init\n");
	return 0;
}





int SaveWVFormat(LocationHistory& lh, std::wstring filename)
{
	HANDLE hFile;
	DWORD numberOfLocations;
	DWORD bytesWritten;

	static const char* const magic = "WVF1";

	//copy whole thing at once.
	numberOfLocations = lh.locations.size();
	WVFormat* flatArray = new WVFormat[numberOfLocations];

	for (DWORD i = 0; i < numberOfLocations; i++) {
		flatArray[i].timestamp = lh.locations[i].timestamp;
		flatArray[i].lon = lh.locations[i].longitude;
		flatArray[i].lat = lh.locations[i].latitude;
	}

	hFile = CreateFile(filename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	WriteFile(hFile, magic, 4, &bytesWritten, NULL);
	WriteFile(hFile, &numberOfLocations, sizeof(numberOfLocations), &bytesWritten, NULL);
	WriteFile(hFile, flatArray, numberOfLocations * sizeof(WVFormat), &bytesWritten, NULL);

	delete[] flatArray;
	CloseHandle(hFile);

	return 0;
}



int StartGLProgram()
{
	// start GL context and O/S window using the GLFW helper library
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return 1;
	}


	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(mainView.windowDimensions.width, mainView.windowDimensions.height, "World Tracker", NULL, NULL);
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



	//Setup GL setings
	glViewport(0, 0, mainView.windowDimensions.width, mainView.windowDimensions.height);
	glfwSetKeyCallback(window, key_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetFramebufferSizeCallback(window, size_callback);

	glDisable(GL_DEPTH_TEST); //we're not doing 3d
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glBlendEquation(GL_FUNC_ADD);


	//set up the background
	backgroundLayer.Setup();
	backgroundLayer.SetupTextures();

	//set up and compile the shaders
	backgroundLayer.SetupShaders();
	pathLayer.SetupShaders();
	pointsLayer.SetupShaders();

	DisplayIfGLError("after set up and compile the shaders", false);

	//Set up the FBO that we draw onto
	fboInfo.SetupFrameBufferObject(mainView.windowDimensions.width, mainView.windowDimensions.height);

	//initial values
	mainView.viewNSWE.setTarget({ -36.83f, -37.11f, 174.677f, 174.961f });
	mainView.viewNSWE.target.makeratio((double)mainView.windowDimensions.height / (double)mainView.windowDimensions.width);
	mainView.viewNSWE.setViewAtTarget();
	mainView.viewNSWE.moveTowards(1000000000000.0);

	//make a new region, which is the viewport
	mainView.regions.push_back(new Region());

	regionsLayer.displayRegions.resize(1);
	regionsLayer.SetupVertices();
	regionsLayer.SetupShaders();

	heatmapLayer.Setup(mainView.windowDimensions.width, mainView.windowDimensions.height);


	//GUI setup
	guiAtlas.Initialise();


	//MAIN LOOP
	while (!glfwWindowShouldClose(window)) {
		if (!io.WantCaptureMouse) {	//if Imgui doesn't want the mouse
			Input::ManageMouseMoveClickAndDrag(window, &locationHistory, &mainView);
		}

		globalOptions.seconds = (float)glfwGetTime();

		//get the view moving towards the target
		mainView.viewNSWE.moveTowards(globalOptions.seconds);


		if (locationHistory.LoadedNotInitialised())	{
			printf("Initialising things that need file to be fully loaded\n");
			DisplayIfGLError("before GLRenderLayer::CreateLocationVBO(lh->lodInfo);", false);

			GLRenderLayer::CreateLocationVBO(locationHistory.lodInfo);
			DisplayIfGLError("after GLRenderLayer::CreateLocationVBO(lh->lodInfo);", false);


			pathLayer.SetupVertices();
			pointsLayer.SetupVertices();
			heatmapLayer.SetupVertices();

			DisplayIfGLError("after *SetupVertices", false);
			locationHistory.SetInitialised(true);
			printf("inited");
			locationHistory.JoinLoaderThread();
		}

		glClear(GL_COLOR_BUFFER_BIT);
		
		int currentLod= locationHistory.lodInfo.LodFromDPP(mainView.viewNSWE.width() / mainView.windowDimensions.width);


		//Heatmap rendering to FBO
		if (locationHistory.IsInitialised() && globalOptions.showHeatmap) {
			heatmapLayer.Draw(locationHistory.lodInfo, currentLod, mainView.windowDimensions, mainView.viewNSWE);
		}


		//Draw to offscreen FBO as the main draw surface
		DisplayIfGLError("before fboInfo.BindToDrawTo();", false);
		fboInfo.BindToDrawTo();

		//Background layer has worldmap, heatmap and highres
		backgroundLayer.heatmapTexture = heatmapLayer.texture;
		DisplayIfGLError("before backgroundLayer.Draw", false);
		backgroundLayer.Draw(&mainView);

		//We only draw the points if everything is loaded and initialised.
		if (locationHistory.IsInitialised()) {
			
			if (globalOptions.showPaths) {
				pathLayer.Draw(locationHistory.lodInfo, currentLod, mainView.windowDimensions.width, mainView.windowDimensions.height, &mainView.viewNSWE, globalOptions.linewidth, globalOptions.seconds, globalOptions.cycleSeconds);
			}

			if (globalOptions.showPoints) {
				pointsLayer.Draw(locationHistory.lodInfo, currentLod, mainView.windowDimensions.width, mainView.windowDimensions.height, &mainView.viewNSWE);
			}

			regionsLayer.UpdateFromRegionsData(mainView.regions);
			regionsLayer.Draw(mainView.windowDimensions.width, mainView.windowDimensions.height, &mainView.viewNSWE);


			//locationHistory.CalculateStatistics([](const Location& location) {return mainView.viewNSWE.containsPoint(location.latitude,location.longitude) && location.correctedTimestamp>=globalOptions.earliestTimeToShow && location.correctedTimestamp <= globalOptions.latestTimeToShow; });
		}

		//Start the ImGui Frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();


		DisplayIfGLError("before fboInfo.Draw(lh->windowDimensions.width, lh->windowDimensions.height);", false);

		//draw the FBO onto the screen
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		fboInfo.Draw(mainView.windowDimensions.width, mainView.windowDimensions.height);
		DisplayIfGLError("after fboInfo.Draw(lh->windowDimensions.width, lh->windowDimensions.height);", false);

		if (locationHistory.IsLoadingFile()) {
			Gui::ShowLoadingWindow(locationHistory);
		}
		else
		{
			Gui::MakeGUI(locationHistory, &mainView);	//make the ImGui stuff
		}

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (!mainView.viewNSWE.isMoving())	glfwWaitEventsTimeout(1.0f / 60.0f);	//this runs constantly, but max of ~60fps
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

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Input::HandleKey(window, key, scancode, action, mods, &mainView);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	Input::HandleScroll(window, xoffset, yoffset, &mainView);
}


void size_callback(GLFWwindow* window, int windowNewWidth, int windowNewHeight)
{
	//printf("Resize %i %i\t", windowNewWidth, windowNewHeight);
	mainView.windowDimensions.height = windowNewHeight;
	mainView.windowDimensions.width = windowNewWidth;

	glViewport(0, 0, windowNewWidth, windowNewHeight);

	fboInfo.UpdateSize(mainView.windowDimensions.width, mainView.windowDimensions.height);
	heatmapLayer.UpdateSize(mainView.windowDimensions.width, mainView.windowDimensions.height);

	mainView.viewNSWE.target.makeratio((double)mainView.windowDimensions.height / (double)mainView.windowDimensions.width);

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
	StartGLProgram();
	return 0;
}

void ReloadBackgroundImage()
{
	backgroundLayer.LoadBackgroundImageToTexture(FileManager::GetBackgroundImageFilename().c_str());
}

int GetBackgroundImageTexture()
{
	return backgroundLayer.worldTexture;
}
