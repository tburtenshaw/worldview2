#pragma once
#include <glad/glad.h> 
#include <GLFW/glfw3.h>
//#include "header.h"
struct XY;
class WORLDCOORD;
class LocationHistory;

enum class MouseMode
{
	ScreenNavigation,
	PointSelect,
	RegionSelect
};

class MouseActions {
private:
	bool isDragging;
public:
	MouseActions();

	double xpos; //as these are doubles from the glfw function
	double ypos;
	int lmbState;

	MouseMode mouseMode;
	WORLDCOORD longlatMouse;
	
	XY dragStartXY;
	WORLDCOORD dragStartLatLong;
	
	
	void SetStartOfDrag();
	void SetStartOfDrag(float x, float y);

	void FinishDragging();
	bool IsDragging();
	XY GetDragDelta();

};

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void ManageMouseMoveClickAndDrag(GLFWwindow* window, LocationHistory *lh);
void MouseNavigation(MouseActions* mouse, MovingTarget* viewNSWE, LocationHistory* lh);
void RegionSelect(MouseActions* mouse, MovingTarget* viewNSWE, LocationHistory* lh);