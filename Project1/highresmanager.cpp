#include <vector>
#include <string>
#include <thread>

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

}

void HighResManager::DecideBestTex(RECTDIMENSION windowSize, NSWE* nswe)
{
	float pixelsperdegree;

	pixelsperdegree = (float)windowSize.width / (float)nswe->width();

	if (pixelsperdegree < (4096.0 / 360.0)) {	//does the background have a high enough pixel density
		bestImage = 0;
		return;
	}
	
	if (nswe->east > 166.0f) {
		bestImage = 1;
	}
	else
	{
		bestImage = 2;
	}

}

NSWE* HighResManager::GetBestNSWE()
{
	if (bestImage == subImageLoaded) {
		return &nswe[bestImage];
	}
	else if (bestImage>0)
	{
	//	subImageLoaded = 0;
		LoadBestTex();
	}

	return &nswe[0];
}

void HighResManager::ImageLoadThread(int n)
{
	printf("%s\n", filename[n].c_str());
	rawImageData = stbi_load(filename[n].c_str(), &width, &height, &nrChannels, 0);
	if (!rawImageData) { printf("didn't load.\n"); }
	dataReady = n;
}

void HighResManager::LoadBestTex()
{
	printf("Loading texture\n");
	if (dataReady != bestImage) {
		//load the image from file
		printf("Loading from file %i\n",bestImage);
		dataReady = 0;
		//std::thread imageload(&HighResManager::ImageLoadThread,this,bestImage);
		ImageLoadThread(bestImage);
	}
	else if (subImageLoaded != bestImage) {
		printf("Setting up subimage\n");
		//set the subimage, and free the data
		glBindTexture(GL_TEXTURE_2D, highresTexture);
		printf("After bind: glGetError %i\n", glGetError());
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, rawImageData);
		stbi_image_free(rawImageData);

		glGenerateMipmap(GL_TEXTURE_2D);
		printf("glGetError %i\n", glGetError());
		subImageLoaded = dataReady;
		dataReady = 0;
	}
}