#pragma once
#include "header.h"

class Spectrum {

	//friend class Spectrum_Handler;
private:
	struct ColourPoint {
		RGBA colour;
		float distance;

		ColourPoint(const RGBA& c, float d) : colour(c), distance(d) {}
	};


	std::string name;
	std::vector <ColourPoint> colourPoints;
public:
	Spectrum(std::string name_, std::vector<ColourPoint> colourPoints_)
		: name(name_), colourPoints(colourPoints_) {}

	RGBA GetColourAtDistance(float dist) const;

};

class Spectrum_Handler {
private:
	static inline std::vector <Spectrum> spectrums
	{
		{"Test",
		{
			{{0,0,0,255},{0.0}},
			{{0,0,250,255},{0.2}},
			{ {120,80,255,255},{0.4}},
			{ {255,0,0,255},{0.6}},
			{ {255,255,0,255},{0.8}},
			{ {255,255,255,255},{1.0}}
		}
		}
	};
public:
	static RGBA GetPointFromSpectrum(int n, float dist);

};