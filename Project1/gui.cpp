#include <imgui/imgui.h>
#include "gui.h"
#include "header.h"

void Gui::MakeGUI(LocationHistory * lh)
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
			fdays[i] = testRegion->dayofweeks[i];
			fdays[i] /= 3600;
			if (fdays[i] > maxday) {
				maxday = fdays[i];
			}
		}
		ImGui::PlotHistogram("", fdays, 7, 0, "Time spent each day", 0, maxday, ImVec2(0, 80), sizeof(float));
	}

	ImGui::Text("Time (hours): %.1f", testRegion->GetHoursInRegion());

	ImGui::End();

	ImGui::Begin("Path drawing");
	ImGui::Checkbox("Show paths", &globalOptions.showPaths);
	ImGui::SliderFloat("Line thickness", &globalOptions.linewidth, 0.0f, 20.0f, "%.2f");
	ImGui::SliderFloat("Cycle", &globalOptions.cycle, 1.0f, 3600.0f * 7.0 * 24, "%.0f");
	ImGui::End();

	ImGui::Begin("Heatmap");
	ImGui::Checkbox("_Predict paths", &globalOptions.predictpath);
	ImGui::Checkbox("_Blur by accurracy", &globalOptions.blurperaccuracy);
	ImGui::SliderInt("Minimum accuracy", &globalOptions.minimumaccuracy, 0, 200, "%d");
	const char* items[] = { "Viridis", "Inferno" };
	ImGui::Combo("Palette", &globalOptions.palette, items, IM_ARRAYSIZE(items));
	ImGui::End();

	ImGui::Begin("Selected");
	ImGui::Checkbox("Show points", &globalOptions.showPoints);
	ImGui::SliderFloat("Point radius", &globalOptions.pointradius, 0, 100, "%.1f");
	ImGui::End();

	return;
}