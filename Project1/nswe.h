#pragma once
//#include "header.h"

//forward declaration
class WorldCoord;
class MovingTarget;
#include <algorithm>
#include <ostream>

class NSWE {
public:
	NSWE();
	NSWE(double n, double s, double w, double e);
	NSWE(int n, int s, int w, int e);

	void operator=(const NSWE sourceNSWE);
	void operator=(const MovingTarget sourceMT);

	bool operator==(const NSWE& other) const;
	bool operator!=(const NSWE& other) const;


	double north;
	double south;
	double west;
	double east;

	void constrainValues();
	void setvalues(double n, double s, double w, double e);
	void setvalues(const NSWE &setToThis);
	double width() const;
	double height() const;
	double area() const;
	void nudgehorizontal(double p);
	void nudgevertical(double p);
	void moveby(double x, double y);


	NSWE createExpandedBy(const double factor) const;
	NSWE intersectionWith(const NSWE otherNSWE) const;
	WorldCoord centre() const;

	void zoom(double z, const WorldCoord c);

	void makeratio(double ratio);

	bool containsPoint(const double latitude, const double longitude) const;
	bool overlapsWith(const NSWE &nswe) const;

	friend std::ostream& operator<<(std::ostream& os, NSWE const& m) {
		return os << "N: " << m.north << ", S: " << m.south << ", W: " << m.west << ", E: " << m.east;
	}

};



class MovingTarget : public NSWE {

public:
	NSWE target;

	void moveTowards(double currenttime);	//true if we're still moving, false if we're done
	void setTarget(const NSWE &nsweTarget, double stime, double ttime);

	void makeDirty();
	void setMoving(bool torf);
	bool isDirty() const;
	bool isMoving() const;
	
	void setTimer(double t);
	void setTarget(const NSWE &nsweTargetLocation);
	void setViewAtTarget();


private:
	double oldnorth, oldsouth, oldwest, oldeast;

	double targettime;
	double starttime;


	bool dirty;
	bool moving;
};
