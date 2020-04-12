#include <vector>
#include <string>
#include <thread>

#include <chrono>

#include "header.h"
#include "highresmanager.h"
#include "nswe.h"

#include <STB/stb_image.h>

#define GLEW_STATIC
#include <glew.h>
#include <GLFW/glfw3.h>

HighResManager::HighResManager()
{
	bestImage = 0;
	dataReady = 0;
	subImageLoaded = 0;
	fileThreadLoading = false;

	rawImageData = NULL;
	height = 0;
	width = 0;
	nrChannels = 0;

	highresTexture = 0;

	filename.push_back("Background");
	nswe.push_back(NSWE(10000, 20000, 30000, 40000));	//should never best displayed


	filename.push_back("D:/n-34s-48w166e179.png");
	nswe.push_back(NSWE(-34.0f, -48.0f, 166.0f, 179.0f));

	filename.push_back("D:/n55s38w0e17.png");
	nswe.push_back(NSWE(55.0f, 38.0f, 0.0f, 17.0f));

	filename.push_back("D:/n-16s-28w145e154.png");
	nswe.push_back(NSWE(-16.0f, -28.0f, 145.0f, 154.0f));

	filename.push_back("D:/n50s47w-124e-121.png");
	nswe.push_back(NSWE(50.0f, 47.0f, -124.0f, -121.0f));


}

void HighResManager::DecideBestTex(RECTDIMENSION windowSize, NSWE* viewportNSWE)
{
	float pixelsperdegree;

	pixelsperdegree = (float)windowSize.width / (float)viewportNSWE->width();

	if (pixelsperdegree < (4096.0 / 360.0)) {	//does the background have a high enough pixel density
		bestImage = 0;
		return;
	}
	
	
	float areaofviewport = viewportNSWE->area();
	float bestscore;

	bestscore = 0;
	for (int i=1;i<nswe.size();i++)	{
		NSWE intersection = viewportNSWE->interectionWith(nswe[i]);
		float areatotest = nswe[i].area();
		float areaofintersection = intersection.area();

		float score = std::min(areaofintersection / areatotest, areaofintersection / areaofviewport);

		if (score > bestscore) {
			bestscore = score;
			bestImage = i;
		}
	}
	return;

}

NSWE* HighResManager::GetBestNSWE()
{
	if (bestImage == subImageLoaded) {
		return &nswe[bestImage];
	}
	else if (bestImage>0)
	{
		LoadBestTex();
	}

	return &nswe[0];
}

void HighResManager::ImageLoadThread(int n)
{
	printf("Filename: %s\n", filename[n].c_str());
	rawImageData = stbi_load(filename[n].c_str(), &width, &height, &nrChannels, 0);
	if (!rawImageData) { printf("didn't load.\n"); }
	else { printf("loaded.\n"); }
	dataReady = n;
	fileThreadLoading = false;
	subImageLoading = true;
	subImageHeightLoaded = 0;
}

void HighResManager::LoadBestTex()
{
	//printf("Loading texture\n");
	

	if ((dataReady != bestImage)) {
		//load the image from file
		if (fileThreadLoading == false) {
			printf("Loading from file %i\n", bestImage);
			fileThreadLoading = true;
			dataReady = 0;
			std::thread(&HighResManager::ImageLoadThread, this, bestImage).detach();
		}
	}
	if ((subImageLoaded != bestImage) && (fileThreadLoading==false)) {	//the dataready is the best, but the subimg hasn't been made, and we're not fileThreadLoading
//		printf("Setting up subimage\n");
	
		//set the subimage, and free the data
		glBindTexture(GL_TEXTURE_2D, highresTexture);
		int heightToLoad = std::min(height - subImageHeightLoaded, 512);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, subImageHeightLoaded, width, heightToLoad, GL_RGB, GL_UNSIGNED_BYTE, rawImageData+subImageHeightLoaded*width* nrChannels);
//		printf("Height to load: %i", heightToLoad);

		subImageHeightLoaded += heightToLoad;

		if (subImageHeightLoaded == height) {
			printf("free\n");
			stbi_image_free(rawImageData);
			glGenerateMipmap(GL_TEXTURE_2D);
			subImageLoaded = dataReady;
			dataReady = 0;
			subImageLoading = false;
		}


	}
}