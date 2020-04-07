#include <imgui/imgui.h>
#include "gui.h"
#include "header.h"
#include "nswe.h"
#include "regions.h"
#include "mytimezone.h"
#include <string>
#include <vector>

void Gui::MakeGUI(LocationHistory * lh)
{
	GlobalOptions * options;
	options = lh->globalOptions;

	std::string sigfigs;
	std::string sCoords;
	sigfigs = Gui::BestSigFigsFormat(lh->viewNSWE, lh->windowDimensions);
	


	ImGui::Begin("Map information");
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	
	sCoords = "Long: " + sigfigs + ", Lat: " + sigfigs;
	ImGui::Text(sCoords.c_str(), lh->longlatMouse->longitude, lh->longlatMouse->latitude);
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


	ImGui::Begin("Path drawing");
	ImGui::Checkbox("Show paths", &options->showPaths);
	ImGui::SliderFloat("Line thickness", &options->linewidth, 0.0f, 20.0f, "%.2f");
	ImGui::SliderFloat("Cycle", &options->cycle, 1.0f, 3600.0f * 7.0 * 24, "%.0f");
	ImGui::End();

	ImGui::Begin("Heatmap");
	ImGui::Checkbox("_Predict paths", &options->predictpath);
	ImGui::Checkbox("_Blur by accurracy", &options->blurperaccuracy);
	ImGui::SliderInt("Minimum accuracy", &options->minimumaccuracy, 0, 200, "%d");
	const char* items[] = { "Viridis", "Inferno", "Turbo" };
	ImGui::Combo("Palette", &options->palette, items, IM_ARRAYSIZE(items));
	ImGui::End();

	ImGui::Begin("Selected");
	ImGui::Checkbox("Show points", &options->showPoints);
	ImGui::SliderFloat("Point radius", &options->pointradius, 0, 100, "%.1f");
	ImGui::End();

	return;
}

void Gui::ShowRegionInfo(Region* r)
{
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
			fdays[i] = r->dayofweeks[i];
			fdays[i] /= 3600;
			if (fdays[i] > maxday) {
				maxday = fdays[i];
			}
		}
		ImGui::PlotHistogram("", fdays, 7, 0, "Time spent each day", 0, maxday, ImVec2(0, 80), sizeof(float));
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
	float hours = r->minimumsecondstobeincludedinday;
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
