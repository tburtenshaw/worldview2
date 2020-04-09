#define GLEW_STATIC
#include <glew.h>
#include <GLFW/glfw3.h>
#include "header.h"
#include "input.h"
#include "nswe.h"
#include "heatmap.h"
#include "regions.h"
#include <stdio.h>

//extern MovingTarget viewNSWE;
//extern RECTDIMENSION windowDimensions;
//extern WORLDCOORD longlatMouse;
//extern BackgroundInfo bgInfo;
extern LocationHistory* pLocationHistory;



NSWE originalNSWE;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_RELEASE) {	//we'll ignore any releases of keys
		return;
	}

	float step = 0.05;

	int leftpressed, rightpressed, uppressed, downpressed;

	leftpressed = glfwGetKey(window, GLFW_KEY_LEFT);
	rightpressed = glfwGetKey(window, GLFW_KEY_RIGHT);
	uppressed = glfwGetKey(window, GLFW_KEY_UP);
	downpressed = glfwGetKey(window, GLFW_KEY_DOWN);

	MovingTarget* viewNSWE;
	viewNSWE = pLocationHistory->viewNSWE;

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
		viewNSWE->target.zoom(0.9, viewNSWE->target.centre());
	}
	if (key == GLFW_KEY_KP_SUBTRACT) {
		viewNSWE->target.zoom(1 / 0.9, viewNSWE->target.centre());
	}

	viewNSWE->starttime = glfwGetTime();
	viewNSWE->targettime = glfwGetTime() + 0.4;
}


void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);


	WORLDCOORD mapCoord;
	MovingTarget* viewNSWE;
	viewNSWE = pLocationHistory->viewNSWE;

	mapCoord.SetFromWindowXY(xpos, ypos, viewNSWE->target, pLocationHistory->windowDimensions);

	if (yoffset > 0) { viewNSWE->target.zoom(0.8, mapCoord); }
	else if (yoffset < 1) { viewNSWE->target.zoom(1 / 0.8, mapCoord); }

	viewNSWE->target.makeratio(1);
	viewNSWE->starttime = glfwGetTime();
	viewNSWE->targettime = glfwGetTime() + 0.4;

	return;
}

void ManageMouseMoveClickAndDrag(GLFWwindow* window, LocationHistory *lh)
{
	MovingTarget* viewNSWE;
	MouseActions* mouse;
	viewNSWE = lh->viewNSWE;
	mouse = lh->mouseInfo;

	glfwGetCursorPos(window, &mouse->xpos, &mouse->ypos);

	mouse->longlatMouse.SetFromWindowXY(mouse->xpos, mouse->ypos, *viewNSWE, lh->windowDimensions);
	mouse->lmbState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	
	switch (mouse->mouseMode) {
	case MouseMode::ScreenNavigation:
		MouseNavigation(mouse, viewNSWE, lh);
		break;
	case MouseMode::RegionSelect:
		RegionSelect(mouse, viewNSWE, lh);
		break;
	case MouseMode::PointSelect:
		//PointSelect;
		break;
	default:
		printf("other");
	}
	
	
	return;
}

void RegionSelect(MouseActions* mouse, MovingTarget* viewNSWE, LocationHistory* lh)
{

	if (mouse->lmbState == GLFW_PRESS) {
		if (!mouse->IsDragging()) {	//if we're not dragging yet		
			mouse->SetStartOfDrag();
			mouse->dragStartLatLong.SetFromWindowXY(mouse->xpos, mouse->ypos, *viewNSWE, lh->windowDimensions);

			lh->regions.push_back(new Region());
		}
		else {
			
			lh->regions.back()->SetNSWE(mouse->longlatMouse.latitude,  mouse->dragStartLatLong.latitude, mouse->longlatMouse.longitude, mouse->dragStartLatLong.longitude);
			
			lh->regions.back()->Populate(lh);
		}
	}
	if (mouse->lmbState == GLFW_RELEASE) {
		if (mouse->IsDragging()) {
			
			mouse->FinishDragging();
		}
	}
}

void MouseNavigation(MouseActions* mouse, MovingTarget* viewNSWE, LocationHistory*lh)
{
	if (mouse->lmbState == GLFW_PRESS) {
		if (!mouse->IsDragging()) {	//if we're not dragging yet		
			mouse->SetStartOfDrag();

			originalNSWE = *viewNSWE;
			viewNSWE->setMoving(true);

		}
		else {	//if we're dragging
			XY delta;

			//printf("d %f, mdds %f. ", delta.x, mouseDrag.dragStartXY.x);

			delta = mouse->GetDragDelta();

			float dppx, dppy;
			dppx = viewNSWE->width() / lh->windowDimensions->width;
			dppy = viewNSWE->height() / lh->windowDimensions->height;

			viewNSWE->target.setvalues(originalNSWE.north + dppy * delta.y, originalNSWE.south + dppy * delta.y, originalNSWE.west - dppx * delta.x, originalNSWE.east - dppx * delta.x);
			viewNSWE->setMoving(true);

			//printf("degrees: %f %f\n", dppx * deltax, dppy * deltay);

			viewNSWE->starttime = glfwGetTime();

		}
	}
	if (mouse->lmbState == GLFW_RELEASE) {
		if (mouse->IsDragging()) {
			//printf("up");
			mouse->FinishDragging();
			viewNSWE->setMoving(false);
		}
	}
}


MouseActions::MouseActions()
{
	xpos = 0.0;
	ypos = 0.0;
	lmbState = 0;
	mouseMode = MouseMode::ScreenNavigation;
	isDragging = 0;
	dragStartXY.x = 0;
	dragStartXY.y = 0;
	longlatMouse.latitude = 0;
	longlatMouse.longitude = 0;
}


void MouseActions::SetStartOfDrag()
{
	dragStartXY.x = xpos;
	dragStartXY.y = ypos;
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
