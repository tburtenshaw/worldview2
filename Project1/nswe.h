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

	double north;
	double south;
	double west;
	double east;

	void constrainvalues();
	void setvalues(double n, double s, double w, double e);
	void setvalues(NSWE setToThis);
	void setto(NSWE *setthis);
	double width() const;
	double height() const;
	double area() const;
	void nudgehorizontal(double p);
	void nudgevertical(double p);
	void moveby(double x, double y);


	NSWE createExpandedBy(const double factor) const;
	NSWE intersectionWith(NSWE otherNSWE) const;
	WorldCoord centre() const;

	void zoom(double z, WorldCoord c);

	void makeratio(double ratio);

	bool containspoint(double latitude, double longitude) const;

	friend std::ostream& operator<<(std::ostream& os, NSWE const& m) {
		return os << "N: " << m.north << ", S: " << m.south << ", W: " << m.west << ", E: " << m.east;
	}

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
	double oldnorth, oldsouth, oldwest, oldeast;

	bool dirty;
	bool moving;
};
