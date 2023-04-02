#pragma once
#include <string>
#include <map>
#include "header.h"

enum class Icon {
	def,
	open,
	close,
	save,
	heatmap,
	points,
	lines,
	spectrum
};

class AtlasEntry {
private:
	std::string name;
	RectDimension size; //intended display
	vec2f uv0;
	vec2f uv1;
public:
	AtlasEntry(const std::string n, RectDimension s, vec2f uv0, vec2f uv1) :name(n), size(s), uv0(uv0), uv1(uv1) {};
	RectDimension GetSize() const;	
	float GetWidth() const;
	float GetHeight() const;
	vec2f GetUV0() const;
	vec2f GetUV1() const;
	std::string GetName() const;
};

class GuiAtlas {
private:
	int width;
	int height;
	void FillAtlas();
	void AddIconToAtlas(enum class Icon, std::string name, RectDimension size, void* data);
	UVpair GetNextUvAndAdvance(RectDimension size);
	void CreateSpectrumPreview();
	UVpair spectrumUV;


	float currentUVx;
	float currentUVy;
	float currentUVheight;
	GLuint textureId; // OpenGL texture id


	std::map<enum class Icon, AtlasEntry> entry;
public:
	GuiAtlas(int w, int h) :width(w), height(h), currentUVx(0.0f), currentUVy(0.0f), currentUVheight(0.0f), textureId(0),spectrumUV(){};
	void Initialise();
	GLuint GetTextureId() const;

	const AtlasEntry& GetEntry(Icon icon) const;
	UVpair GetSpectrumUV(int number);


};

extern GuiAtlas guiAtlas;