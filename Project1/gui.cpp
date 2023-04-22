#include <imgui.h>
#include "gui.h"
#include "header.h"
#include "options.h"	//globalOptions
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

#undef IMGUI_DEFINE_PLACEMENT_NEW
#define IMGUI_DEFINE_PLACEMENT_NEW
#undef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>
#include "guiatlas.h"
#include "spectrum.h"
#include <iostream>


// Defined in winconsole.cpp
extern GlobalOptions globalOptions;

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

void Gui::DebugWindow(LocationHistory* lh,  MainViewport* vp)	{
	//Debug
	ImGui::Begin("Debug");
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::Text("Time to load: %.3f seconds. (%.1f MB/sec)", lh->secondsToLoad, (float)(lh->filesize/1024/1024)/lh->secondsToLoad);


	std::string sigfigs = Gui::BestSigFigsFormat(vp->DegreesPerPixel());
	std::string sCoords = "Mouse cursor: Long: " + sigfigs + ", Lat: " + sigfigs;

	ImGui::Text(sCoords.c_str(), MouseActions::longlatMouse.longitude, MouseActions::longlatMouse.latitude);
	ImGui::Text("DPMP: %f. PPD: %f. LOD: %i", 1000000.0f * vp->viewNSWE.width() / vp->windowDimensions.width,
		vp->windowDimensions.width / vp->viewNSWE.width(),
		lh->lodInfo.LodFromDPP(vp->DegreesPerPixel()));


	static int textureToView = 0;
	ImGui::InputInt("Texture", &textureToView);
	GLint maxTextureUnits;
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnits);

	if (textureToView < 0) { textureToView = maxTextureUnits - 1; }


	int w, h;
	glBindTexture(GL_TEXTURE_2D, (GLuint)textureToView);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);

	ImGui::Image((void*)textureToView, ImVec2(ImGui::GetWindowContentRegionMax().x, ImGui::GetWindowContentRegionMax().x * h / w));

	ImGui::End();
}

void Gui::ToolbarWindow(LocationHistory* lh)
{
	ImGui::Begin("Toolbar");
	if (ImGui::Button("Open")) {
		if (ChooseFileToOpen(lh)) {
			lh->isFileChosen = true;
			lh->isFullyLoaded = false;
			lh->isInitialised = false;
			lh->isLoadingFile = false;
		}
		//lh->heatmap->MakeDirty();
	}

	bool disabled = false;
	if (lh->isLoadingFile == true || lh->isInitialised == false) {
		disabled = true;
	}

	if (disabled)
		ImGui::BeginDisabled();

	if (ImGui::Button("Close")) {
		lh->CloseLocationFile();
		lh->filename = L"";
	}
	if (disabled)
		ImGui::EndDisabled();

	if (ImGui::Button("Save")) {
		if (lh->isFullyLoaded == true) {
			std::wstring filename = ChooseFileToSave(lh);
			if (filename.size() > 0) {
				SaveWVFormat(lh, filename);
			}
		}
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

	ToolbarButton(Icon::open);
	ToolbarButton(Icon::save);
	ToolbarButton(Icon::close);
	if (ToolbarButton(Icon::heatmap)) {
		globalOptions.ShowHeatmap();
	}
	if (ToolbarButton(Icon::points)) {
		globalOptions.ShowPoints();
	}


	ImGui::End();
}

void Gui::InfoWindow(LocationHistory* lh, MainViewport* vp)
{
	std::string sigfigs;	//this holds the string template (e.g. %.4f) that is best to display a unit at the current zoom
	std::string sCoords;
	sigfigs = Gui::BestSigFigsFormat(vp->DegreesPerPixel());

	ImGui::Begin("Map information");
	char displayfilename[260];
	size_t i;
	wcstombs_s(&i, displayfilename, 260, lh->filename.c_str(), 259);

	ImGui::Text("File name: %s", displayfilename);
	ImGui::Text("File size: %i", lh->filesize);

	ImGui::Text("Number of points: %i", lh->locations.size());

	sCoords = "N:" + sigfigs + ", S:" + sigfigs + ", W:" + sigfigs + ", E:" + sigfigs;
	ImGui::Text(sCoords.c_str(), vp->viewNSWE.north, vp->viewNSWE.south, vp->viewNSWE.west, vp->viewNSWE.east);

	ImGui::Text("Earliest: %s", MyTimeZone::FormatUnixTime(MyTimeZone::AsLocalTime(lh->stats.earliestTimestamp), MyTimeZone::FormatFlags::SHOW_TIME |globalOptions.dateFormat.GetDateCustomFormat()).c_str());
	ImGui::Text("Latest: %s", MyTimeZone::FormatUnixTime(MyTimeZone::AsLocalTime(lh->stats.latestTimestamp), MyTimeZone::FormatFlags::SHOW_TIME | globalOptions.dateFormat.GetDateCustomFormat()).c_str());

	//Gui::ShowRegionInfo(vp->regions[0], globalOptions);

	ImGui::End();

}

void Gui::SettingsWindow()
{
	ImGui::Begin("Display settings");
	ImGui::Text("Date order");

	constexpr long demoTime = 1618285680;	//13th, so not ambiguous

	static int e = 0;
	std::string dmy = MyTimeZone::FormatUnixTime(demoTime, (globalOptions.dateFormat.GetDateCustomFormat() & ~MyTimeZone::FormatFlags::DMY & ~MyTimeZone::FormatFlags::MDY) | MyTimeZone::FormatFlags::DMY);
	std::string mdy = MyTimeZone::FormatUnixTime(demoTime, (globalOptions.dateFormat.GetDateCustomFormat() & ~MyTimeZone::FormatFlags::DMY & ~MyTimeZone::FormatFlags::MDY) | MyTimeZone::FormatFlags::MDY);
	std::string ymd = MyTimeZone::FormatUnixTime(demoTime, (globalOptions.dateFormat.GetDateCustomFormat() & ~MyTimeZone::FormatFlags::DMY & ~MyTimeZone::FormatFlags::MDY));

	if (globalOptions.dateFormat.GetDateCustomFormat() & MyTimeZone::FormatFlags::DMY) {
		e = MyTimeZone::FormatFlags::DMY;
	}
	else if (globalOptions.dateFormat.GetDateCustomFormat() & MyTimeZone::FormatFlags::MDY) {
		e = MyTimeZone::FormatFlags::MDY;
	}
	else
		e = MyTimeZone::FormatFlags::YMD;

	ImGui::RadioButton(dmy.c_str(), &e, MyTimeZone::FormatFlags::DMY);
	ImGui::RadioButton(mdy.c_str(), &e, MyTimeZone::FormatFlags::MDY);
	ImGui::RadioButton(ymd.c_str(), &e, MyTimeZone::FormatFlags::YMD);
	globalOptions.dateFormat.SetDateOrder(e);

	ImGui::Text("Month");
	std::string month_num = MyTimeZone::FormatUnixTime(demoTime, (globalOptions.dateFormat.GetDateCustomFormat() & ~MyTimeZone::FormatFlags::MONTH_SHORT ));
	std::string month_short = MyTimeZone::FormatUnixTime(demoTime, (globalOptions.dateFormat.GetDateCustomFormat() & ~MyTimeZone::FormatFlags::MONTH_SHORT) | MyTimeZone::FormatFlags::MONTH_SHORT);
//	std::string month_long = MyTimeZone::FormatUnixTime(demoTime, (globalOptions.GetDateCustomFormat() & ~MyTimeZone::FormatFlags::DMY & ~MyTimeZone::FormatFlags::MDY));

	if (globalOptions.dateFormat.GetDateCustomFormat() & MyTimeZone::FormatFlags::MONTH_SHORT) {
		e = MyTimeZone::FormatFlags::MONTH_SHORT;
	}
	else {
		e = MyTimeZone::FormatFlags::MONTH_NUM;
	}

	ImGui::RadioButton(month_num.c_str(), &e, MyTimeZone::FormatFlags::MONTH_NUM);
	ImGui::RadioButton(month_short.c_str(), &e, MyTimeZone::FormatFlags::MONTH_SHORT);
//	ImGui::RadioButton(ymd.c_str(), &e, MyTimeZone::FormatFlags::YMD);
	globalOptions.dateFormat.SetMonthFormat(e);




	ImGui::End();
}

void Gui::ChooseBackgroundWindow()
{
	ImGui::Begin("Background image");
	ImGui::Text("filename");
	ImGui::Button("...##Change Filename");
	ImGui::End();
}

void Gui::MakeGUI(LocationHistory* lh, MainViewport *vp)
{
	Gui::DebugWindow(lh, vp);
	Gui::InfoWindow(lh, vp);

	//delete any regions marked to be deleted
	for (std::size_t i = 1; i < vp->regions.size(); i++) {
		if (vp->regions[i]->toDelete) {
			vp->regions.erase(vp->regions.begin() + i);

			vp->regions[i - 1]->needsRedraw = true;
		}
	}

	//display the info for all regions
	for (std::size_t i = 1; i < vp->regions.size(); i++) {
		if (vp->regions[i]->shouldShowWindow) {
			ImGui::Begin((vp->regions[i]->displayname + "###regionwindow" + std::to_string(vp->regions[i]->id)).c_str());
			Gui::ShowRegionInfo(vp->regions[i]);
			ImGui::End();
		}
	}

	ImGui::ShowDemoWindow();

	Gui::SettingsWindow();
	Gui::ChooseBackgroundWindow();


	ImGui::Begin("Time selection");
	Gui::DateSelect(lh);
	ImGui::End();

	ImGui::Begin("Options");
	if (globalOptions.IsHeatmapVisible()) {
		Gui::HeatmapOptions();
	}
	if (globalOptions.IsPointsVisible()) {
		Gui::PointsOptions(lh);
	}
	ImGui::End();


	Gui::ToolbarWindow(lh);

	return;
}

bool Gui::ToolbarButton(enum class Icon icon) {
	
	const AtlasEntry& entry = guiAtlas.GetEntry(icon);
	bool b = ImGui::ImageButton(entry.GetName().c_str(), (void*)guiAtlas.GetTextureId(), entry.GetSize(), entry.GetUV0(), entry.GetUV1());
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip(entry.GetName().c_str());
	}

	return b;
}

void Gui::PointsOptions(LocationHistory* lh)
{
	ImGui::SliderFloat("Point size", &globalOptions.pointdiameter, 0.0f, 10.0f, "%.1f pixels");
	ImGui::SliderFloat("Opacity", &globalOptions.pointalpha, 0.0f, 1.0f, "%.2f");
	ImGui::Checkbox("Connect points", &globalOptions.showPaths);

	if (globalOptions.showPaths) {
		ImGui::SliderFloat("Line thickness", &globalOptions.linewidth, 1.0f, 8.0f, "%.1f");
		globalOptions.showPoints = false;
	}
	ImGui::Checkbox("Travel highlight", &globalOptions.showHighlights);
	if (globalOptions.showHighlights) {
		ImGui::SliderFloat("Highlight distance", &globalOptions.minutestravelbetweenhighlights, 5.0f, 24.0f * 60.0f, "%.1f minutes");
		ImGui::SliderFloat("Cycle frequency", &globalOptions.secondsbetweenhighlights, 1.0f, 60.0f, "%.1f seconds");
		float motionSpeedX;
		motionSpeedX = globalOptions.minutestravelbetweenhighlights * 60.0f / globalOptions.secondsbetweenhighlights;
		bool b;
		b = ImGui::SliderFloat("Motion speed", &motionSpeedX, 1.0, 3600.0, "%.0fX");
		if (b) { globalOptions.secondsbetweenhighlights = globalOptions.minutestravelbetweenhighlights * 60.0f / motionSpeedX; }
	}

	const char* colourbynames[] = { "Time", "Hour of day", "Day of week", "Month of year", "Year" };
	ImGui::Combo("Colour by", &globalOptions.colourby, colourbynames, IM_ARRAYSIZE(colourbynames));

	static ImVec4 color[24] = {};

	if (globalOptions.colourby == 4) {
		globalOptions.indexPaletteYear = Palette_Handler::MatchingPalette(globalOptions.indexPaletteYear, Palette::YEAR);
		int n = 0;
		for (int year = MyTimeZone::GetYearFromTimestamp(lh->stats.earliestTimestamp); (year < MyTimeZone::GetYearFromTimestamp(lh->stats.latestTimestamp) + 1) && (n < 24); year++) {
			color[n] = Palette_Handler::PaletteColorImVec4(globalOptions.indexPaletteYear, year);

			std::string text = "Year ";
			text += std::to_string(year);

			if (ImGui::ColorEdit4(text.c_str(), (float*)&color[n], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaPreviewHalf | ImGuiColorEditFlags_AlphaBar)) {
				Palette_Handler::SetColourImVec4(globalOptions.indexPaletteYear, year, color[n]);
			}
			if ((n + 1) % 6) { ImGui::SameLine(); }
			n++;
		}
		if (ImGui::Button(Palette_Handler::PaletteName(globalOptions.indexPaletteYear).c_str())) {
			globalOptions.indexPaletteYear = Palette_Handler::NextMatchingPalette(globalOptions.indexPaletteYear, Palette::YEAR);
		}
		if (ImGui::Button("left")) {
			Palette_Handler::RotatePaletteLeft(globalOptions.indexPaletteYear);
		}
		if (ImGui::Button("right")) {
			Palette_Handler::RotatePaletteRight(globalOptions.indexPaletteYear);
		}
	}

	if (globalOptions.colourby == 2) {
		for (int i = 0; i < 7; i++) {
			globalOptions.indexPaletteWeekday = Palette_Handler::MatchingPalette(globalOptions.indexPaletteWeekday, Palette::WEEKDAY);
			color[i] = Palette_Handler::PaletteColorImVec4(globalOptions.indexPaletteWeekday, i);
			if (ImGui::ColorEdit4(MyTimeZone::daynames[i].c_str(), (float*)&color[i], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaPreviewHalf | ImGuiColorEditFlags_AlphaBar)) {
				Palette_Handler::SetColourImVec4(globalOptions.indexPaletteWeekday, i, color[i]);
			}
			ImGui::SameLine();
		}
		if (ImGui::Button(Palette_Handler::PaletteName(globalOptions.indexPaletteWeekday).c_str())) {
			globalOptions.indexPaletteWeekday = Palette_Handler::NextMatchingPalette(globalOptions.indexPaletteWeekday, Palette::WEEKDAY);
		}
		ImGui::PushButtonRepeat(true);
		if (ImGui::ArrowButton("##rotpalleft", ImGuiDir_Left)) {
			Palette_Handler::RotatePaletteLeft(globalOptions.indexPaletteWeekday);
		}
		if (ImGui::ArrowButton("##rotpalright", ImGuiDir_Right)) {
			Palette_Handler::RotatePaletteRight(globalOptions.indexPaletteWeekday);
		}
		ImGui::PopButtonRepeat();
	}

	if (globalOptions.colourby == 1) {
		globalOptions.indexPaletteHour = Palette_Handler::MatchingPalette(globalOptions.indexPaletteHour, Palette::HOUR);
		for (int i = 0; i < 24; i++) {
			color[i] = Palette_Handler::PaletteColorImVec4(globalOptions.indexPaletteHour, i);
			std::string text = "Hour ";
			text += std::to_string(i);

			if (ImGui::ColorEdit4(text.c_str(), (float*)&color[i], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaPreviewHalf | ImGuiColorEditFlags_AlphaBar)) {
				Palette_Handler::SetColourImVec4(globalOptions.indexPaletteHour, i, color[i]);
			}
			if ((i + 1) % 6) { ImGui::SameLine(); }
		}
	}
	if (globalOptions.colourby == 0) {
		const char* cyclenames[] = { "One minute", "One hour", "One day", "One week", "One month", "One year", "Five years", "Other" };
		const float cycleresults[] = { 60.0f,3600.0f,3600.0f * 24.0f,3600.0f * 24.0f * 7.0f,3600.0f * 24.0f * 365.25f / 12.0f,3600.0f * 24.0f * 365.25f ,3600.0f * 24.0f * 365.25f * 5.0f,0.0f };

		static int uiCycleSelect = 0;
		if (ImGui::Combo("Cycle over", &uiCycleSelect, cyclenames, IM_ARRAYSIZE(cyclenames))) {
			if (uiCycleSelect < IM_ARRAYSIZE(cyclenames) - 1) {
				globalOptions.cycleSeconds = cycleresults[uiCycleSelect];
			}
		}
		if (ImGui::SliderFloat("Cycle", &globalOptions.cycleSeconds, 60.0f, 3600.0f * 24.0f * 365.0f * 5.0f, "%.0f seconds", 6.0f)) {
			uiCycleSelect = 7;
			for (int i = 0; i < IM_ARRAYSIZE(cyclenames) - 1; i++) {
				if (globalOptions.cycleSeconds == cycleresults[i]) {
					uiCycleSelect = i;
				}
			}
		}
	}
}

void Gui::HeatmapOptions()
{

	//ImGui::Checkbox("Show heatmap", &globalOptions.showHeatmap);
	ImGui::SliderFloat("Gaussian blur", &globalOptions.gaussianblur, 0.0f, 25.0f, "%.1f");
	ImGui::SliderFloat("Max value", &globalOptions.heatmapmaxvalue, 0.0f, 1000000.0f, "%.1f", ImGuiSliderFlags_Logarithmic);
	ImGui::SliderInt("Minimum accuracy", &globalOptions.minimumaccuracy, 0, 200, "%d");
	ImGui::SliderFloat("Debug", &globalOptions.debug, 0.0f, 1.0f, "%.1f");

	//const char* palettenames[] = { "Viridis", "Inferno", "Turbo","Test"};
	std::vector<std::string> spectrumNames = Spectrum_Handler::ListSpectrums();
	
	ImGui::Combo("Palette", &globalOptions.heatmapPaletteIndex,
		[](void* data, int idx, const char** out_text) {
			auto& names = *static_cast<std::vector<std::string>*>(data);
	*out_text = names[idx].c_str();
	return true;
		}, &spectrumNames, static_cast<int>(spectrumNames.size()));
	
	

	//const AtlasEntry& entry = guiAtlas.GetEntry(Icon::spectrum);
	RectDimension displaySize(256, 32);
	UVpair spectrumUV = guiAtlas.GetSpectrumUV(globalOptions.heatmapPaletteIndex);
	
	if (ImGui::ArrowButton("##rotpalleft", ImGuiDir_Left)) {
		globalOptions.heatmapPaletteIndex--;
		if (globalOptions.heatmapPaletteIndex < 0) { globalOptions.heatmapPaletteIndex = Spectrum_Handler::GetNumberOfSpectrums()-1; }
	}
	ImGui::SameLine();
	//ImGui::Text("%s", Spectrum_Handler::GetSpectrumName(globalOptions.heatmapPaletteIndex));
	//ImGui::SameLine();
	ImGui::Image((void*)guiAtlas.GetTextureId(), displaySize, spectrumUV.uv0, spectrumUV.uv1);
	ImGui::SameLine();
	if (ImGui::ArrowButton("##rotpalright", ImGuiDir_Right)) {
		globalOptions.heatmapPaletteIndex++;
		if (globalOptions.heatmapPaletteIndex >= Spectrum_Handler::GetNumberOfSpectrums()) { globalOptions.heatmapPaletteIndex = 0; }
	}


}

void Gui::DateSelect(LocationHistory* lh)
{


	std::string earliestTimeString = MyTimeZone::FormatUnixTime(MyTimeZone::AsLocalTime(globalOptions.earliestTimeToShow), globalOptions.dateFormat.GetDateCustomFormat()).c_str();

	ImGui::SliderScalar("Earliest date", ImGuiDataType_U32, &globalOptions.earliestTimeToShow, &lh->stats.earliestTimestamp, &lh->stats.latestTimestamp, earliestTimeString.c_str());

	std::string latestTimeString = MyTimeZone::FormatUnixTime(MyTimeZone::AsLocalTime(globalOptions.latestTimeToShow),  globalOptions.dateFormat.GetDateCustomFormat()).c_str();

	ImGui::SliderScalar("Latest date", ImGuiDataType_U32, &globalOptions.latestTimeToShow, &lh->stats.earliestTimestamp, &lh->stats.latestTimestamp, latestTimeString.c_str());

	//Advance by a day
	ImGui::PushButtonRepeat(true);
	int advance = 1;
	if (ImGui::GetIO().KeyCtrl) { advance = 7; }
	if (ImGui::ArrowButton("##minusday", ImGuiDir_Left)) {
		globalOptions.earliestTimeToShow = MyTimeZone::AdvanceByDays(globalOptions.earliestTimeToShow, -advance);
	}
	ImGui::SameLine();
	if (ImGui::ArrowButton("##plusday", ImGuiDir_Right)) {
		globalOptions.earliestTimeToShow = MyTimeZone::AdvanceByDays(globalOptions.earliestTimeToShow, advance);
	}
	ImGui::PopButtonRepeat();


	//ensure the earliest time is always H=0, min=0,sec=0, latest 1 to midnight
	globalOptions.earliestTimeToShow = MyTimeZone::DateWithThisTime(globalOptions.earliestTimeToShow, 0, 0, 0);
	globalOptions.latestTimeToShow = MyTimeZone::DateWithThisTime(globalOptions.latestTimeToShow, 23, 59, 59);


	//constrain to the possible points, earliest
	if (globalOptions.earliestTimeToShow > MyTimeZone::AsLocalTime(lh->stats.latestTimestamp)) {
		globalOptions.earliestTimeToShow = MyTimeZone::AsLocalTime((lh->stats.latestTimestamp));
	}
	if (globalOptions.earliestTimeToShow < MyTimeZone::AsLocalTime(lh->stats.earliestTimestamp)) {
		globalOptions.earliestTimeToShow = MyTimeZone::AsLocalTime(lh->stats.earliestTimestamp);
	}
	//then latest
	if (globalOptions.latestTimeToShow > MyTimeZone::AsLocalTime(lh->stats.latestTimestamp)) {
		globalOptions.latestTimeToShow = MyTimeZone::AsLocalTime((lh->stats.latestTimestamp));
	}
	if (globalOptions.latestTimeToShow < MyTimeZone::AsLocalTime(lh->stats.earliestTimestamp)) {
		globalOptions.latestTimeToShow = MyTimeZone::AsLocalTime(lh->stats.earliestTimestamp);
	}


}

void Gui::ShowRegionInfo(Region* r)
{
	static char str1[128];
	strcpy_s(str1, r->displayname.c_str());
	if (ImGui::InputTextWithHint("Region name", "Name the region", str1, IM_ARRAYSIZE(str1))) {
		r->displayname = str1;
	}

	static ImVec4 colour;
	colour = r->colour.AsImVec4();
	if (ImGui::ColorEdit3("Colour", (float*)&colour, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
		r->colour = colour;
		r->needsRedraw = true;
	}

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
		DayHistogram(r, 80.f);
	}

	ImGui::Text("Time (hours): %.1f", r->GetHoursInRegion());

	Gui::ListDatesInRegion(r);

	if (r->id > 0) {	//don't do this for the viewport region
		if (ImGui::Button("Delete")) {
			r->toDelete = true;
		}
	}
}

void Gui::DayHistogram(Region* r, float height)
{
	ImVec2 frame_size;
	const char* label = "Time per weekday";

	float maxday = 0;
	float fdays[7]{};
	float totaldays = 0;

	for (int i = 0; i < 7; i++) {
		fdays[i] = (float)r->dayofweeks[i];
		//fdays[i] /= 3600;
		if (fdays[i] > maxday) {
			maxday = fdays[i];
		}
		totaldays += fdays[i];
	}

	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return;

	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);

	const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
	if (frame_size.x == 0.0f)
		frame_size.x = ImGui::CalcItemWidth();
	if (frame_size.y == 0.0f)
		frame_size.y = height;

	const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + frame_size);
	const ImRect inner_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);
	const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0));
	ImGui::ItemSize(total_bb, style.FramePadding.y);
	if (!ImGui::ItemAdd(total_bb, 0, &frame_bb))
		return;
	const bool hovered = ImGui::ItemHoverable(frame_bb, id);

	ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);

	if (hovered && inner_bb.Contains(g.IO.MousePos))
	{
		const float t = ImClamp((g.IO.MousePos.x - inner_bb.Min.x) / (inner_bb.Max.x - inner_bb.Min.x), 0.0f, 0.9999f);
		const int v_idx = (int)(t * 7);

		std::string timeunitstodisplay;

		ImGui::SetTooltip("%s: %s", MyTimeZone::daynameslong[(v_idx + 6) % 7].c_str(), MyTimeZone::DisplayBestTimeUnits(fdays[v_idx]));
	}

	ImVec2 topleftofbar, bottomrightofbar;
	topleftofbar.x = inner_bb.Min.x;
	bottomrightofbar.y = inner_bb.Max.y - label_size.y;

	int gap = 1;
	float thickness = inner_bb.GetWidth() / 7 - gap;

	if (thickness < 2.0) {
		gap = 0;
		inner_bb.GetWidth() / 7;
	}

	for (int i = 0; i < 7; i++) {
		bottomrightofbar.x = topleftofbar.x + thickness;
		topleftofbar.y = inner_bb.Max.y - label_size.y - (fdays[i] / maxday * (inner_bb.GetHeight()-label_size.y));
		window->DrawList->AddRectFilled(topleftofbar, bottomrightofbar, Palette_Handler::PaletteColorImU32(globalOptions.indexPaletteWeekday, i));

		int percentint;
		percentint = static_cast<int>(fdays[i] * 100.0f / totaldays + 0.5f);
		std::string percent = std::to_string(percentint) + "%";

		
		//print the percentage on the bar
		ImGui::RenderTextClipped(topleftofbar, bottomrightofbar, percent.c_str(), NULL, NULL, ImVec2(0.5f, 0.0f));

		//the label
		ImGui::RenderTextClipped(ImVec2(topleftofbar.x, bottomrightofbar.y ), ImVec2(bottomrightofbar.x, bottomrightofbar.y+ label_size.y), MyTimeZone::daynames[(i + 6) % 7].c_str(), NULL, NULL, ImVec2(0.5f, 0.0f));

		topleftofbar.x += thickness + gap;
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
	ImGui::SliderFloat("Minimum hours", &hours, 0, 24, "%.1f", 1.0);
	r->minimumsecondstobeincludedinday = hours * 60 * 60;
	ImGui::Text("Days %i", r->numberofdays);
	ImGui::ListBox("Dates in region", &i, cstrings.data(), cstrings.size(), 4);
}

const char* Gui::BestSigFigsFormat(const double dpp)
{
	//const double dpp = vp->DegreesPerPixel();

	if (dpp > 1)	return "%.0f";
	if (dpp > 0.1)	return "%.1f";
	if (dpp > 0.01)	return "%.2f";
	if (dpp > 0.001)	return "%.3f";
	if (dpp > 0.0001)	return "%.4f";
	return "%.5f";
}

bool Gui::ChooseFileToOpen(LocationHistory* lh)
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
	ofn.lpstrDefExt = L"json";
	ofn.nFilterIndex = 1;

#pragma warning(disable : 4996) //_CRT_SECURE_NO_WARNINGS
	std::mbstowcs(filename, "*.json;*.wvf", 259);
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER;
	ofn.lpstrFilter = L"All Files (*.*)\0*.*\0All Supported Files (*.json, *.wvf)\0*.json;*.wvf\0Google History JSON Files (*.json)\0*.json\0WorldView Files (*.wvf)\0*.wvf\0\0";

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

std::wstring Gui::ChooseFileToSave(LocationHistory* lh)
{
	OPENFILENAME ofn;
	wchar_t filename[MAX_PATH];

	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(ofn);
	ofn.hInstance = GetModuleHandle(NULL);
	ofn.hwndOwner = GetActiveWindow();
	ofn.lpstrFile = filename;
	ofn.nMaxFile = sizeof(filename);
	ofn.lpstrTitle = L"Export";
	ofn.nFilterIndex = 1;
	ofn.lpstrDefExt = L"wvf";

#pragma warning(disable : 4996) //_CRT_SECURE_NO_WARNINGS
	std::mbstowcs(filename, "*.wvf", 259);
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER;
	ofn.lpstrFilter = L"All Files (*.*)\0*.*\0WorldView Files (*.wvf)\0*.wvf\0\0";

	bool result;
	result = GetSaveFileName(&ofn);

	if (result) {
		//wprintf(L"Filename: %s\n", filename);
		return filename;
	}
	else
	{
		return L"";
	}
}