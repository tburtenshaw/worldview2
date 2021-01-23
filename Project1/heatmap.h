#pragma once
class NSWE;
class Shader;
//#include "header.h"
//#include "nswe.h"

#define MAX_HEATMAP_DIMENSION 2048

class Heatmap {
private:
	bool dirty;
public:
	int width;
	int height;
	NSWE *nswe;
	float overdrawfactor;

	int activeheatmap;

	float pixel[MAX_HEATMAP_DIMENSION * MAX_HEATMAP_DIMENSION];
	float maxPixel;

	int roughHeatmap[MAX_HEATMAP_DIMENSION* MAX_HEATMAP_DIMENSION /24 /24];	//this is used to determine whether we blur it (as don't want to waste time blurring regions of empty)

	void CreateHeatmap(NSWE* nswe, int n);
	void StampGaussian(float fx, float fy, float stddev, long seconds);
	void GaussianBlur(float sigma);

	void MakeDirty();
	void MakeClean();
	bool IsDirty();


	Heatmap();
	~Heatmap();
};

