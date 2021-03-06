#include <imgui.h>
#include "gui.h"
#include "header.h"
#include "nswe.h"
#include "regions.h"
#include "mytimezone.h"
#include "input.h"
#include "heatmap.h"
#include <string>
#include <vector>

#include <Windows.h>

void Gui::ShowLoadingWindow(LocationHistory* lh)
{
	ImGui::SetNextWindowSize(ImVec2(500.0f, 140.0f));
	ImGui::SetNextWindowPos(ImVec2(200.0f, 300.0f));
	ImGui::Begin("Loading", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
	float p = (float)lh->totalbytesread / (float)lh->filesize;
	if (lh->totalbytesread < lh->filesize) {
		ImGui::Text("Processed %.1f MB (of %.1f MB)", (float)lh->totalbytesread / 0x100000, (float)lh->filesize / 0x100000);
	}
	else {
		ImGui::Text("Initialising...");
	}
	ImGui::ProgressBar(p);
	ImGui::End();
}

void Gui::MakeGUI(LocationHistory * lh)
{
	GlobalOptions * options;
	options = lh->globalOptions;

	std::string sigfigs;	//this holds the string template (e.g. %.4f) that is best to display a unit at the current zoom
	std::string sCoords;
	sigfigs = Gui::BestSigFigsFormat(lh->viewNSWE, lh->windowDimensions);
	
	ImGui::Begin("Map information");
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	

	char displayfilename[260];
	size_t i;
	wcstombs_s(&i, displayfilename,260, lh->filename.c_str(), 260);

	ImGui::Text("File name: %s",displayfilename);
	ImGui::Text("File size: %i", lh->filesize);

	sCoords = "Cursor: Long: " + sigfigs + ", Lat: " + sigfigs;
	ImGui::Text(sCoords.c_str(), lh->mouseInfo->longlatMouse.longitude, lh->mouseInfo->longlatMouse.latitude);
	ImGui::Text("Number of points: %i", lh->locations.size());

	sCoords = "N:" + sigfigs + ", S:" + sigfigs + ", W:" + sigfigs + ", E:" + sigfigs;
	ImGui::Text(sCoords.c_str(), lh->viewNSWE->north, lh->viewNSWE->south, lh->viewNSWE->west, lh->viewNSWE->east);

	ImGui::Text("Earliest: %s", MyTimeZone::FormatUnixTime(MyTimeZone::FixToLocalTime(lh->earliesttimestamp), MyTimeZone::FormatFlags::SHOW_TIME| MyTimeZone::FormatFlags::DMY).c_str());
	ImGui::Text("Latest: %s", MyTimeZone::FormatUnixTime(MyTimeZone::FixToLocalTime(lh->latesttimestamp), MyTimeZone::FormatFlags::SHOW_TIME | MyTimeZone::FormatFlags::TEXT_MONTH).c_str());


	Gui::ShowRegionInfo(lh->regions[0]);


	ImGui::End();

	for (std::size_t i = 1; i < lh->regions.size(); i++) {
		
		if (lh->regions[i]->toDelete) {
			lh->regions.erase(lh->regions.begin() + i);
		}
		if (lh->regions[i]->shouldShowWindow) {
			ImGui::Begin(lh->regions[i]->displayname.c_str());
			Gui::ShowRegionInfo(lh->regions[i]);
			ImGui::End();
		}
	}


	ImGui::ShowDemoWindow();
	//ImGui::ShowStyleEditor();


	ImGui::Begin("Path drawing");
	ImGui::Checkbox("Show paths", &options->showPaths);
	ImGui::SliderFloat("Line thickness", &options->linewidth, 1.0f, 8.0f, "%.1f");

	const char* cyclenames[] = { "One minute", "One hour", "One day", "One week", "One month", "One year", "Five years", "Other" };
	const float cycleresults[] = { 60.0f,3600.0f,3600.0f * 24.0f,3600.0f * 24.0f * 7.0f,3600.0f * 24.0f * 365.25f / 12.0f,3600.0f * 24.0f * 365.25f ,3600.0f * 24.0f * 365.25f * 5.0f,0.0f};
	
	static int uiCycleSelect = 0;
	ImGui::Combo("Cycle over", &uiCycleSelect, cyclenames, IM_ARRAYSIZE(cyclenames));
	if (uiCycleSelect < IM_ARRAYSIZE(cyclenames)-1) {
		options->cycle = cycleresults[uiCycleSelect];
	}
	ImGui::SliderFloat("Cycle", &options->cycle, 60.0f, 3600.0f * 24.0f * 365.0f * 5.0f, "%.0f", 6.0f);
	for (int i = 0; i < IM_ARRAYSIZE(cyclenames) - 1; i++) {
		if (options->cycle == cycleresults[i]) {
			uiCycleSelect = i;
		}
	}



	static ImVec4 color[7] = { ImVec4(-1.0f,0.0f,0.0f,0.0f),ImVec4(-1.0f,0.0f,0.0f,0.0f),ImVec4(-1.0f,0.0f,0.0f,0.0f),ImVec4(-1.0f,0.0f,0.0f,0.0f),ImVec4(-1.0f,0.0f,0.0f,0.0f),ImVec4(-1.0f,0.0f,0.0f,0.0f),ImVec4(-1.0f,0.0f,0.0f,0.0f) };	//set negative if not loaded
	for (int i = 0; i < 7; i++) {
		if (color[i].x < 0.0f) {
			printf("neg %i ", i);
			color[i].x = (float)options->paletteDayOfWeek[i].r / 255.0f;
			color[i].y = (float)options->paletteDayOfWeek[i].g / 255.0f;
			color[i].z = (float)options->paletteDayOfWeek[i].b / 255.0f;
			color[i].w = (float)options->paletteDayOfWeek[i].a / 255.0f;
		}

		if (ImGui::ColorEdit4(MyTimeZone::daynames[i].c_str(), (float*)&color[i], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaPreviewHalf | ImGuiColorEditFlags_AlphaBar)) {
			options->paletteDayOfWeek[i].r = (unsigned char)(color[i].x * 255.0f);
			options->paletteDayOfWeek[i].g = (unsigned char)(color[i].y * 255.0f);
			options->paletteDayOfWeek[i].b = (unsigned char)(color[i].z * 255.0f);
			options->paletteDayOfWeek[i].a = (unsigned char)(color[i].w * 255.0f);
			options->regenPathColours = true;
		}
		ImGui::SameLine();
	}

	ImGui::End();

	static float oldBlur=0;
	static float oldMinimumaccuracy = 0;
	static float oldBlurperaccurary = 0;
	ImGui::Begin("Heatmap");
	ImGui::Checkbox("Show heatmap", &options->showHeatmap);
	ImGui::SliderFloat("Gaussian blur", &options->gaussianblur,0.0f,10.0f, "%.1f");	
	ImGui::Checkbox("_Predict paths", &options->predictpath);
	ImGui::Checkbox("_Blur by accurracy", &options->blurperaccuracy);
	ImGui::SliderInt("Minimum accuracy", &options->minimumaccuracy, 0, 200, "%d");
	const char* palettenames[] = { "Viridis", "Inferno", "Turbo" };
	ImGui::Combo("Palette", &options->palette, palettenames, IM_ARRAYSIZE(palettenames));
	
	if (options->blurperaccuracy != oldBlurperaccurary) {
		oldBlurperaccurary = options->blurperaccuracy;
		lh->heatmap->MakeDirty();
	}

	if (options->gaussianblur != oldBlur) {
		oldBlur = options->gaussianblur;
		lh->heatmap->MakeDirty();
	}
	if (options->minimumaccuracy != oldMinimumaccuracy) {
		oldMinimumaccuracy = options->minimumaccuracy;
		lh->heatmap->MakeDirty();
	}

	ImGui::End();

	ImGui::Begin("Selected");
	ImGui::Checkbox("Show points", &options->showPoints);
	ImGui::SliderFloat("Point radius", &options->pointradius, 0, 100, "%.1f");
	ImGui::End();

	ImGui::Begin("Toolbar");
	if (ImGui::Button("Open")) {
		if (ChooseFile(lh)) {
			lh->isFileChosen = true;
			lh->isFullyLoaded = false;
			lh->isInitialised = false;
			lh->isLoadingFile = false;
		}
		lh->heatmap->MakeDirty();
	}
	if (ImGui::Button("Close")) {
		lh->isFileChosen = false;
		lh->isFullyLoaded = true;	//we're loaded with nothing
		lh->isInitialised = false;
		lh->isLoadingFile = false;
		if (!lh->locations.empty()) {
			lh->locations.clear();
		}
		lh->heatmap->MakeDirty();
	}
	if (ImGui::Button("Nav")) {
		lh->mouseInfo->mouseMode = MouseMode::ScreenNavigation;
	}
	if (ImGui::Button("Select")) {
		lh->mouseInfo->mouseMode = MouseMode::PointSelect;
	}
	if (ImGui::Button("Regions")) {
		lh->mouseInfo->mouseMode = MouseMode::RegionSelect;
	}




	ImGui::End();

	return;
}

void Gui::ShowRegionInfo(Region* r)
{
	ImGui::Text("N:%.4f S:%.4f W:%.4f E:%.4f", r->nswe.north, r->nswe.south, r->nswe.west, r->nswe.east);
	ImGui::Text("Height: %.2f Width:%.2f", r->nswe.height(), r->nswe.width());
	
	{
		float maxhour = 0;
		float fhours[24];
		for (int i = 0; i < 24; i++) {
			fhours[i] = r->hours[i];
			fhours[i] /= 3600;
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
			fdays[i] = (float)r->dayofweeks[i];
			fdays[i] /= 3600;
			if (fdays[i] > maxday) {
				maxday = fdays[i];
			}
		}
		ImGui::PlotHistogram("", fdays, 7, 0, "Time spent per weekday", 0, maxday, ImVec2(0, 80), sizeof(float));
	}

	ImGui::Text("Time (hours): %.1f", r->GetHoursInRegion());

	Gui::ListDatesInRegion(r);

	if (r->id > 0) {	//don't do this for the viewport region
		if (ImGui::Button("Delete")) {
			r->toDelete = true;
		}
	}
}

void Gui::ListDatesInRegion(Region* r)
{
	float hours = (float)r->minimumsecondstobeincludedinday;
	hours /= 60 * 60;

	std::vector<std::string> dates;
	r->FillVectorWithDates(dates);
	std::vector<const char*> cstrings{};
	for (const auto& string : dates)
		cstrings.push_back(string.c_str());
	
	int i;
	//ImGui::SliderFloat("Minimum hours", &hours, 0, 24, "%.1f", 1.0);
	r->minimumsecondstobeincludedinday = hours * 60 * 60;
	ImGui::Text("Days %i", r->numberofdays);
	ImGui::ListBox("Dates in region", &i, cstrings.data(), cstrings.size(), 4);
}

const char* Gui::BestSigFigsFormat(NSWE* nswe, RECTDIMENSION *rect)
{
	float dpp;	//degrees per pixel
	dpp = nswe->width()/rect->width;

	if (dpp > 1)	return "%.0f";
	if (dpp > 0.1)	return "%.1f";
	if (dpp > 0.01)	return "%.2f";
	if (dpp > 0.001)	return "%.3f";
	if (dpp > 0.0001)	return "%.4f";
	return "%.5f";
}

bool Gui::ChooseFile(LocationHistory * lh)
{
	OPENFILENAME ofn;	
	wchar_t filename[MAX_PATH];

	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(ofn);
	ofn.hInstance = GetModuleHandle(NULL);
	ofn.hwndOwner = GetActiveWindow();
	ofn.lpstrFile = filename;
	ofn.nMaxFile = sizeof(filename);
	ofn.lpstrTitle = L"Import";
	ofn.nFilterIndex = 1;
	//strcpy(filename, "*.json;");
#pragma warning(disable : 4996) //_CRT_SECURE_NO_WARNINGS
	std::mbstowcs(filename, "*.json;", 7);
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY |OFN_EXPLORER;
	ofn.lpstrFilter = L"All Files (*.*)\0*.*\0All Supported Files (*.json)\0*.json\0Google History JSON Files (*.json)\0*.json\0\0";

	bool result;
	result = GetOpenFileName(&ofn);
	
	if (result) {
		wprintf(L"Filename: %s\n", filename);
		lh->filename = filename;
		return true;
	}
	else
	{
		return false;
	}
}