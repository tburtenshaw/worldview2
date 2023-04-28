#pragma once
#include "header.h"

class Spectrum {
private:
	struct ColourPoint {
		RGBA colour;
		float distance;

		ColourPoint(const RGBA& c, float d) : colour(c), distance(d) {}
	};


	std::string name;
	std::vector <ColourPoint> colourPoints;

	void FillColourPointsFromGradient(const std::vector<RGBA>& gradient, int maxPoints);
	float ErrorFromGradient(const std::vector<RGBA>& gradient);

	void FineTuneColourValues(const std::vector<RGBA>& gradient);

public:
	Spectrum(std::string name_, std::vector<ColourPoint> colourPoints_)
		: name(name_), colourPoints(colourPoints_) {}

	RGBA GetColourAtDistance(float dist) const;
	std::string GetSpectrumName() const;
	int GetSpectrumSize() const;


	static Spectrum CreateOptimalEvenSpacedSpectrumFromGradient(std::string name, const std::vector<RGBA>& gradient,int maxPoints = 16, float relTarget = 0.1f, float absTarget = 0.002);
	
};

class SpectrumHandler {
private:
	static inline std::vector <Spectrum> spectrums
	{
		{"Test",
		{
			{{0,0,0,255},{0.0f}},
			{{0,0,250,255},{0.2f}},
			{ {120,80,255,255},{0.4f}},
			{ {255,0,0,255},{0.6f}},
			{ {255,255,0,255},{0.8f}},
			{ {255,255,255,255},{1.0f}}
		}
		},
		
		{"BlackRed",
		{
			{{0,0,0,255},{0.0f}},
			{{255,0,0,255},{1.0f}},
		}
		},

		{"GreenYellowBlue",
		{
			{{0,255,0,255},{0.0f}},
			{{255,255,0,255},{0.5f}},
			{{0,0,255,255},{1.0f}},
		}
		},


	};
public:
	static void AddSpectrum(Spectrum s);
	static RGBA GetPointFromSpectrum(size_t n, float dist);
	static size_t GetNumberOfSpectrums();
	static std::string GetSpectrumName(size_t n);
	static int GetSpectrumSize(size_t n);
	static std::vector<std::string> ListSpectrums();
};