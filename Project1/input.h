#pragma once
#include <glad/glad.h> 
#include <GLFW/glfw3.h>

struct XY;
class WorldCoord;
class LocationHistory;
class MovingTarget;

enum class MouseMode	//to be fair, not really the mouse mode, more the whole system
{
	ScreenNavigation,
	PointSelect,
	RegionSelect
};

class MouseActions {
private:

	
public:
	static double xpos; //as these are doubles from the glfw function
	static double ypos;
	static int lmbState;
	static int rmbState;

	static bool isDragging;
	static XY dragStartXY;


	static WorldCoord longlatMouse;
	static WorldCoord dragStartLatLong;
	static MouseMode mouseMode;
	
	static void SetMouseMode(MouseMode m);
	static void SetStartOfDrag();
	static void SetStartOfDrag(float x, float y);

	static void FinishDragging();
	static bool IsDragging();
	static XY GetDragDelta();

};

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void size_callback(GLFWwindow* window, int windowNewWidth, int windowNewHeight);
void ManageMouseMoveClickAndDrag(GLFWwindow* window, LocationHistory *lh);
void MouseNavigation(MovingTarget* viewNSWE, LocationHistory* lh);
void RegionSelect(MovingTarget* viewNSWE, LocationHistory* lh);