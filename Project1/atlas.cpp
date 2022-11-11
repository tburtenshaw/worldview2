#include <glad/glad.h>
#include "atlas.h"
#include <limits>
#include <iostream>
#include <stb_image.h>
#include <thread>
#include "header.h"


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

void Atlas::OutputDrawOrderedUVListForUniform(MainViewport* vp, int* numberOfItems, vec4f* arrayNSWE, vec2f* arrayMult, vec2f*arrayAdd, int maxItems)
{
	//the main function called by the draw call
	
	//Find anything visible, load from file if not already
	for (auto &image :images)	{
		if (image.OverlapsWith(vp->viewNSWE)) {
			//also need to exclude if only  a few pixels
			if (image.nswe.width() / vp->DegreesPerPixel() > 1.0) {
				if (image.NeedsLoadingFromFile()) {
					image.needsLoading = false;
					std::thread(&HighResImage::LoadFileToAtlas, std::ref(image)).detach();
				}
			}
		}
	}


	//If fully loaded, then find a space for it
	for (auto& image : images) {
		if (image.dataLoaded) {
			if (!image.inAtlas) {
				image.SetAtlasPosition(MakeSpaceFor(image.position.width, image.position.height));
			}
		}
	}

	//Then load part of the subimage
	for (auto& image : images) {
		if (image.inAtlas) {
			//std::cout << "Position: " << image.filename << "\n" <<image.position.x << "," <<image.position.y <<"\n";
			if (!image.textureLoaded) {
				image.LoadTexture(texture);
			}
		}
	}

	//Now make the output array based on visible
	int n = 0;
	for (auto& image : images) {
		if (image.OverlapsWith(vp->viewNSWE) && image.textureLoaded && (image.nswe.width() / vp->DegreesPerPixel() > 1.0) && (n<maxItems)) {
			//work out a transform
			double targetW = (image.nswe.west - vp->viewNSWE.west) / vp->viewNSWE.width();
			double targetE = (image.nswe.east - vp->viewNSWE.west) / vp->viewNSWE.width();

			double targetWidth = targetE - targetW;

			double atlasTargetX = (double)image.position.x / (double)textureWidth;
			double atlasTargetWidth = (double)image.position.width / (double)textureWidth;

			//std::cout << "W:" << targetW << " E:" << targetE;
			//std::cout << "gl * " << atlasTargetWidth / targetWidth << " + " << atlasTargetX - targetW * atlasTargetWidth / targetWidth << "\n";

			double targetS = (image.nswe.south - vp->viewNSWE.south) / vp->viewNSWE.height();
			double targetN = (image.nswe.north - vp->viewNSWE.south) / vp->viewNSWE.height();
			double targetHeight = targetN - targetS;
			//std::cout << "N:" << targetN << " S:" << targetS;
			double atlasTargetY = (double)image.position.y / (double)textureHeight;
			double atlasTargetHeight = (double)image.position.height / (double)textureHeight;
			//std::cout << "gl * " << atlasTargetHeight / targetHeight << " + " << atlasTargetY - targetS * atlasTargetHeight / targetHeight << "\n";
			if (arrayNSWE && arrayMult && arrayAdd) {
				arrayNSWE[n].x = (float)targetN * vp->windowDimensions.height;
				arrayNSWE[n].y = (float)targetS * vp->windowDimensions.height;
				arrayNSWE[n].z = (float)targetW * vp->windowDimensions.width;
				arrayNSWE[n].w = (float)targetE * vp->windowDimensions.width;

				arrayMult[n].x = atlasTargetWidth / targetWidth;
				arrayMult[n].y = -atlasTargetHeight / targetHeight;
				arrayAdd[n].x = atlasTargetX - targetW * atlasTargetWidth / targetWidth;
				arrayAdd[n].y = atlasTargetY + targetN * atlasTargetHeight / targetHeight;

				++n;
				//std::cout << n << " ";
			}
			*numberOfItems = n;


		}
	}

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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);//no mipmap
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	std::cout << "made " << texture << " a " << width << "by" << height << "texture\n";
	textureWidth = width;
	textureHeight = height;
	
	
	//glGenerateMipmap(GL_TEXTURE_2D);

	//Create the initial partition the size of whole texture
	CreateMainPartition(width, height);

	PopulateHighResImages();

}

GLuint Atlas::getTexture() const
{
	return texture;
}

void HighResImage::SetAtlasPosition(AtlasRect newPos)
{
	position = newPos;
	inAtlas = true;
}

void HighResImage::LoadFileToAtlas()
{
	//int nrChannels=0;
	rawImageData = stbi_load(filename.c_str(), &position.width, &position.height, &nrChannels, 0);

	if (!rawImageData) {
		printf("Didn't load.\n");
		return;
	}
	std::cout << "Loaded: " << filename << " Size:" << position.width << "x" << position.height << needsLoading << std::endl;

	dataLoaded = true;
}

void HighResImage::LoadTexture(GLuint texture)
{
	int linesToLoad = std::min(position.height - subImageLinesLoaded, 512); //load at most 512 lines
	glBindTexture(GL_TEXTURE_2D, texture);
	//std::cout <<texture << "pos" << position.x << "," << position.y + subImageLinesLoaded << "size" << position.width << "x"  << linesToLoad << "\n";
	glTexSubImage2D(GL_TEXTURE_2D, 0, position.x, position.y + subImageLinesLoaded, position.width, linesToLoad, GL_RGB, GL_UNSIGNED_BYTE, rawImageData + (long)subImageLinesLoaded * position.width * nrChannels);
	subImageLinesLoaded += linesToLoad;


	if (subImageLinesLoaded == position.height) {
		printf("Loaded Texture. Now free\n");
		stbi_image_free(rawImageData);
		//glGenerateMipmap(GL_TEXTURE_2D);
		
		textureLoaded = true;
		//subImageLoaded = dataReady;
		//dataReady = 0;
		//subImageLoading = false;
	}


	glBindTexture(GL_TEXTURE_2D, 0);

}

bool HighResImage::OverlapsWith(NSWE other)
{
	return nswe.overlapsWith(other);
}

bool HighResImage::NeedsLoadingFromFile()
{
	return needsLoading;
}
