#pragma once
#include <vector>
#include <string>
#include "nswe.h"

//
class MainViewport;
class vec2f;
class vec4f;

struct AtlasRect {
	int x, y;
	int width, height;
};

enum class ImageState {
	error,
	fileNotLoaded,
	fileLoading,
	dataLoaded,	//image data loaded from file
	hasAtlasPosNeedsSubImageLoading,
	textureLoaded
};

class HighResImage {
private:
	std::string filename;
	NSWE nswe;

	AtlasRect position;

	unsigned char* rawImageData;
	int nrChannels;

	ImageState state;

	//bool needsLoading;
	//bool dataLoaded;
	//bool inAtlas;
	//bool textureLoaded;

	int subImageLinesLoaded;
	friend class Atlas;
public:
	HighResImage(std::string filename, NSWE nswe) : filename(filename), nswe(nswe),
		position{ 0,0,0,0 },
		rawImageData(nullptr), nrChannels(0), state(ImageState::fileNotLoaded), subImageLinesLoaded(0) {}
	
	void SetAtlasPosition(AtlasRect position);
	void LoadFileData();
	void LoadTexture(GLuint texture);

	bool OverlapsWith(NSWE nswe);
	bool NeedsLoadingFromFile();
	bool NeedsAtlasPosition();
	bool NeedsSubImageLoading();
	bool FullyLoaded();
	
	const double PixelsPerDegree() const;
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
	//void LoadVisibleHighResImages(const NSWE& viewNSWE);
	
	int textureWidth;
	int textureHeight;

	GLuint texture;
public:

	void OutputDrawOrderedUVListForUniform(MainViewport* vp, int* numberOfItems, vec4f* arrayNSWE, vec2f* arrayMult, vec2f* arrayAdd, const int maxItems);
	void Setup(int width = 16384, int height = 4096);
	GLuint getTexture() const;

};
