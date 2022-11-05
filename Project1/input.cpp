#include <glad/glad.h> 
#include <GLFW/glfw3.h>
#include "header.h"
#include "input.h"
#include "nswe.h"
#include "heatmap.h"
#include "regions.h"
#include "mygl.h"
#include <stdio.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>


//extern LocationHistory* pLocationHistory;

NSWE originalNSWE;

//set defaults
bool MouseActions::isDragging = false;
int MouseActions::lmbState = 0;
int MouseActions::rmbState = 0;
double MouseActions::xpos = 0.0;
double MouseActions::ypos = 0.0;
MouseMode MouseActions::mouseMode = MouseMode::ScreenNavigation;
WorldCoord MouseActions::longlatMouse = { 0,0 };
WorldCoord MouseActions::dragStartLatLong = { 0,0 };
XY MouseActions::dragStartXY = { 0,0 };


void Input::HandleKey(GLFWwindow* window, int key, int scancode, int action, int mods, MainViewport* vp)
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureKeyboard) {
		return ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
	}
	
	if (action == GLFW_RELEASE) {	//we'll ignore any releases of keys
		return;
	}

	float step = 0.05f;

	int leftpressed, rightpressed, uppressed, downpressed;

	leftpressed = glfwGetKey(window, GLFW_KEY_LEFT);
	rightpressed = glfwGetKey(window, GLFW_KEY_RIGHT);
	uppressed = glfwGetKey(window, GLFW_KEY_UP);
	downpressed = glfwGetKey(window, GLFW_KEY_DOWN);

	MovingTarget* viewNSWE;
	viewNSWE = &vp->viewNSWE;

	if (GLFW_PRESS == leftpressed) {
		viewNSWE->target.nudgehorizontal(-step);
	}

	if (GLFW_PRESS == rightpressed) {
		viewNSWE->target.nudgehorizontal(step);
	}

	if (GLFW_PRESS == uppressed) {
		viewNSWE->target.nudgevertical(step);
	}
	if (GLFW_PRESS == downpressed) {
		viewNSWE->target.nudgevertical(-step);
	}

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		//glfwSetWindowShouldClose(window, GLFW_TRUE);
	}

	if (key == GLFW_KEY_KP_ADD) {
		viewNSWE->target.zoom(0.9f, viewNSWE->target.centre());
	}
	if (key == GLFW_KEY_KP_SUBTRACT) {
		viewNSWE->target.zoom(1.0f / 0.9f, viewNSWE->target.centre());
	}

	viewNSWE->starttime = glfwGetTime();
	viewNSWE->targettime = glfwGetTime() + 0.4;
}


void Input::HandleScroll(GLFWwindow* window, double xoffset, double yoffset, MainViewport *vp)
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureMouse) {
		return ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
	}
	
	double xpos, ypos;	//this function uses doubles
	glfwGetCursorPos(window, &xpos, &ypos);


	WorldCoord mapCoord;
	MovingTarget* viewNSWE;
	viewNSWE = &vp->viewNSWE;

	mapCoord.SetFromWindowXY((float)xpos, (float)ypos, viewNSWE->target, vp->windowDimensions);

	if (yoffset > 0.0) { viewNSWE->target.zoom(0.8f, mapCoord); }
	else if (yoffset < 1.0) { viewNSWE->target.zoom(1.0f / 0.8f, mapCoord); }

	viewNSWE->target.makeratio((double)vp->windowDimensions.height/ (double)vp->windowDimensions.width);

	viewNSWE->starttime = glfwGetTime();
	viewNSWE->targettime = glfwGetTime() + 0.4;

	return;
}



void Input::ManageMouseMoveClickAndDrag(GLFWwindow* window, LocationHistory *lh, MainViewport *vp)
{
	MovingTarget* viewNSWE;
	viewNSWE = &vp->viewNSWE;
	glfwGetCursorPos(window, &MouseActions::xpos, &MouseActions::ypos);
	
	MouseActions::longlatMouse.SetFromWindowXY(MouseActions::xpos, MouseActions::ypos, *viewNSWE, vp->windowDimensions);
	MouseActions::lmbState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	MouseActions::rmbState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);

	switch (MouseActions::mouseMode) {
	case MouseMode::ScreenNavigation:
		Input::MouseNavigation(vp);
		break;
	case MouseMode::RegionSelect:
		Input::RegionSelect(vp, lh);
		break;
	case MouseMode::PointSelect:
		//PointSelect;
		break;
	default:
		printf("other");
	}
	
	
	return;
}

void Input::RegionSelect(MainViewport* vp, LocationHistory* lh)
{
	if (MouseActions::rmbState == GLFW_PRESS) {
		//if right button is down, ugly hack to allow nav
		MouseActions::lmbState = GLFW_PRESS;
		Input::MouseNavigation(vp);
		return;
	}

	if (MouseActions::lmbState == GLFW_PRESS) {
		if (!MouseActions::IsDragging()) {	//if we're not dragging yet		
			MouseActions::SetStartOfDrag();
			MouseActions::dragStartLatLong.SetFromWindowXY(MouseActions::xpos, MouseActions::ypos, vp->viewNSWE, vp->windowDimensions);

			vp->regions.push_back(new Region());
		}
		//else {
			
			vp->regions.back()->SetNSWE(MouseActions::longlatMouse.latitude,  MouseActions::dragStartLatLong.latitude, MouseActions::longlatMouse.longitude, MouseActions::dragStartLatLong.longitude);
			
			vp->regions.back()->Populate(lh);
		//}
	}
	if (MouseActions::lmbState == GLFW_RELEASE) {
		if (MouseActions::IsDragging()) {
			
			MouseActions::FinishDragging();
		}
	}
}

void Input::MouseNavigation(MainViewport *vp)
{
	if (MouseActions::lmbState == GLFW_PRESS) {
		if (!MouseActions::IsDragging()) {	//if we're not dragging yet		
			MouseActions::SetStartOfDrag();

			originalNSWE = vp->viewNSWE;
			vp->viewNSWE.setMoving(true);

		}
		else {	//if we're dragging
			XY delta;

			//printf("d %f, mdds %f. ", delta.x, mouseDrag.dragStartXY.x);

			delta = MouseActions::GetDragDelta();

			float dppx, dppy;
			dppx = vp->viewNSWE.width() / vp->windowDimensions.width;
			dppy = vp->viewNSWE.height() / vp->windowDimensions.height;

			vp->viewNSWE.target.setvalues(originalNSWE.north + dppy * delta.y, originalNSWE.south + dppy * delta.y, originalNSWE.west - dppx * delta.x, originalNSWE.east - dppx * delta.x);
			vp->viewNSWE.setMoving(true);

			//printf("degrees: %f %f\n", dppx * deltax, dppy * deltay);

			vp->viewNSWE.starttime = glfwGetTime();

		}
	}
	if (MouseActions::lmbState == GLFW_RELEASE) {
		if (MouseActions::IsDragging()) {
			//printf("up");
			MouseActions::FinishDragging();
			vp->viewNSWE.setMoving(false);
		}
	}
}


void MouseActions::SetMouseMode(MouseMode m)
{
	mouseMode = m;
}

void MouseActions::SetStartOfDrag()
{
	dragStartXY.x = (float)xpos;
	dragStartXY.y = (float)ypos;
	isDragging = 1;
}

void MouseActions::SetStartOfDrag(float x, float y)
{
	dragStartXY.x = x;
	dragStartXY.y = y;
	isDragging = 1;
	return;
}

void MouseActions::FinishDragging()
{
	isDragging = 0;
}

bool MouseActions::IsDragging()
{
	return isDragging;
}

XY MouseActions::GetDragDelta()
{
	XY delta;
	delta.x =xpos - dragStartXY.x;
	delta.y = ypos - dragStartXY.y;
	
	return delta;
}
