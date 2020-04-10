#pragma once
#include <vector>
#include <string>

//pre
class NSWE;

class HighResManager {
public:
	int bestImage;
	int dataReady; //which file the data belongs to
	int subImageLoaded;	//which file is loaded into the subimage with glTexSubImage2D

	unsigned char* rawImageData;
	int width, height, nrChannels;

	unsigned int highresTexture;	//this is what the texture is stored in, we need it to bind things later.

	std::vector <std::string> filename;
	std::vector <NSWE> nswe;

	HighResManager();

	void DecideBestTex(RECTDIMENSION windowSize, NSWE* nswe);
	NSWE *GetBestNSWE();
	void ImageLoadThread(int n);
	void LoadBestTex();
};