#pragma once
#include <glad/glad.h> 
#include <GLFW/glfw3.h>

struct XY;
class WorldCoord;
class LocationHistory;
class MovingTarget;
class MainViewport;

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

namespace Input {
	
	void ManageMouseMoveClickAndDrag(GLFWwindow* window, LocationHistory* lh, MainViewport *vp);
	void MouseNavigation(MainViewport* vp);
	void RegionSelect(MainViewport* vp, LocationHistory* lh);
	void HandleScroll(GLFWwindow* window, double xoffset, double yoffset, MainViewport* vp);
	void HandleKey(GLFWwindow* window, int key, int scancode, int action, int mods, MainViewport* vp);
}