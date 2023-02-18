#pragma once
#include <string>
#include <map>
#include "header.h"

class GuiAtlas;

enum class Icon {
	open,
	close,
	save,
	heatmap,
	points,
	lines
};

class AtlasEntry {
private:
	std::string name;
	RectDimension size;
	vec2f uv0;
	vec2f uv1;
public:
	AtlasEntry(GuiAtlas &a, const std::string n, RectDimension s);
};

class GuiAtlas {
	int width;
	int height;
	void FillAtlas();

	float currentUVx;
	float currentUVy;
	float currentUVheight;


public:
	GuiAtlas(int w, int h) :width(w), height(h), currentUVx(0.0f), currentUVy(0.0f), currentUVheight(0.0f) {};
	std::map<enum class Icon, AtlasEntry> entry;
};