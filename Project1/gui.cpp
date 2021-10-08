#include <imgui.h>
#include "gui.h"
#include "header.h"
#include "nswe.h"
#include "regions.h"
#include "mytimezone.h"
#include "input.h"
#include "heatmap.h"
#include "palettes.h"
#include <string>
#include <vector>
#include <ctime>

#include <Windows.h>

void Gui::ShowLoadingWindow(LocationHistory* lh)
{
	ImGui::SetNextWindowSize(ImVec2(500.0f, 140.0f));
	ImGui::SetNextWindowPos(ImVec2(200.0f, 300.0f));
	ImGui::Begin("Loading Location History", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
	float p = (float)lh->totalbytesread / (float)lh->filesize;
	if (lh->totalbytesread < lh->filesize) {
		char displayfilename[260];
		size_t i;
		wcstombs_s(&i, displayfilename, 260, lh->filename.c_str(), 259);

		ImGui::Text("File name: %s", displayfilename);
		ImGui::Text("Processed %.1f MB (of %.1f MB)", (float)lh->totalbytesread / 0x100000, (float)lh->filesize / 0x100000);
	}
	else {
		ImGui::Text("Initialising...");
	}
	ImGui::ProgressBar(p);
	ImGui::End();
}

void Gui::MakeGUI(LocationHistory* lh)
{
	GlobalOptions* options;
	options = lh->globalOptions;

	std::string sigfigs;	//this holds the string template (e.g. %.4f) that is best to display a unit at the current zoom
	std::string sCoords;
	sigfigs = Gui::BestSigFigsFormat(lh->viewNSWE, lh->windowDimensions);

	ImGui::Begin("Map information");
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

	char displayfilename[260];
	size_t i;
	wcstombs_s(&i, displayfilename, 260, lh->filename.c_str(), 259);

	ImGui::Text("File name: %s", displayfilename);
	ImGui::Text("File size: %i", lh->filesize);

	sCoords = "Cursor: Long: " + sigfigs + ", Lat: " + sigfigs;
	ImGui::Text(sCoords.c_str(), MouseActions::longlatMouse.longitude, MouseActions::longlatMouse.latitude);
	ImGui::Text("Number of points: %i", lh->locations.size());

	sCoords = "N:" + sigfigs + ", S:" + sigfigs + ", W:" + sigfigs + ", E:" + sigfigs;
	ImGui::Text(sCoords.c_str(), lh->viewNSWE->north, lh->viewNSWE->south, lh->viewNSWE->west, lh->viewNSWE->east);

	ImGui::Text("Earliest: %s", MyTimeZone::FormatUnixTime(MyTimeZone::FixToLocalTime(lh->earliesttimestamp), MyTimeZone::FormatFlags::SHOW_TIME | MyTimeZone::FormatFlags::DMY).c_str());
	ImGui::Text("Latest: %s", MyTimeZone::FormatUnixTime(MyTimeZone::FixToLocalTime(lh->latesttimestamp), MyTimeZone::FormatFlags::SHOW_TIME | MyTimeZone::FormatFlags::TEXT_MONTH).c_str());

	Gui::ShowRegionInfo(lh->regions[0]);

	ImGui::End();

	//delete any regions marked to be deleted
	for (std::size_t i = 1; i < lh->regions.size(); i++) {
		if (lh->regions[i]->toDelete) {
			lh->regions.erase(lh->regions.begin() + i);
			
			lh->regions[i-1]->needsRedraw = true;
		}
	}
	
	for (std::size_t i = 1; i < lh->regions.size(); i++) {
		if (lh->regions[i]->shouldShowWindow) {
			ImGui::Begin(lh->regions[i]->displayname.c_str());
			Gui::ShowRegionInfo(lh->regions[i]);
			ImGui::End();
		}
	}

	ImGui::ShowDemoWindow();
	//ImGui::ShowStyleEditor();

	static float oldBlur = 0;
	static float oldMinimumaccuracy = 0;
	static float oldBlurperaccurary = 0;
	ImGui::Begin("Heatmap");
	ImGui::Checkbox("Show heatmap", &options->showHeatmap);
	ImGui::SliderFloat("Gaussian blur", &options->gaussianblur, 0.0f, 10.0f, "%.1f");
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
	
	ImGui::Begin("Location display");
	ImGui::Checkbox("Show points", &options->showPoints);

	static int earliestDayOfMonth = 0;
	static int earliestMonth = 0;
	static int earliestYear = 0;

	earliestDayOfMonth = 0;
	time_t t;
	struct std::tm correctedTime;

	t = options->earliestTimeToShow;
	gmtime_s(&correctedTime, &t);
	earliestDayOfMonth = correctedTime.tm_mday;
	

	ImGui::Text("%i", earliestDayOfMonth);
	ImGui::SliderScalar("Earliest date", ImGuiDataType_U32, &options->earliestTimeToShow, &lh->earliesttimestamp, &lh->latesttimestamp, "%u");
	ImGui::SliderScalar("Latest date", ImGuiDataType_U32, &options->latestTimeToShow, &lh->earliesttimestamp, &lh->latesttimestamp, "%u");

	

	ImGui::SliderFloat("Point size", &options->pointdiameter, 0.0f, 10.0f, "%.1f pixels");
	ImGui::SliderFloat("Opacity", &options->pointalpha, 0.0f, 1.0f, "%.2f");
	ImGui::Checkbox("Connect points", &options->showPaths);
	if (options->showPaths) {
		ImGui::SliderFloat("Line thickness", &options->linewidth, 1.0f, 8.0f, "%.1f");
	}
	ImGui::Checkbox("Travel highlight", &options->showHighlights);
	if (options->showHighlights) {
		ImGui::SliderFloat("Highlight distance", &options->minutestravelbetweenhighlights, 5.0f, 24.0f * 60.0f, "%.1f minutes");
			ImGui::SliderFloat("Cycle frequency", &options->secondsbetweenhighlights, 1.0f, 60.0f, "%.1f seconds");
			float motionSpeedX;
			motionSpeedX = options->minutestravelbetweenhighlights * 60.0f / options->secondsbetweenhighlights;
			bool b;
			b = ImGui::SliderFloat("Motion speed", &motionSpeedX, 1.0, 3600.0, "%.0fX");
			if (b) { options->secondsbetweenhighlights = options->minutestravelbetweenhighlights * 60.0f / motionSpeedX; }
	}

	const char* colourbynames[] = { "Time", "Hour of day", "Day of week", "Month of year", "Year" };
	ImGui::Combo("Colour by", &options->colourby, colourbynames, IM_ARRAYSIZE(colourbynames));

	static ImVec4 color[24] = {};
	


	if (options->colourby == 4) {
		options->indexPaletteYear = Palette_Handler::MatchingPalette(options->indexPaletteYear, Palette::YEAR);
		int n = 0;
		for (int year = MyTimeZone::GetYearFromTimestamp(lh->earliesttimestamp); (year < MyTimeZone::GetYearFromTimestamp(lh->latesttimestamp)+1) && (n < 24); year++) {
			color[n] = Palette_Handler::PaletteColorImVec4(options->indexPaletteYear, year);

			std::string text = "Year ";
			text += std::to_string(year);

			if (ImGui::ColorEdit4(text.c_str(), (float*)&color[n], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaPreviewHalf | ImGuiColorEditFlags_AlphaBar)) {
				Palette_Handler::SetColourImVec4(options->indexPaletteYear, year, color[n]);
			}
			if ((n + 1) % 6) { ImGui::SameLine(); }
			n++;
		}
		if (ImGui::Button(Palette_Handler::PaletteName(options->indexPaletteYear).c_str())) {
			options->indexPaletteYear = Palette_Handler::NextMatchingPalette(options->indexPaletteYear, Palette::YEAR);
		}
		if (ImGui::Button("left")) {
			Palette_Handler::RotatePaletteLeft(options->indexPaletteYear);
		}
		if (ImGui::Button("right")) {
			Palette_Handler::RotatePaletteRight(options->indexPaletteYear);
		}
	}


	if (options->colourby == 2) {
		for (int i = 0; i < 7; i++) {
			options->indexPaletteWeekday = Palette_Handler::MatchingPalette(options->indexPaletteWeekday, Palette::WEEKDAY);
			color[i]= Palette_Handler::PaletteColorImVec4(options->indexPaletteWeekday, i);
			if (ImGui::ColorEdit4(MyTimeZone::daynames[i].c_str(), (float*)&color[i], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaPreviewHalf | ImGuiColorEditFlags_AlphaBar)) {
				Palette_Handler::SetColourImVec4(options->indexPaletteWeekday, i, color[i]);
			}
			ImGui::SameLine();
		}
		if (ImGui::Button(Palette_Handler::PaletteName(options->indexPaletteWeekday).c_str())) {
			options->indexPaletteWeekday = Palette_Handler::NextMatchingPalette(options->indexPaletteWeekday, Palette::WEEKDAY);
		}
		ImGui::PushButtonRepeat(true);
		if (ImGui::ArrowButton("##rotpalleft", ImGuiDir_Left)) {
			Palette_Handler::RotatePaletteLeft(options->indexPaletteWeekday);
		}
		if (ImGui::ArrowButton("##rotpalright", ImGuiDir_Right)) {
			Palette_Handler::RotatePaletteRight(options->indexPaletteWeekday);
		}
		ImGui::PopButtonRepeat();

	}

	if (options->colourby == 1) {
		options->indexPaletteHour = Palette_Handler::MatchingPalette(options->indexPaletteHour, Palette::HOUR);
		for (int i = 0; i < 24; i++) {
			color[i] = Palette_Handler::PaletteColorImVec4(options->indexPaletteHour, i);
			std::string text = "Hour ";
			text += std::to_string(i);

			if (ImGui::ColorEdit4(text.c_str(), (float*)&color[i], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaPreviewHalf | ImGuiColorEditFlags_AlphaBar)) {
				Palette_Handler::SetColourImVec4(options->indexPaletteHour, i, color[i]);
			}
			if ((i + 1) % 6) { ImGui::SameLine(); }
		}
	}
	if (options->colourby == 0) {
		const char* cyclenames[] = { "One minute", "One hour", "One day", "One week", "One month", "One year", "Five years", "Other" };
		const float cycleresults[] = { 60.0f,3600.0f,3600.0f * 24.0f,3600.0f * 24.0f * 7.0f,3600.0f * 24.0f * 365.25f / 12.0f,3600.0f * 24.0f * 365.25f ,3600.0f * 24.0f * 365.25f * 5.0f,0.0f };

		static int uiCycleSelect = 0;
		if (ImGui::Combo("Cycle over", &uiCycleSelect, cyclenames, IM_ARRAYSIZE(cyclenames))) {
			if (uiCycleSelect < IM_ARRAYSIZE(cyclenames) - 1) {
				options->cycleSeconds = cycleresults[uiCycleSelect];
			}
		}
		if (ImGui::SliderFloat("Cycle", &options->cycleSeconds, 60.0f, 3600.0f * 24.0f * 365.0f * 5.0f, "%.0f seconds", 6.0f)) {
			uiCycleSelect = 7;
			for (int i = 0; i < IM_ARRAYSIZE(cyclenames) - 1; i++) {
				if (options->cycleSeconds == cycleresults[i]) {
					uiCycleSelect = i;
				}
			}
		}
	}

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
		MouseActions::mouseMode = MouseMode::ScreenNavigation;
	}
	if (ImGui::Button("Select")) {
		MouseActions::mouseMode = MouseMode::PointSelect;
	}
	if (ImGui::Button("Regions")) {
		MouseActions::mouseMode = MouseMode::RegionSelect;
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

const char* Gui::BestSigFigsFormat(NSWE* nswe, RectDimension rect)
{
	float dpp;	//degrees per pixel
	dpp = nswe->width() / rect.width;

	if (dpp > 1)	return "%.0f";
	if (dpp > 0.1)	return "%.1f";
	if (dpp > 0.01)	return "%.2f";
	if (dpp > 0.001)	return "%.3f";
	if (dpp > 0.0001)	return "%.4f";
	return "%.5f";
}

bool Gui::ChooseFile(LocationHistory* lh)
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

#pragma warning(disable : 4996) //_CRT_SECURE_NO_WARNINGS
	std::mbstowcs(filename, "*.json;", 7);
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER;
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