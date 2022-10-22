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
#include "mytimezone.h"

LocationHistory* pLocationHistory;

FrameBufferObjectInfo fboInfo;	//this is what everything is drawn to first.

//Different things I might draw to the map
BackgroundLayer backgroundLayer;
PathLayer pathLayer;
PointsLayer pointsLayer;
RegionsLayer regionsLayer;
HeatmapLayer heatmapLayer;

//Options
GlobalOptions globalOptions;

int LocationHistory::CloseLocationFile()	//closes the file, empties the arrays, resets names/counts
{
	if (isLoadingFile) {
		return 2;
	}

	isFileChosen = false;
	isFullyLoaded = true;	//we're loaded with nothing
	isInitialised = false;
	isLoadingFile = false;

	if (!locations.empty()) {
		locations.clear();
	}
	if (!lodInfo.pathPlotLocations.empty()) {
		lodInfo.pathPlotLocations.clear();
	}

	//lh->filename = L"";	//can't do this, as it's where the file to load is stored.
	filesize = 0;
	
	return 0;
}

int LocationHistory::OpenAndReadLocationFile()
{
	HANDLE hLocationFile;

	if (isLoadingFile) {	//we don't want to load if we're already loading something
		return 2;
	}

	hLocationFile = CreateFile(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if (hLocationFile == INVALID_HANDLE_VALUE) {
		isFileChosen = false;
		isLoadingFile = false;
		isFullyLoaded = false;
		return 1;
	}

	CloseLocationFile();	//close any existing file
	isLoadingFile = true;
	isFileChosen = true;
	isInitialised = false;

	LARGE_INTEGER LIfilesize;
	GetFileSizeEx(hLocationFile, &LIfilesize);
	filesize = LIfilesize.QuadPart;
	printf("File size: %i\n", filesize);

	std::string extension = std::filesystem::path(filename).extension().string();
	if (extension == ".json") {
		LoadJsonFile(this, hLocationFile);
	}
	else if (extension == ".wvf") {
		LoadWVFormat(this, hLocationFile);
	}
	CloseHandle(hLocationFile);

	//sort by GMT timestamp
	std::sort(locations.begin(), locations.end());	//stable_sort might be better, as doesn't muck around if the same size

	//generate local timestamps
	for (auto& loc : locations) {
		loc.correctedTimestamp = MyTimeZone::FixToLocalTime(loc.timestamp);
	}

	//copy the array to a more compact one for sending to GPU
	GenerateLocationLODs();

	//Statistics, including count, earliest/latest etc.
	GenerateStatsOnLoad();



	//adjust view, options based on loaded.
	globalOptions.earliestTimeToShow = stats.earliestTimestamp;
	globalOptions.latestTimeToShow = stats.latestTimestamp;

	viewNSWE.target=FindBestView();
	viewNSWE.target.makeratio((float)windowDimensions.height / (float)windowDimensions.width);


	//Plan: to have five levels of detail (5 LoDs) from [0] to [4]. These are used just for plotting.
	//0 is detail <10 degrees per million pixels, then 1 is <100 DPMP etc.
	//All get reset if 24 hours passes.

	isLoadingFile = false;
	isFullyLoaded = true;
	isInitialised = false;
	isFileChosen = false;
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



	//Setup GL setings
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
	backgroundLayer.Setup();
	backgroundLayer.SetupTextures();

	//set up and compile the shaders
	backgroundLayer.SetupShaders();
	pathLayer.SetupShaders();
	pointsLayer.SetupShaders();

	DisplayIfGLError("after set up and compile the shaders", false);

	//Set up the FBO that we draw onto
	fboInfo.SetupFrameBufferObject(lh->windowDimensions.width, lh->windowDimensions.height);

	//initial values
	lh->viewNSWE.target.setvalues(-36.83f, -37.11f, 174.677f - 0.0f, 174.961f - 0.0f);
	lh->viewNSWE.target.makeratio((float)lh->windowDimensions.height / (float)lh->windowDimensions.width);
	lh->viewNSWE.setvalues(lh->viewNSWE.target);
	lh->viewNSWE.movetowards(1000000000000.0);

	//make a new region, which is the viewport
	lh->regions.push_back(new Region());

	regionsLayer.displayRegions.resize(1);
	regionsLayer.SetupVertices();
	regionsLayer.SetupShaders();

	heatmapLayer.Setup(lh->windowDimensions.width, lh->windowDimensions.height);



	//MAIN LOOP
	while (!glfwWindowShouldClose(window)) {
		if (!io.WantCaptureMouse) {	//if Imgui doesn't want the mouse
			ManageMouseMoveClickAndDrag(window, lh);
		}

		globalOptions.seconds = (float)glfwGetTime();

		//get the view moving towards the target
		lh->viewNSWE.movetowards(globalOptions.seconds);

		if (pLocationHistory->isFileChosen && !pLocationHistory->isLoadingFile) {
			std::thread loadingthread(&LocationHistory::OpenAndReadLocationFile, pLocationHistory);
			loadingthread.detach();
		}

		if ((lh->isInitialised == false) && (lh->isFullyLoaded) && (lh->isLoadingFile == false)) {
			printf("Initialising things that need file to be fully loaded\n");
			DisplayIfGLError("before GLRenderLayer::CreateLocationVBO(lh->lodInfo);", false);

			GLRenderLayer::CreateLocationVBO(lh->lodInfo);
			DisplayIfGLError("after GLRenderLayer::CreateLocationVBO(lh->lodInfo);", false);


			pathLayer.SetupVertices();
			pointsLayer.SetupVertices();
			heatmapLayer.SetupVertices();

			DisplayIfGLError("after *SetupVertices", false);

			lh->isInitialised = true;
		}

		
		
		
		glClear(GL_COLOR_BUFFER_BIT);
		
		int currentLod= lh->lodInfo.LodFromDPP(lh->viewNSWE.width() / lh->windowDimensions.width);


		//Heatmap rendering to FBO
		if (lh->isInitialised && lh->isFullyLoaded) {
			heatmapLayer.Draw(lh->lodInfo, currentLod, lh->windowDimensions.width, lh->windowDimensions.height, &lh->viewNSWE, &globalOptions);
		}


		//Set an offscreen FBO as the main draw surface
		DisplayIfGLError("before fboInfo.BindToDrawTo();", false);
		fboInfo.BindToDrawTo();

		//Background layer has worldmap, heatmap and highres
		backgroundLayer.heatmapTexture = heatmapLayer.texture;
		DisplayIfGLError("before backgroundLayer.Draw", false);
		backgroundLayer.Draw(lh->windowDimensions, lh->viewNSWE, globalOptions);

		//We only draw the points if everything is loaded and initialised.
		if (lh->isInitialised && lh->isFullyLoaded) {
			if (lh->viewNSWE.isDirty()) {
				lh->regions[0]->SetNSWE(&lh->viewNSWE.target);
				lh->regions[0]->Populate(lh);
			}

			if (globalOptions.showPaths) {
				pathLayer.Draw(lh->lodInfo, currentLod, lh->windowDimensions.width, lh->windowDimensions.height, &lh->viewNSWE, globalOptions.linewidth, globalOptions.seconds, globalOptions.cycleSeconds);
			}

			if (globalOptions.showPoints) {
				pointsLayer.Draw(lh->lodInfo, currentLod, lh->windowDimensions.width, lh->windowDimensions.height, &lh->viewNSWE, &globalOptions);
			}

			regionsLayer.UpdateFromRegionsData(lh->regions);
			regionsLayer.Draw(lh->windowDimensions.width, lh->windowDimensions.height, &lh->viewNSWE);

		}

		//Start the ImGui Frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();


		DisplayIfGLError("before fboInfo.Draw(lh->windowDimensions.width, lh->windowDimensions.height);", false);
		//draw the FBO onto the screen
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		fboInfo.Draw(lh->windowDimensions.width, lh->windowDimensions.height);
		DisplayIfGLError("after fboInfo.Draw(lh->windowDimensions.width, lh->windowDimensions.height);", false);

		if (lh->isLoadingFile == false) {
			Gui::MakeGUI(lh, &globalOptions);	//make the ImGui stuff
		}
		else if (lh->isLoadingFile == true) {
			Gui::ShowLoadingWindow(lh);
		}

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (!lh->viewNSWE.isMoving())	glfwWaitEventsTimeout(1.0f / 60.0f);	//this runs constantly, but max of ~60fps
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


void size_callback(GLFWwindow* window, int windowNewWidth, int windowNewHeight)
{
	//printf("Resize %i %i\t", windowNewWidth, windowNewHeight);
	pLocationHistory->windowDimensions.height = windowNewHeight;
	pLocationHistory->windowDimensions.width = windowNewWidth;

	glViewport(0, 0, windowNewWidth, windowNewHeight);

	fboInfo.UpdateSize(pLocationHistory->windowDimensions.width, pLocationHistory->windowDimensions.height);
	heatmapLayer.UpdateSize(pLocationHistory->windowDimensions.width, pLocationHistory->windowDimensions.height);

	pLocationHistory->viewNSWE.target.makeratio((float)pLocationHistory->windowDimensions.height / (float)pLocationHistory->windowDimensions.width);

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
		std::thread loadingthread(&LocationHistory::OpenAndReadLocationFile, &locationHistory);
		loadingthread.detach();
	}

	StartGLProgram(pLocationHistory);

	return 0;
}