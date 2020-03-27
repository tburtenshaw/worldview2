#pragma once
//#include "header.h"

//forward declaration
class WORLDCOORD;

class NSWE {
public:
	NSWE();
	NSWE(float n, float s, float w, float e);
	
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

	WORLDCOORD centre();

	void zoom(float z, WORLDCOORD c);

	void makeratio(float ratio);
};

class movingTarget : public NSWE {

public:
	NSWE target;

	double targettime;
	double starttime;

	void movetowards(double currenttime);
	void settarget(NSWE nsweTarget, double stime, double ttime);

private:
	float abs(float f);


};
