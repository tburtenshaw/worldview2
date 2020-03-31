#define GLEW_STATIC
#include <glew.h>
#include <GLFW/glfw3.h>
#include "header.h"
#include "input.h"
#include "nswe.h"
#include "heatmap.h"
#include <stdio.h>

extern NSWE targetNSWE;
extern movingTarget viewNSWE;
extern RECTDIMENSION windowDimensions;
extern WORLDCOORD longlatMouse;
extern BackgroundInfo bgInfo;

MouseActions mouseDrag;

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


	if (GLFW_PRESS == leftpressed) {
		viewNSWE.target.nudgehorizontal(-step);
	}

	if (GLFW_PRESS == rightpressed) {
		viewNSWE.target.nudgehorizontal(step);
	}

	if (GLFW_PRESS == uppressed) {
		viewNSWE.target.nudgevertical(step);
	}
	if (GLFW_PRESS == downpressed) {
		viewNSWE.target.nudgevertical(-step);
	}

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		UpdateHeatmapTexture(&viewNSWE.target, &bgInfo);
		//glfwSetWindowShouldClose(window, GLFW_TRUE);
	}

	if (key == GLFW_KEY_KP_ADD) {
		viewNSWE.target.zoom(0.9, viewNSWE.target.centre());
	}
	if (key == GLFW_KEY_KP_SUBTRACT) {
		viewNSWE.target.zoom(1 / 0.9, viewNSWE.target.centre());
	}

	viewNSWE.starttime = glfwGetTime();
	viewNSWE.targettime = glfwGetTime() + 0.4;
}


void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);


	WORLDCOORD mapCoord;

	mapCoord.SetFromWindowXY(xpos, ypos, viewNSWE.target, windowDimensions);

	if (yoffset > 0) { viewNSWE.target.zoom(0.8, mapCoord); }
	else if (yoffset < 1) { viewNSWE.target.zoom(1 / 0.8, mapCoord); }

	viewNSWE.target.makeratio(1);
	viewNSWE.starttime = glfwGetTime();
	viewNSWE.targettime = glfwGetTime() + 0.4;

	return;
}


void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	longlatMouse.SetFromWindowXY(xpos, ypos, viewNSWE, windowDimensions);

	int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	if (state == GLFW_PRESS) {
		if (mouseDrag.isDragging == 0) {	//if we're not dragging yet
			mouseDrag.isDragging = 1;
			mouseDrag.SetStart(xpos,ypos);
	
			originalNSWE = viewNSWE;
			viewNSWE.setMoving(true);

		}
		else {	//if we're dragging
			XY delta;

			//printf("d %f, mdds %f. ", delta.x, mouseDrag.dragStartXY.x);

			delta.x = xpos - mouseDrag.dragStartXY.x;
			delta.y = (ypos - mouseDrag.dragStartXY.y);

			float dppx, dppy;
			dppx = viewNSWE.width() / windowDimensions.width;
			dppy = viewNSWE.height() / windowDimensions.height;

			viewNSWE.target.setvalues(originalNSWE.north + dppy * delta.y, originalNSWE.south + dppy * delta.y, originalNSWE.west - dppx * delta.x, originalNSWE.east - dppx * delta.x);
			viewNSWE.setMoving(true);

			//printf("degrees: %f %f\n", dppx * deltax, dppy * deltay);

			viewNSWE.starttime = glfwGetTime();

		}
	}
	if (state == GLFW_RELEASE) {
		if (mouseDrag.isDragging) {
			//printf("up");
			mouseDrag.isDragging = 0;
			viewNSWE.setMoving(false);
		}
	}
	return;
}

void MouseActions::SetStart(float x, float y)
{
	dragStartXY.x = x;
	dragStartXY.y = y;
	return;
}
