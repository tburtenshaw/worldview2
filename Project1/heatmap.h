#pragma once
class NSWE;
class Shader;
//#include "header.h"
//#include "nswe.h"


class Heatmap {
private:
	bool dirty;
public:
	int width;
	int height;
	NSWE *nswe;

	int activeheatmap;

	float pixel[4096*4096];
	float maxPixel;

	void StampGaussian(float fx, float fy, float stddev, long seconds);
	void GaussianBlur(float sigma);

	void MakeDirty();
	void MakeClean();
	bool IsDirty();


	Heatmap();
	~Heatmap();
};

