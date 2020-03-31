#pragma once
//#include "header.h"

//forward declaration
class WORLDCOORD;
class movingTarget;

class NSWE {
public:
	NSWE();
	NSWE(float n, float s, float w, float e);

	void operator=(const NSWE sourceNSWE);
	void operator=(const movingTarget sourceMT);

	float north;
	float south;
	float west;
	float east;

	void constrainvalues();
	void setvalues(float n, float s, float w, float e);
	void setto(NSWE *setthis);
	float width();
	float height();
	void nudgehorizontal(float p);
	void nudgevertical(float p);
	void moveby(float x, float y);


	NSWE createExpandedBy(float factor);
	WORLDCOORD centre();

	void zoom(float z, WORLDCOORD c);

	void makeratio(float ratio);

	bool containspoint(float latitude, float longitude);

};

class movingTarget : public NSWE {

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
