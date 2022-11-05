#include <glad/glad.h>
#include "atlas.h"
#include <limits>
#include <iostream>
#include <stb_image.h>
#include <thread>

void Atlas::CreateMainPartition(const int width, const int height)
{
	partitions.emplace_back(0, 0, width, height);
}

void Atlas::PopulateHighResImages()
{
	//this might alternatively load from a directory or a list
	images.emplace_back("d:/n-34s-48w166e179.png", NSWE(-34.0f, -48.0f, 166.0f, 179.0f));
	images.emplace_back("d:/n-16s-28w145e154.png", NSWE(-16.0, -28.0, 145.0, 154.0));
	images.emplace_back("d:/n-22s-25w-47e-42.png", NSWE(-22.0, -25.0, -47.0, -42.0));
	images.emplace_back("d:/n-25s-26w-55e-54.png", NSWE(-25.0, -26.0, -55.0, -54.0));
	images.emplace_back("d:/n-33s-35w150e152.png", NSWE(-33.0, -35.0, 150.0, 152.0));
	images.emplace_back("d:/n-33s-35w18e20.png", NSWE(-33.0, -35.0, 18.0, 20.0));
	images.emplace_back("d:/n31s24w72e79.png", NSWE(31.0, 24.0, 72.0, 79.0));
	images.emplace_back("d:/n39s31w-123e-115.png", NSWE(39.0, 31.0, -123.0, -115.0));
	images.emplace_back("d:/n51s46w-125e-120.png", NSWE(51.0, 46.0, -125.0, -120.0));
	images.emplace_back("d:/n55s38w0e17.png", NSWE(55.0, 38.0, 0.0, 17.0));

}


AtlasRect Atlas::MakeSpaceFor(int width, int height)
{
	int smallestArea = std::numeric_limits<int>::max();
	Partition* bestPartition=nullptr;

	for (auto &partition : partitions) {
		
		//loop through and find the smallest area that is available and fits
		if (partition.available && partition.position.height >= height && partition.position.width >= width) {	//if it could fit
			int area = partition.position.height * partition.position.width;	//find the area

			if (area < smallestArea) {
				bestPartition = &partition;
				smallestArea = area;
			}
		}
	}

	if (!bestPartition) {
		return AtlasRect(0,0,0,0);
	}
	
	bestPartition->available = false;
	Partition copyBP = *bestPartition;
	printf("rect(%i,%i, %i,%i,10);", copyBP.position.x, copyBP.position.y, width, height);
	//printf("Block %i,%i. X: %i, Y: %i. W: %i, h: %i\n", width, height, copyBP.x, copyBP.y, copyBP.width, copyBP.height);

	//make two new partitions
	if (copyBP.position.width - width > 0) {
		//printf("Made: X: %i, Y: %i. W: %i, h: %i\n", copyBP.x + width,copyBP.y, copyBP.width-width, copyBP.height);
		partitions.emplace_back(copyBP.position.x + width, copyBP.position.y, copyBP.position.width - width, copyBP.position.height); //to the right, full height
	}
	if (copyBP.position.height - height > 0) {
		//printf("And: X: %i, Y: %i. W: %i, h: %i\n", copyBP.x, copyBP.y+height, width, copyBP.height-height);
		partitions.emplace_back(copyBP.position.x, copyBP.position.y + height, width, copyBP.position.height - height); //one above the best, same width as it
	}

	AtlasRect returnRect;

	returnRect = { copyBP.position.x, copyBP.position.y, width, height };
	partitions.emplace_back(returnRect, false); //one the image is in, it starts as not available

	return returnRect;
}

void Atlas::OutputDrawOrderedUVListForUniform(const NSWE& viewNSWE, int* numberOfItems, float* array, int maxItems)
{
	//the main function called by the draw call
	//WE ALSO NEED VIEWPORT SIZE as the textures depend on the dpp

	for (auto &image :images)	{
		if (image.OverlapsWith(viewNSWE)) {
			//std::cout << "Overlaps:" << image.filename << viewNSWE << image.nswe << std::endl;
			if (image.NeedsLoadingFromFile()) {
				image.needsLoading = false;
				std::thread(&HighResImage::LoadFileToAtlas, image).detach();
				//image.LoadFileToAtlas();
			}
		}
	}

	//the atlas position, and the glsubimage won't be in the loading thread
	//image.SetAtlasPosition(MakeSpaceFor(image.position.width, image.position.height));

}

void Atlas::Setup(int width, int height)
{
	//Set up the texture to load files to
	GLint maxTextureSize;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
	printf("Max texture size: %i\n", maxTextureSize);

	width = std::min(width, maxTextureSize);
	height = std::min(height, maxTextureSize);

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glGenerateMipmap(GL_TEXTURE_2D);

	//Create the initial partition the size of whole texture
	CreateMainPartition(width, height);

	PopulateHighResImages();

}

GLuint Atlas::getTexture() const
{
	return texture;
}

void HighResImage::LoadFileToAtlas()
{
	int nrChannels=0;
	rawImageData = stbi_load(filename.c_str(), &position.width, &position.height, &nrChannels, 0);

	if (!rawImageData) {
		printf("Didn't load.\n");
		return;
	}
	std::cout << "Loaded: " << filename << " Size:" << position.width << "x" << position.height << needsLoading << std::endl;

	dataLoaded = true;
}

bool HighResImage::OverlapsWith(NSWE other)
{
	return nswe.overlapsWith(other);
}

bool HighResImage::NeedsLoadingFromFile()
{
	return needsLoading;
}
