#include "header.h"
#include "nswe.h"
#include "heatmap.h"

NSWE::NSWE()
{
	north = 90; south = -90; west = -180; east = 180;
}

NSWE::NSWE(float n, float s, float w, float e) {
	setvalues(n, s, w, e);
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

void NSWE::constrainvalues() {
	if (west > 180) { west -= 360; east -= 360; }
	if (east < -180) { east += 360; west += 360; }

}

void NSWE::setvalues(float n, float s, float w, float e) {
	north = n; south = s; west = w; east = e;
	constrainvalues();
}

void NSWE::setvalues(NSWE setToThis) {
	north = setToThis.north; south = setToThis.south; west = setToThis.west; east = setToThis.east;
	constrainvalues();
}

void NSWE::setto(NSWE *setthis) {
	north = setthis->north;
	south = setthis->south;
	west = setthis->west;
	east = setthis->east;
}

	float NSWE::width() {
		float w = (east - west);
		if (w == 0) {//don't allow this, as there'll be division by zero and crashes
			west -= 10;
			east += 10;
			return 20;
		}
		return w;
	}

	float NSWE::height() {
		return (north - south);
	}

	float NSWE::area()
	{
		return (north - south) * (east - west);
	}

	void NSWE::nudgehorizontal(float p) {	//the amount moved as a ratio (i.e. 0.1 = 10%)
		float w;
		w = east - west;
		east += w * p;
		west += w * p;

		constrainvalues();
		/*
		if (west > 180.0) {//constrain the left part of view
			east = east - (west - 180);
			west = 180;
		}
		if (east < -180.0) {//constrain the left part of view
			west = west+(-180-east);
			east = -180;
		}
		*/

	}

	void NSWE::nudgevertical(float p) {	//the amount moved as a ratio (i.e. 0.1 = 10%)
		float h;
		h = north - south;
		north += h * p;
		south += h * p;
		return;
	}

	void NSWE::moveby(float x, float y)
	{
		west += x;
		east += x;
		north += y;
		south += y;
		return;
	}

	NSWE NSWE::createExpandedBy(float factor)
	{
		//this makes the height and width both "factor" larger, centred at the middle, to a max of -180,90->180,-90.
		NSWE outputNSWE;
		
		float h = north - south;
		float w = east - west;

		float midlat = (north + south) / 2;
		float midlong = (east + west) / 2;

		outputNSWE.north = midlat + (h / 2) * factor;
		outputNSWE.south = midlat - (h / 2) * factor;
		outputNSWE.west = midlong - (w / 2) * factor;
		outputNSWE.east = midlong + (w / 2) * factor;

		//printf("Expand by: %f Start: H:%f W:%f. %.7f, %.7f, %.7f, %.7f. Then %.7f, %.7f, %.7f, %.7f, .\n",factor, h,w,north,south,west,east,outputNSWE.north, outputNSWE.south, outputNSWE.west, outputNSWE.east);

		return outputNSWE;
	}

	NSWE NSWE::interectionWith(NSWE otherNSWE)
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

	WORLDCOORD NSWE::centre() {
		WORLDCOORD c;

		c.longitude = (west + east) / 2;
		c.latitude = (north + south) / 2;

		return c;
	}



	void NSWE::zoom(float z, WORLDCOORD c) {

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

	void NSWE::makeratio(float ratio) {
		float width, height;
		width = east - west;
		height = north - south;

		float targetheight = width * ratio;

		north += (targetheight - height) / 2;
		south -= (targetheight - height) / 2;
		
	}

	bool NSWE::containspoint(float latitude, float longitude)
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



	void MovingTarget::movetowards(double currenttime) {

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

		float normalisedtime = 1 - (tdiff / duration);

		int sidesright = 0;

		//check if close enough (within 0.1%)
		if (abs(north - target.north) / target.height() < 0.001) {
			north = target.north;
			sidesright++;
		}
		else {
			north = north * (1 - normalisedtime) + (target.north * normalisedtime);
		}

		if (abs(south - target.south) / target.height() < 0.001) {
			south = target.south;
			sidesright++;
		}
		else {
			south = south * (1 - normalisedtime) + (target.south * normalisedtime);
		}

		if (abs(west - target.west) / target.width() < 0.001) {
			west = target.west;
			sidesright++;
		}
		else {
			west = west * (1 - normalisedtime) + (target.west * normalisedtime);
			//printf("w: %.9f %.9f", east, abs(west - target.west));
		}

		if (abs(east - target.east) / target.width() < 0.001) {
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
			targettime = 0;
			moving = false;
			dirty = true;
			return;
		}
			
		//printf("moving %.6f %.6f %.6f %.6f %i\n", north, south, west, east,sidesright);
		moving = true;
		return;
	}

	void MovingTarget::settarget(NSWE nsweTarget, double stime, double ttime) {
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

	bool MovingTarget::isDirty()
	{
		if (dirty) return true;
		return false;
	}

	bool MovingTarget::isMoving()
	{
		if (moving)	return true;
		return false;
	}

	float MovingTarget::abs(float f)
	{
		if (f < 0) {
			return -f;
		}
		return f;
	}
