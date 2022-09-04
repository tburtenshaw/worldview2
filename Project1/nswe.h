#pragma once
//#include "header.h"

//forward declaration
class WorldCoord;
class MovingTarget;
#include <algorithm>

class NSWE {
public:
	NSWE();
	NSWE(float n, float s, float w, float e);

	void operator=(const NSWE sourceNSWE);
	void operator=(const MovingTarget sourceMT);

	float north;
	float south;
	float west;
	float east;

	void constrainvalues();
	void setvalues(float n, float s, float w, float e);
	void setvalues(NSWE setToThis);
	void setto(NSWE *setthis);
	float width() const;
	float height() const;
	float area() const;
	void nudgehorizontal(float p);
	void nudgevertical(float p);
	void moveby(float x, float y);


	NSWE createExpandedBy(float factor) const;
	NSWE intersectionWith(NSWE otherNSWE) const;
	WorldCoord centre() const;

	void zoom(float z, WorldCoord c);

	void makeratio(float ratio);

	bool containspoint(float latitude, float longitude) const;

};

class MovingTarget : public NSWE {

public:
	NSWE target;

	double targettime;
	double starttime;

	


	void movetowards(double currenttime);	//true if we're still moving, false if we're done
	void settarget(NSWE nsweTarget, double stime, double ttime);

	void makeDirty();
	void setMoving(bool torf);
	bool isDirty();
	bool isMoving();


private:
	float abs(float f);

	float oldnorth, oldsouth, oldwest, oldeast;


	bool dirty;
	bool moving;
};
