#pragma once
#include <vector>
#include <string>
#include "nswe.h"

class HighResManager {
public:
	int bestImage;
	int dataReady; //which file the data belongs to
	int subImageLoaded;	//which file is loaded into the subimage with glTexSubImage2D
	bool fileThreadLoading;
	bool subImageLoading;
	int subImageLinesLoaded;	//the height so far uploaded

	unsigned char* rawImageData;
	int width, height, nrChannels;

	//unsigned int highresTexture;	//this is what the texture is stored in, we need it to bind things later.

	std::vector <std::string> filename;
	std::vector <NSWE> nswe;

	HighResManager();

	void DecideBestTex(RectDimension windowSize, const NSWE & viewportNSWE);
	NSWE *GetBestNSWE(unsigned int gltexture);
	void ImageLoadThread(int n);
	void LoadBestTex(unsigned int gltexture);
};