#pragma once
#include <vector>
#include <string>
#include "nswe.h"

class MainViewport;

struct AtlasRect {
	int x, y;
	int width, height;
};

class HighResImage {
private:
	std::string filename;
	NSWE nswe;

	AtlasRect position;

	unsigned char* rawImageData;
	int nrChannels;

	bool needsLoading;
	bool dataLoaded;
	bool inAtlas;
	bool textureLoaded;

	int subImageLinesLoaded;
	friend class Atlas;
public:
	HighResImage(std::string filename, NSWE nswe) : filename(filename), nswe(nswe),
		inAtlas(false), position{ 0,0,0,0 },
		rawImageData(nullptr),needsLoading(true), dataLoaded(false), subImageLinesLoaded(0) {}
	
	void SetAtlasPosition(AtlasRect position);
	void LoadFileToAtlas();
	void LoadTexture(GLuint texture);

	bool OverlapsWith(NSWE nswe);
	bool NeedsLoadingFromFile();
};

class Partition {
private:	
	AtlasRect position;
	bool available;
	friend class Atlas;

public:
	Partition(int x, int y, int width, int height) : position({ x,y,width,height }), available(true) {}
	Partition(AtlasRect r) : position(r), available(true) {}
	Partition(AtlasRect r, bool available) : position(r), available(available) {}
};

class Atlas {
private:
	std::vector <HighResImage> images;
	std::vector <Partition> partitions;

	void CreateMainPartition(const int width, const int height);
	void PopulateHighResImages();	//loads (e.g. from a directory) all the high res aerial photos

	AtlasRect MakeSpaceFor(int width, int height);
	void LoadVisibleHighResImages(const NSWE& viewNSWE);
	GLuint texture;
public:

	void OutputDrawOrderedUVListForUniform(MainViewport* vp, int* numberOfItems, float* array, int maxItems); //should only include ones that need to be drawn
	void Setup(int width = 16384, int height = 4096);
	GLuint getTexture() const;

};
