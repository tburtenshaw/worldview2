#include "header.h"
#include "nswe.h"
#include "heatmap.h"

NSWE::NSWE()
{
	north = 90.0; south = -90.0; west = -180.0; east = 180.0;
}

NSWE::NSWE(double n, double s, double w, double e)
{
	setvalues(n, s, w, e);
}

NSWE::NSWE(int n, int s, int w, int e)
{
	setvalues((double)n, (double)s, (double)w, (double)e);
}


void NSWE::operator=(const NSWE sourceNSWE)
{
	north = sourceNSWE.north;
	south = sourceNSWE.south;
	west = sourceNSWE.west;
	east = sourceNSWE.east;
}

void NSWE::operator=(const MovingTarget sourceMT)
{
	north = sourceMT.north;
	south = sourceMT.south;
	west = sourceMT.west;
	east = sourceMT.east;
}

bool NSWE::operator==(const NSWE& other) const
{
	return (north == other.north) && (south == other.south) &&
		(west == other.west) && (east == other.east);
}

bool NSWE::operator!=(const NSWE& other) const
{
	return !(*this == other);
}



void NSWE::constrainValues() {
	if (west > 180) { west -= 360; east -= 360; }
	if (east < -180) { east += 360; west += 360; }

}

void NSWE::setvalues(double n, double s, double w, double e) {
	north = n; south = s; west = w; east = e;
	constrainValues();
}

void NSWE::setvalues(const NSWE &setToThis) {
	north = setToThis.north; south = setToThis.south; west = setToThis.west; east = setToThis.east;
	constrainValues();
}


double NSWE::width() const
	{
		return (east - west);
	}

double NSWE::height() const
	{
		return (north - south);
	}

double NSWE::area() const
	{
		return (north - south) * (east - west);
	}

	void NSWE::nudgehorizontal(double p) {	//the amount moved as a ratio (i.e. 0.1 = 10%)
		double w;
		w = east - west;
		east += w * p;
		west += w * p;

		constrainValues();
	}

	void NSWE::nudgevertical(double p) {	//the amount moved as a ratio (i.e. 0.1 = 10%)
		double h;
		h = north - south;
		north += h * p;
		south += h * p;
		return;
	}

	void NSWE::moveby(double x, double y)
	{
		west += x;
		east += x;
		north += y;
		south += y;
		return;
	}

	NSWE NSWE::createExpandedBy(const double factor) const
	{
		//this makes the height and width both "factor" larger, centred at the middle, to a max of -180,90->180,-90.
		NSWE outputNSWE;
		
		double h = north - south;
		double w = east - west;

		double midlat = (north + south) / 2;
		double midlong = (east + west) / 2;

		outputNSWE.north = midlat + (h / 2) * factor;
		outputNSWE.south = midlat - (h / 2) * factor;
		outputNSWE.west = midlong - (w / 2) * factor;
		outputNSWE.east = midlong + (w / 2) * factor;

		//printf("Expand by: %f Start: H:%f W:%f. %.7f, %.7f, %.7f, %.7f. Then %.7f, %.7f, %.7f, %.7f, .\n",factor, h,w,north,south,west,east,outputNSWE.north, outputNSWE.south, outputNSWE.west, outputNSWE.east);

		return outputNSWE;
	}

	NSWE NSWE::intersectionWith(const NSWE otherNSWE) const
	{
		NSWE outputNSWE;

		outputNSWE.north = std::min(north, otherNSWE.north);
		outputNSWE.south = std::max(south, otherNSWE.south);
		outputNSWE.west= std::max(west, otherNSWE.west);
		outputNSWE.east = std::min(east, otherNSWE.east);

		if ((outputNSWE.south > outputNSWE.north) || (outputNSWE.west > outputNSWE.east)) {
			outputNSWE.north = outputNSWE.south = outputNSWE.west = outputNSWE.east = 0;
		}


		return outputNSWE;
	}

	WorldCoord NSWE::centre() const {
		WorldCoord c;

		c.longitude = (west + east) / 2.0;
		c.latitude = (north + south) / 2.0;

		return c;
	}



	void NSWE::zoom(double z, const WorldCoord c) {

		//printf("Zoom into %f %f, by %f.\n", c.latitude, c.longitude, z);
		//printf("Before: N%f,S%f,W%f,E%f\n", north, south, west, east);
		west -= c.longitude;
		east -= c.longitude;
		north -= c.latitude;
		south -= c.latitude;

		west *= z;
		east *= z;
		north *= z;
		south *= z;

		//if the zoom is so the viewpoint is less than this many degrees.
		if (height() < 0.01) {
			z = 0.01/height();
			west *= z;
			east *= z;
			north *= z;
			south *= z;
		}
		else if (height() >1000) { //same thing for zooming out too much
			z = 1000 / height();
			west *= z;
			east *= z;
			north *= z;
			south *= z;
		}

		west += c.longitude;
		east += c.longitude;
		north += c.latitude;
		south += c.latitude;

		//printf("After: N%f,S%f,W%f,E%f\n", north, south, west, east);
		//printf("This leaves us at %f %f.\n", c.latitude, c.longitude);
			
		return;

	}

	void NSWE::makeratio(double ratio) {
		double width, height;
		width = east - west;
		height = north - south;

		double targetheight = width * ratio;

		north += (targetheight - height) / 2.0;
		south -= (targetheight - height) / 2.0;
		
	}

	bool NSWE::containsPoint(double latitude, double longitude) const
	{
		if (latitude>north)
			return false;
		if (latitude <south)
			return false;
		if (longitude < west)
			return false;
		if (longitude > east)
			return false;

		return true;
	}

	bool NSWE::overlapsWith(const NSWE &nswe) const
	{
		//if the west is further than the east in either case, they can't overlap
		if ((west > nswe.east) || (nswe.west > east))
			return false;
		if ((south > nswe.north) || (nswe.south > north))
			return false;

		//this enables zero area to still overlap, okay for this
		return true;
	}



	void MovingTarget::moveTowards(double currenttime) {

		if (currenttime >= targettime) {
			north = target.north;
			south = target.south;
			west = target.west;
			east = target.east;

			moving = false;
			
			if (!dirty) {
				if ((oldnorth != north) || (oldsouth != south) || (oldwest != west) || (oldeast != east)) {
					dirty = true;
					oldnorth = north; oldsouth = south; oldwest = west; oldeast = east;
				}
			}
			else
			{
				dirty = false;
			}

			return;
		}

		double duration = targettime - starttime;
		double tdiff = targettime - currenttime;

		double normalisedtime = 1 - (tdiff / duration);

		int sidesright = 0;

		//check if close enough (within 0.1%)
		if (std::abs(north - target.north) / target.height() < 0.001) {
			north = target.north;
			sidesright++;
		}
		else {
			north = north * (1 - normalisedtime) + (target.north * normalisedtime);
		}

		if (std::abs(south - target.south) / target.height() < 0.001) {
			south = target.south;
			sidesright++;
		}
		else {
			south = south * (1 - normalisedtime) + (target.south * normalisedtime);
		}

		if (std::abs(west - target.west) / target.width() < 0.001) {
			west = target.west;
			sidesright++;
		}
		else {
			west = west * (1 - normalisedtime) + (target.west * normalisedtime);
			//printf("w: %.9f %.9f", east, abs(west - target.west));
		}

		if (std::abs(east - target.east) / target.width() < 0.001) {
			east = target.east;
			sidesright++;
		}
		else {
			east = east * (1 - normalisedtime) + (target.east * normalisedtime);
			//printf("e: %.9f %.9f", east, abs(east - target.east));
		}
		
		if (sidesright==4) {
			north = target.north;
			south = target.south;
			west = target.west;
			east = target.east;
			targettime = 0.0;
			moving = false;
			dirty = true;
			return;
		}
			
		//printf("moving %.6f %.6f %.6f %.6f %i\n", north, south, west, east,sidesright);
		moving = true;
		return;
	}

	void MovingTarget::setTarget(const NSWE &nsweTarget, double stime, double ttime) {
		target.north = nsweTarget.north;
		target.south = nsweTarget.south;
		target.west = nsweTarget.west;
		target.east = nsweTarget.east;
		starttime = stime;
		targettime = ttime;
		
		dirty = true;
	}

	void MovingTarget::makeDirty()
	{
		dirty = true;
	}

	void MovingTarget::setMoving(bool torf)
	{
		moving = torf;
	}

	bool MovingTarget::isDirty() const
	{
		return dirty;
	}

	bool MovingTarget::isMoving() const
	{
		return moving;
	}

	void MovingTarget::setTimer(double t)
	{
		starttime = glfwGetTime();
		targettime = starttime + t;
	}

	void MovingTarget::setTarget(const NSWE &nsweTargetLocation)
	{
		target = nsweTargetLocation;
		target.constrainValues();
	}

	void MovingTarget::setViewAtTarget()
	{
		this->setvalues(target);
	}
