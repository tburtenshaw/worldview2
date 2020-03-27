#pragma once
#include <glew.h>
#include <GLFW/glfw3.h>
//#include "header.h"
struct XY;

class MouseActions {
public:
	XY dragStartXY;
	int isDragging;
	void SetStart(float x, float y);
};

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);