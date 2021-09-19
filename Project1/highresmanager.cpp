#include <vector>
#include <string>
#include <thread>

#include "header.h"
#include "highresmanager.h"
#include "nswe.h"

#include <stb_image.h>

#include <glad/glad.h> 
#include <GLFW/glfw3.h>
#include <iostream>
#include <filesystem>


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

	subImageLinesLoaded = 0;
	subImageLoading = false;

	highresTexture = 0;

	filename.push_back("Background");
	nswe.push_back(NSWE(10000.0f, 20000.0f, 30000.0f, 40000.0f));	//should never be displayed


	filename.push_back("D:/n-34s-48w166e179.png"); //nz
	nswe.push_back(NSWE(-34.0f, -48.0f, 166.0f, 179.0f));


	
	filename.push_back("D:/n55s38w0e17.png");
	nswe.push_back(NSWE(55.0f, 38.0f, 0.0f, 17.0f));

	filename.push_back("D:/n-16s-28w145e154.png");
	nswe.push_back(NSWE(-16.0f, -28.0f, 145.0f, 154.0f));

	filename.push_back("D:/n51s46w-125e-120.png");
	nswe.push_back(NSWE(51.0f, 46.0f, -125.0f, -120.0f));

	filename.push_back("D:/n-33s-35w18e20.png");
	nswe.push_back(NSWE(-33.0f, -35.0f, 18.0f, 20.0f));
	
	filename.push_back("d:/n-25s-26w-55e-54.png");
	nswe.push_back(NSWE(-25.0f, -26.0f, -55.0f, -54.0f));

	filename.push_back("d:/n-22s-25w-47e-42.png");
	nswe.push_back(NSWE(-22.0f, -25.0f, -47.0f, -42.0f));

	filename.push_back("d:/n-33s-35w150e152.png");
	nswe.push_back(NSWE(-33.0f, -35.0f, 150.0f, 152.0f));

	filename.push_back("d:/n31s24w72e79.png");
	nswe.push_back(NSWE(31.0f, 24.0f, 72.0f, 79.0f));
	
	filename.push_back("d:/n39s31w-123e-115.png");
	nswe.push_back(NSWE(39.0f, 31.0f, -123.0f, -115.0f));

	std::filesystem::path p="n39s31w-123e-115.png";
	std::filesystem::path ap;

	
	ap = std::filesystem::absolute(p);


	printf("Path: %s\n", ap.parent_path().string().c_str());
	std::cout << "Absolute path for " << p << " is "
		<< std::filesystem::absolute(p) << '\n';


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
		NSWE intersection = viewportNSWE->intersectionWith(nswe[i]);
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
	if ((bestImage == subImageLoaded) && ((bestImage==dataReady) || (dataReady==0))) {
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
	printf("Filename: %s, number: %i\n", filename[n].c_str(),n);
	rawImageData = stbi_load(filename[n].c_str(), &width, &height, &nrChannels, 0);
	if (!rawImageData) { 
		printf("Didn't load.\n");
		fileThreadLoading = false;
		//now we should remove the file from the list
		filename.erase(filename.begin()+n);
		nswe.erase(nswe.begin() + n);

		bestImage = 0;
		subImageLoaded = 0;
		fileThreadLoading = false;
		subImageLoading = false;
		subImageLinesLoaded = 0;

		return;
	}
	//else { printf("loaded. W:%i H:%i\n", width, height); }
	dataReady = n;
	fileThreadLoading = false;
	subImageLoading = true;
	subImageLinesLoaded = 0;
}

void HighResManager::LoadBestTex()
{
	//printf("Loading texture\n");
	

	if ((dataReady != bestImage) && (subImageLoading==false)) {
		//load the image from file
		if (fileThreadLoading == false) {
			printf("Loading from file %i\n", bestImage);
			fileThreadLoading = true;
			dataReady = 0;
			std::thread(&HighResManager::ImageLoadThread, this, bestImage).detach();
		}
	}
	if ((subImageLoading==true) && (fileThreadLoading==false)) {
		//printf("subimageloading. W:%i H:%i\n", width, height);
		glBindTexture(GL_TEXTURE_2D, highresTexture);
		//if we try to load a whole image at once, it'll cause stutter, so we do a few lines at a time
		int linesToLoad = std::min(height - subImageLinesLoaded, 512); //load at most 512 lines
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, subImageLinesLoaded, width, linesToLoad, GL_RGB, GL_UNSIGNED_BYTE, rawImageData + (long)subImageLinesLoaded*width* nrChannels);
		subImageLinesLoaded += linesToLoad;

		if (subImageLinesLoaded == height) {
			printf("free\n");
			stbi_image_free(rawImageData);
			glGenerateMipmap(GL_TEXTURE_2D);
			subImageLoaded = dataReady;
			dataReady = 0;
			subImageLoading = false;
		}
	}
}