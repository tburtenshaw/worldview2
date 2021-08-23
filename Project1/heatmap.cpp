#include "header.h"
#include "nswe.h"
#include "heatmap.h"
#include "shaders.h"
#include <vector>
#include <stdio.h>
#include <iostream>
#include <math.h>
#include <algorithm>

extern LocationHistory* pLocationHistory;

void Heatmap::CreateHeatmap(NSWE * inputNswe, int n) {
	
	GlobalOptions* options;
	options = pLocationHistory->globalOptions;

	NSWE expanded;

	expanded = inputNswe->createExpandedBy(overdrawfactor);

	nswe->setto(&expanded);
	
	maxPixel = 0;
	memset(pixel, 0, sizeof(pixel));
	
	//this acts as a mask, so we don't try to blur nothing
	memset(roughHeatmap, 0, sizeof(roughHeatmap));


	unsigned long tsold;
	long tsdiff;	//the difference between the timestamps

	tsold = pLocationHistory->locations.front().timestamp;

	float p;
	p = 0;
	

	//clamp to MAX_HEATMAP_DIMENSION
	width = std::min(pLocationHistory->windowDimensions->width, MAX_HEATMAP_DIMENSION);
	height = std::min(pLocationHistory->windowDimensions->height, MAX_HEATMAP_DIMENSION);

	printf("Heatmap pixel size: W%i H%i. West: %f, East: %f, Map width: %f\n",width,height,nswe->west,nswe->east,nswe->width());

	for (std::vector<LOCATION>::iterator iter = pLocationHistory->locations.begin(); iter != pLocationHistory->locations.end(); ++iter) {
		int x,y, xold, yold;
		double fx, fy;

		tsdiff = iter->timestamp - tsold;
		if (tsdiff > 60*60*24*365) { tsdiff = 0; }	//if the change in time is over a year
		if (tsdiff < 0) { tsdiff = 0; }	//or negative, then we don't use it

		double wrappedLongitude;
		wrappedLongitude = iter->longitude;

		//if the viewport is too far west, then move data over
		if ((nswe->east< 180.0) && (nswe->west <= -180.0) &&(wrappedLongitude > nswe->east))	{
					wrappedLongitude -= 360.0;
		}

		//same with the eastern data
		if ((nswe->west > -180.0) && (nswe->east >= 180.0) && (wrappedLongitude < nswe->west)) {
			wrappedLongitude += 360.0;
		}

		fx = (wrappedLongitude - (double)nswe->west) / (double)nswe->width() * (double)width;
		fy = ((double)nswe->north - iter->latitude) / (double)nswe->height() * (double)height;
		
		
		x = (int)(fx);
		y = (int)(fy);

		float dx, dy;



		if ((x < width) && (x >= 0) && (y < height) && (y >= 0)) {	//if the point falls within the heatmap
			
			//Single pixel
			if (iter->accuracy < options->minimumaccuracy) {
				//pixel[y * width + x] += (tsdiff * 10);

				//antialias amongst four pixels based on rounding.
				dx = (float)fx - (float)x;
				dy = (float)fy - (float)y;
				
				pixel[y * width + x] += (float)(tsdiff * 10)*(1.0f-dx)*(1.0f-dy);
				pixel[y * width + x+1] += (float)(tsdiff * 10) * dx * (1.0f - dy);
				pixel[(y+1) * width + x] += (float)(tsdiff * 10) * (1.0f - dx) * (dy);
				pixel[(y+1) * width + x+1] += (float)(tsdiff * 10) * (  dx) * (dy);


				int roughPixel = (y / 24 * width/24 + x / 24);
				//printf("%i %i: %i %i: %i\n", x,y,y/24,x/24,roughPixel);
				roughHeatmap[std::max(roughPixel - 1,0)] = 1;
				roughHeatmap[roughPixel] = 1;	//there's something there
				roughHeatmap[std::min(roughPixel+1,(int)sizeof(roughHeatmap)-1)] = 1;
				

			}
			


			//Gaussian matrix
			//printf("%i ", iter->accuracy);
			//if (options->blurperaccuracy) {
			//	StampGaussian(fx, fy, iter->accuracy / 10, tsdiff);
			//}

			p = pixel[y * width + x];

			if (p > maxPixel) {
				maxPixel = p;
			}

		}

		tsold = iter->timestamp;
		xold = x;
		yold = y;

	}


	GaussianBlur(options->gaussianblur);
	return;

}

void Heatmap::StampGaussian(float fx, float fy, float stddev, long seconds) {
	int maxradius =5;
	
	int xstart = fx - maxradius;
	int xfinish = fx + maxradius;
	int ystart = fy - maxradius;
	int yfinish = fy + maxradius;
	
	if (xstart < 0)	xstart = 0;
	if (ystart < 0) ystart = 0;
	if (xfinish > width-1) {
		xfinish = width - 1;
	}
	if (yfinish > height - 1) {
		yfinish = height - 1;
	}


	for (int y = ystart; y <= yfinish; y++) {
		for (int x = xstart; x <= xfinish; x++) {
			
			pixel[y * width + x] += (double)seconds*exp(-0.5 * (pow((x - fx) / stddev, 2.0) + pow((y - fy) / stddev, 2.0)))
				/ (2 * 3.14159 * stddev * stddev);

			printf("%i, %i\n", x, y);

		}
	}
}

void Heatmap::GaussianBlur(float sigma)	//this takes a radius, that is rounded to the nearest 0.1
{	
	
	int matrixrow;
	matrixrow = (int)(10.0f * sigma);	//each row is the sigma*10 (so row 4 is 0.4)
	if (matrixrow > 100) { matrixrow = 100; }
	if (matrixrow < 4) { return; }	//as sigma less than 0.4 doesn't blur

	if (height < 3) { return; } //no point if the screen is tiny
	if (width < 3) { return; }

	//I left the matrix as int, as I'm more confident that it'll be stopped at zero.
	//As the the shader taking the loga
	static const int gaussianMatrix[101][44] = {	
{1,0}, //element 0;
{60000, 0}, //1 items+zero. sigma=0.1
{60000, 0}, //1 items+zero. sigma=0.2
{59769, 231, 0}, //2 items+zero. sigma=0.3
{57475, 2525, 0}, //2 items+zero. sigma=0.4
{52832, 7150, 18, 0}, //3 items+zero. sigma=0.5 (equiv to row 1 on Pascal's Triangle)
{47877, 11938, 185, 0}, //3 items+zero. sigma=0.6
{43559, 15701, 735, 4, 0}, //4 items+zero. sigma=0.7
{39929, 18281, 1754, 35, 0}, //4 items+zero. sigma=0.8
{36855, 19880, 3120, 142, 2, 0}, //5 items+zero. sigma=0.9
{34221, 20756, 4631, 380, 11, 0}, //5 items+zero. sigma=1.0 (equiv to row 3 on Pascal's Triangle)
{31938, 21127, 6116, 775, 43, 1, 0}, //6 items+zero. sigma=1.1
{29940, 21157, 7466, 1315, 116, 5, 0}, //6 items+zero. sigma=1.2
{28178, 20962, 8629, 1966, 248, 17, 1, 0}, //7 items+zero. sigma=1.3
{26612, 20620, 9592, 2679, 449, 45, 3, 0}, //7 items+zero. sigma=1.4
{25210, 20187, 10364, 3412, 720, 97, 8, 0}, //7 items+zero. sigma=1.5 (equiv to row 9 on Pascal's Triangle)
{23949, 19700, 10965, 4129, 1052, 181, 21, 2, 0}, //8 items+zero. sigma=1.6
{22808, 19185, 11417, 4807, 1432, 302, 45, 5, 0}, //8 items+zero. sigma=1.7
{21771, 18658, 11743, 5429, 1843, 460, 84, 11, 1, 0}, //9 items+zero. sigma=1.8
{20824, 18131, 11966, 5987, 2271, 653, 142, 24, 3, 0}, //9 items+zero. sigma=1.9
{19956, 17611, 12104, 6479, 2701, 877, 222, 44, 7, 1, 0}, //10 items+zero. sigma=2.0 (equiv to row 16 on Pascal's Triangle)
{19157, 17104, 12172, 6905, 3122, 1125, 323, 74, 14, 2, 0}, //10 items+zero. sigma=2.1
{18420, 16612, 12185, 7270, 3527, 1392, 447, 117, 25, 4, 1, 0}, //11 items+zero. sigma=2.2
{17738, 16138, 12154, 7576, 3909, 1670, 590, 173, 42, 8, 1, 0}, //11 items+zero. sigma=2.3
{17104, 15682, 12087, 7831, 4265, 1953, 751, 243, 66, 15, 3, 0}, //11 items+zero. sigma=2.4
{16514, 15244, 11992, 8038, 4592, 2235, 927, 328, 99, 25, 6, 1, 0}, //12 items+zero. sigma=2.5 (equiv to row 25 on Pascal's Triangle)
{15963, 14825, 11875, 8204, 4888, 2512, 1114, 426, 140, 40, 10, 2, 0}, //12 items+zero. sigma=2.6
{15448, 14424, 11742, 8333, 5156, 2781, 1308, 536, 192, 60, 16, 4, 1, 0}, //13 items+zero. sigma=2.7
{14965, 14041, 11596, 8430, 5394, 3038, 1507, 658, 253, 85, 25, 7, 2, 0}, //13 items+zero. sigma=2.8
{14512, 13674, 11440, 8498, 5605, 3283, 1707, 788, 323, 118, 38, 11, 3, 1, 0}, //14 items+zero. sigma=2.9
{14085, 13324, 11278, 8543, 5790, 3512, 1906, 926, 402, 156, 54, 17, 5, 1, 0}, //14 items+zero. sigma=3.0 (equiv to row 36 on Pascal's Triangle)
{13682, 12988, 11111, 8566, 5951, 3726, 2102, 1069, 490, 202, 75, 25, 8, 2, 1, 0}, //15 items+zero. sigma=3.1
{13302, 12668, 10942, 8572, 6090, 3924, 2294, 1216, 584, 255, 101, 36, 12, 3, 1, 0}, //15 items+zero. sigma=3.2
{12942, 12362, 10771, 8562, 6208, 4107, 2478, 1364, 685, 314, 131, 50, 17, 6, 2, 0}, //15 items+zero. sigma=3.3
{12602, 12068, 10600, 8538, 6308, 4274, 2656, 1514, 791, 379, 167, 67, 25, 8, 3, 1, 0}, //16 items+zero. sigma=3.4
{12278, 11787, 10429, 8504, 6390, 4426, 2825, 1662, 901, 450, 207, 88, 34, 12, 4, 1, 0}, //16 items+zero. sigma=3.5 (equiv to row 49 on Pascal's Triangle)
{11971, 11518, 10259, 8460, 6457, 4563, 2985, 1808, 1013, 526, 253, 112, 46, 18, 6, 2, 1, 0}, //17 items+zero. sigma=3.6
{11679, 11261, 10092, 8407, 6511, 4687, 3136, 1951, 1128, 606, 303, 141, 61, 24, 9, 3, 1, 0}, //17 items+zero. sigma=3.7
{11401, 11013, 9927, 8349, 6552, 4797, 3278, 2090, 1243, 690, 357, 173, 78, 33, 13, 5, 2, 1, 0}, //18 items+zero. sigma=3.8
{11136, 10776, 9764, 8284, 6581, 4896, 3410, 2224, 1358, 777, 416, 209, 98, 43, 18, 7, 2, 1, 0}, //18 items+zero. sigma=3.9
{10883, 10548, 9604, 8215, 6601, 4983, 3533, 2354, 1473, 866, 478, 248, 121, 55, 24, 10, 4, 1, 0}, //18 items+zero. sigma=4.0 (equiv to row 64 on Pascal's Triangle)
{10641, 10329, 9447, 8142, 6611, 5059, 3647, 2477, 1586, 956, 544, 291, 147, 70, 31, 13, 5, 2, 1, 0}, //19 items+zero. sigma=4.1
{10410, 10119, 9294, 8066, 6614, 5125, 3752, 2596, 1697, 1048, 612, 337, 176, 87, 40, 18, 7, 3, 1, 0}, //19 items+zero. sigma=4.2
{10188, 9916, 9144, 7987, 6610, 5182, 3849, 2708, 1805, 1140, 682, 386, 207, 106, 51, 23, 10, 4, 2, 1, 0}, //20 items+zero. sigma=4.3
{9976, 9721, 8997, 7907, 6599, 5230, 3937, 2814, 1910, 1231, 754, 438, 242, 127, 63, 30, 13, 6, 2, 1, 0}, //20 items+zero. sigma=4.4
{9772, 9534, 8853, 7825, 6583, 5271, 4017, 2914, 2012, 1323, 827, 493, 279, 151, 77, 38, 18, 8, 3, 1, 1, 0}, //21 items+zero. sigma=4.5 (equiv to row 81 on Pascal's Triangle)
{9577, 9353, 8713, 7742, 6562, 5305, 4090, 3009, 2111, 1412, 902, 549, 319, 177, 93, 47, 23, 10, 5, 2, 1, 0}, //21 items+zero. sigma=4.6
{9389, 9179, 8576, 7658, 6536, 5332, 4156, 3097, 2205, 1501, 976, 607, 361, 205, 111, 58, 29, 14, 6, 3, 1, 0}, //21 items+zero. sigma=4.7
{9208, 9011, 8443, 7574, 6507, 5352, 4216, 3180, 2296, 1588, 1051, 666, 405, 235, 131, 70, 36, 17, 8, 4, 2, 1, 0}, //22 items+zero. sigma=4.8
{9034, 8848, 8312, 7490, 6474, 5368, 4269, 3256, 2383, 1672, 1126, 727, 450, 268, 153, 83, 44, 22, 11, 5, 2, 1, 0}, //22 items+zero. sigma=4.9
{8867, 8692, 8185, 7406, 6439, 5378, 4316, 3328, 2465, 1755, 1200, 788, 498, 302, 176, 99, 53, 27, 14, 6, 3, 1, 1, 0}, //23 items+zero. sigma=5.0 (equiv to row 99 on Pascal's Triangle)
{8706, 8540, 8062, 7323, 6401, 5384, 4358, 3394, 2544, 1835, 1273, 850, 547, 338, 201, 115, 63, 34, 17, 8, 4, 2, 1, 0}, //23 items+zero. sigma=5.1
{8550, 8394, 7941, 7240, 6361, 5385, 4394, 3455, 2618, 1912, 1346, 913, 596, 376, 228, 133, 75, 41, 21, 11, 5, 2, 1, 0}, //23 items+zero. sigma=5.2
{8400, 8252, 7823, 7157, 6318, 5383, 4426, 3512, 2689, 1987, 1417, 975, 647, 415, 257, 153, 88, 49, 26, 14, 7, 3, 2, 1, 0}, //24 items+zero. sigma=5.3
{8255, 8115, 7708, 7075, 6275, 5377, 4453, 3563, 2755, 2059, 1486, 1037, 699, 455, 287, 174, 102, 58, 32, 17, 9, 4, 2, 1, 0}, //24 items+zero. sigma=5.4
{8116, 7982, 7596, 6994, 6230, 5369, 4476, 3611, 2818, 2127, 1554, 1098, 751, 497, 318, 197, 118, 68, 38, 21, 11, 6, 3, 1, 1, 0}, //25 items+zero. sigma=5.5 (equiv to row 120 on Pascal's Triangle)
{7980, 7854, 7487, 6913, 6183, 5357, 4495, 3654, 2876, 2194, 1620, 1159, 803, 539, 351, 221, 135, 80, 46, 25, 14, 7, 4, 2, 1, 0}, //25 items+zero. sigma=5.6
{7849, 7730, 7381, 6834, 6136, 5343, 4511, 3693, 2932, 2257, 1685, 1219, 856, 583, 384, 246, 153, 92, 54, 30, 17, 9, 5, 2, 1, 1, 0}, //26 items+zero. sigma=5.7
{7723, 7609, 7277, 6756, 6088, 5326, 4523, 3728, 2983, 2317, 1747, 1279, 908, 626, 419, 273, 172, 105, 63, 36, 20, 11, 6, 3, 1, 1, 0}, //26 items+zero. sigma=5.8
{7600, 7492, 7176, 6679, 6040, 5307, 4532, 3760, 3031, 2374, 1807, 1337, 961, 671, 455, 300, 192, 120, 72, 43, 24, 13, 7, 4, 2, 1, 0}, //26 items+zero. sigma=5.9
{7481, 7378, 7077, 6602, 5991, 5287, 4538, 3788, 3076, 2429, 1866, 1394, 1012, 715, 492, 329, 214, 135, 83, 50, 29, 16, 9, 5, 3, 1, 1, 0}, //27 items+zero. sigma=6.0 (equiv to row 143 on Pascal's Triangle)
{7366, 7268, 6981, 6527, 5941, 5264, 4541, 3813, 3117, 2481, 1922, 1449, 1064, 760, 529, 358, 236, 152, 95, 58, 34, 20, 11, 6, 3, 2, 1, 0}, //27 items+zero. sigma=6.1
{7255, 7161, 6887, 6453, 5892, 5241, 4542, 3835, 3156, 2530, 1976, 1503, 1115, 805, 567, 389, 260, 169, 107, 66, 40, 23, 13, 7, 4, 2, 1, 1, 0}, //28 items+zero. sigma=6.2
{7146, 7057, 6795, 6380, 5842, 5216, 4541, 3855, 3191, 2576, 2028, 1556, 1165, 850, 605, 420, 284, 187, 121, 76, 46, 28, 16, 9, 5, 3, 1, 1, 0}, //28 items+zero. sigma=6.3
{7041, 6956, 6706, 6309, 5792, 5189, 4537, 3871, 3224, 2620, 2077, 1608, 1214, 895, 644, 452, 309, 207, 135, 86, 53, 32, 19, 11, 6, 3, 2, 1, 0}, //28 items+zero. sigma=6.4
{6939, 6858, 6618, 6238, 5742, 5162, 4532, 3886, 3254, 2661, 2125, 1657, 1262, 939, 682, 484, 335, 227, 150, 97, 61, 38, 23, 13, 8, 4, 2, 1, 1, 0}, //29 items+zero. sigma=6.5 (equiv to row 168 on Pascal's Triangle)
{6840, 6762, 6533, 6169, 5692, 5134, 4525, 3898, 3281, 2699, 2170, 1706, 1310, 983, 721, 517, 362, 248, 166, 109, 69, 43, 26, 16, 9, 5, 3, 2, 1, 0}, //29 items+zero. sigma=6.6
{6744, 6669, 6450, 6100, 5643, 5105, 4516, 3907, 3306, 2736, 2214, 1752, 1356, 1027, 760, 550, 390, 270, 183, 121, 78, 50, 31, 19, 11, 6, 4, 2, 1, 1, 0}, //30 items+zero. sigma=6.7
{6650, 6578, 6369, 6033, 5594, 5075, 4506, 3915, 3329, 2770, 2255, 1797, 1401, 1070, 799, 584, 417, 292, 200, 134, 88, 56, 35, 22, 13, 8, 4, 3, 1, 1, 0}, //30 items+zero. sigma=6.8
{6559, 6490, 6289, 5967, 5544, 5044, 4494, 3921, 3349, 2802, 2295, 1841, 1446, 1112, 837, 617, 446, 315, 218, 148, 98, 64, 41, 25, 15, 9, 5, 3, 2, 1, 1, 0}, //31 items+zero. sigma=6.9
{6470, 6405, 6211, 5903, 5496, 5013, 4481, 3924, 3367, 2831, 2332, 1882, 1489, 1153, 876, 651, 475, 339, 237, 163, 109, 72, 46, 29, 18, 11, 7, 4, 2, 1, 1, 0}, //31 items+zero. sigma=7.0 (equiv to row 195 on Pascal's Triangle)
{6384, 6321, 6136, 5839, 5447, 4982, 4467, 3927, 3384, 2859, 2368, 1923, 1530, 1194, 914, 685, 504, 363, 257, 178, 121, 80, 53, 34, 21, 13, 8, 5, 3, 2, 1, 0}, //31 items+zero. sigma=7.1
{6300, 6239, 6062, 5776, 5399, 4950, 4452, 3927, 3398, 2884, 2401, 1961, 1571, 1234, 951, 719, 533, 388, 277, 194, 133, 90, 59, 38, 24, 15, 9, 6, 3, 2, 1, 1, 0}, //32 items+zero. sigma=7.2
{6218, 6160, 5989, 5715, 5351, 4918, 4436, 3926, 3411, 2908, 2433, 1998, 1610, 1274, 989, 753, 563, 413, 297, 210, 146, 99, 66, 43, 28, 18, 11, 7, 4, 2, 1, 1, 0}, //32 items+zero. sigma=7.3
{6138, 6083, 5918, 5654, 5304, 4886, 4419, 3924, 3422, 2930, 2463, 2033, 1648, 1312, 1025, 787, 593, 439, 319, 227, 159, 109, 74, 49, 32, 20, 13, 8, 5, 3, 2, 1, 1, 0}, //33 items+zero. sigma=7.4
{6061, 6007, 5849, 5595, 5257, 4853, 4401, 3921, 3431, 2950, 2492, 2067, 1685, 1349, 1061, 820, 623, 464, 340, 245, 173, 120, 82, 55, 36, 23, 15, 9, 6, 3, 2, 1, 1, 0}, //33 items+zero. sigma=7.5 (equiv to row 224 on Pascal's Triangle)
{5985, 5933, 5781, 5536, 5211, 4820, 4382, 3916, 3439, 2969, 2518, 2100, 1721, 1386, 1097, 853, 653, 490, 362, 263, 188, 132, 91, 61, 41, 27, 17, 11, 7, 4, 2, 1, 1, 0}, //33 items+zero. sigma=7.6
{5911, 5861, 5715, 5479, 5165, 4787, 4363, 3910, 3446, 2985, 2543, 2131, 1755, 1421, 1132, 886, 682, 517, 385, 282, 203, 143, 100, 68, 46, 30, 20, 13, 8, 5, 3, 2, 1, 1, 0}, //34 items+zero. sigma=7.7
{5839, 5791, 5650, 5423, 5119, 4754, 4344, 3903, 3451, 3001, 2567, 2160, 1788, 1456, 1166, 919, 712, 543, 407, 301, 218, 156, 109, 76, 51, 34, 23, 15, 9, 6, 4, 2, 1, 1, 0}, //34 items+zero. sigma=7.8
{5769, 5723, 5587, 5367, 5075, 4722, 4323, 3896, 3455, 3015, 2589, 2188, 1820, 1490, 1200, 951, 742, 570, 430, 320, 234, 169, 119, 83, 57, 39, 26, 17, 11, 7, 4, 3, 2, 1, 1, 0}, //35 items+zero. sigma=7.9
{5700, 5656, 5525, 5313, 5030, 4689, 4303, 3887, 3457, 3027, 2610, 2215, 1850, 1522, 1233, 983, 771, 596, 453, 340, 250, 182, 130, 91, 63, 43, 29, 19, 12, 8, 5, 3, 2, 1, 1, 0}, //35 items+zero. sigma=8.0 (equiv to row 255 on Pascal's Triangle)
{5633, 5590, 5464, 5259, 4986, 4656, 4281, 3878, 3459, 3038, 2629, 2240, 1880, 1554, 1265, 1014, 801, 623, 477, 360, 267, 196, 141, 100, 70, 48, 33, 22, 14, 9, 6, 4, 2, 1, 1, 0}, //35 items+zero. sigma=8.1
{5567, 5526, 5404, 5207, 4943, 4623, 4260, 3867, 3459, 3048, 2647, 2264, 1908, 1584, 1296, 1045, 830, 649, 500, 380, 284, 210, 152, 109, 77, 53, 37, 25, 16, 11, 7, 4, 3, 2, 1, 1, 0}, //36 items+zero. sigma=8.2
{5503, 5464, 5346, 5155, 4900, 4590, 4238, 3856, 3459, 3057, 2663, 2287, 1935, 1614, 1327, 1075, 858, 676, 524, 401, 302, 224, 164, 118, 84, 59, 41, 28, 19, 12, 8, 5, 3, 2, 1, 1, 0}, //36 items+zero. sigma=8.3
{5441, 5402, 5289, 5105, 4858, 4557, 4216, 3845, 3457, 3065, 2679, 2308, 1961, 1643, 1357, 1105, 887, 702, 548, 421, 320, 239, 176, 128, 92, 65, 45, 31, 21, 14, 9, 6, 4, 2, 2, 1, 1, 0}, //37 items+zero. sigma=8.4
{5380, 5343, 5233, 5055, 4816, 4525, 4193, 3833, 3455, 3071, 2693, 2329, 1986, 1670, 1386, 1134, 915, 728, 571, 442, 338, 254, 189, 138, 100, 71, 50, 35, 24, 16, 11, 7, 4, 3, 2, 1, 1, 0}, //37 items+zero. sigma=8.5 (equiv to row 288 on Pascal's Triangle)
{5320, 5284, 5178, 5006, 4774, 4493, 4171, 3820, 3451, 3077, 2706, 2348, 2010, 1697, 1414, 1162, 943, 754, 595, 463, 356, 270, 202, 149, 108, 78, 55, 39, 27, 18, 12, 8, 5, 3, 2, 1, 1, 1, 0}, //38 items+zero. sigma=8.6
{5261, 5227, 5124, 4958, 4734, 4460, 4148, 3806, 3447, 3081, 2718, 2366, 2032, 1723, 1441, 1190, 970, 780, 619, 485, 375, 286, 215, 160, 117, 85, 60, 43, 30, 20, 14, 9, 6, 4, 3, 2, 1, 1, 0}, //38 items+zero. sigma=8.7
{5204, 5171, 5072, 4910, 4693, 4428, 4125, 3793, 3443, 3085, 2729, 2383, 2054, 1748, 1468, 1217, 997, 805, 642, 506, 393, 302, 229, 171, 126, 92, 66, 47, 33, 23, 16, 11, 7, 5, 3, 2, 1, 1, 0}, //38 items+zero. sigma=8.8
{5148, 5116, 5020, 4864, 4654, 4397, 4102, 3779, 3437, 3087, 2739, 2399, 2074, 1772, 1494, 1244, 1023, 831, 666, 527, 412, 318, 243, 183, 136, 100, 72, 52, 37, 25, 18, 12, 8, 5, 3, 2, 1, 1, 1, 0}, //39 items+zero. sigma=8.9
{5093, 5062, 4969, 4818, 4614, 4365, 4079, 3764, 3431, 3089, 2747, 2413, 2094, 1795, 1519, 1270, 1049, 856, 689, 549, 431, 335, 257, 194, 145, 108, 78, 57, 40, 28, 20, 14, 9, 6, 4, 3, 2, 1, 1, 0}, //39 items+zero. sigma=9.0 (equiv to row 323 on Pascal's Triangle)
{5040, 5009, 4920, 4773, 4576, 4334, 4055, 3749, 3424, 3090, 2755, 2427, 2113, 1817, 1543, 1295, 1074, 880, 713, 570, 450, 352, 271, 207, 156, 116, 85, 62, 44, 31, 22, 15, 10, 7, 5, 3, 2, 1, 1, 1, 0}, //40 items+zero. sigma=9.1
{4987, 4958, 4871, 4729, 4538, 4303, 4032, 3734, 3417, 3091, 2763, 2440, 2130, 1838, 1567, 1320, 1099, 905, 736, 591, 470, 369, 286, 219, 166, 124, 92, 67, 49, 35, 24, 17, 12, 8, 5, 4, 2, 2, 1, 1, 0}, //40 items+zero. sigma=9.2
{4936, 4907, 4823, 4686, 4500, 4272, 4009, 3718, 3409, 3090, 2769, 2452, 2147, 1858, 1590, 1344, 1124, 929, 758, 612, 489, 386, 301, 232, 177, 133, 99, 73, 53, 38, 27, 19, 13, 9, 6, 4, 3, 2, 1, 1, 0}, //40 items+zero. sigma=9.3
{4886, 4858, 4776, 4643, 4463, 4241, 3985, 3702, 3401, 3089, 2774, 2463, 2163, 1878, 1612, 1368, 1148, 952, 781, 633, 508, 403, 316, 245, 188, 142, 107, 79, 58, 42, 30, 21, 15, 10, 7, 5, 3, 2, 1, 1, 1, 0}, //41 items+zero. sigma=9.4
{4836, 4809, 4730, 4601, 4426, 4211, 3962, 3686, 3392, 3088, 2779, 2474, 2178, 1896, 1633, 1390, 1171, 975, 803, 655, 527, 420, 331, 258, 199, 152, 114, 85, 63, 46, 33, 24, 17, 12, 8, 5, 4, 2, 2, 1, 1, 0}, //41 items+zero. sigma=9.5 (equiv to row 360 on Pascal's Triangle)
{4788, 4762, 4685, 4560, 4390, 4181, 3938, 3670, 3383, 3085, 2783, 2483, 2192, 1914, 1653, 1413, 1194, 998, 826, 675, 547, 438, 347, 271, 210, 161, 122, 92, 68, 50, 36, 26, 19, 13, 9, 6, 4, 3, 2, 1, 1, 1, 0}, //42 items+zero. sigma=9.6
{4740, 4715, 4641, 4519, 4354, 4151, 3915, 3654, 3374, 3082, 2786, 2492, 2205, 1931, 1673, 1434, 1216, 1021, 847, 696, 566, 455, 362, 285, 222, 171, 131, 98, 74, 54, 40, 29, 21, 15, 10, 7, 5, 3, 2, 1, 1, 1, 0}, //42 items+zero. sigma=9.7
{4694, 4670, 4597, 4479, 4319, 4121, 3892, 3637, 3364, 3079, 2789, 2500, 2218, 1947, 1692, 1455, 1238, 1043, 869, 717, 585, 473, 378, 299, 234, 181, 139, 106, 79, 59, 43, 32, 23, 16, 11, 8, 6, 4, 3, 2, 1, 1, 0}, //42 items+zero. sigma=9.8
{4648, 4625, 4554, 4440, 4284, 4092, 3868, 3620, 3354, 3075, 2791, 2507, 2230, 1963, 1710, 1475, 1259, 1064, 890, 737, 604, 490, 394, 313, 246, 192, 148, 113, 85, 64, 47, 35, 25, 18, 13, 9, 6, 4, 3, 2, 1, 1, 1, 0}, //43 items+zero. sigma=9.9
{4604, 4581, 4512, 4401, 4250, 4063, 3845, 3603, 3343, 3071, 2792, 2514, 2241, 1978, 1728, 1495, 1280, 1085, 911, 757, 623, 508, 409, 327, 258, 202, 157, 120, 91, 69, 51, 38, 28, 20, 14, 10, 7, 5, 3, 2, 2, 1, 1, 0}, //43 items+zero. sigma=10.0 (equiv to row 399 on Pascal's Triangle)

	};

	long factor;
	long totalfactors;
	int coeffposition;

	float cropfactor=1.0;//choosing 2, makes the blurred part half the width and height

	cropfactor = overdrawfactor;

	int ystart = height / 2 - height / (2.0 * cropfactor);
	int yend = height / 2 + height / (2.0 * cropfactor);

	int xstart = width / 2 - width / (2.0 * cropfactor);
	int xend = width / 2 + width / (2.0 * cropfactor);


	//if (matrixrow > 3) {	//no blur less than this. (shouldn't need, as already returned above
		//blur horizontal
		float* secondsHoriz;

		secondsHoriz = (float*)malloc(sizeof(float) * width);
		for (int y = ystart; y < yend; y++) {
			//first copy the original line from the seconds array to a temp line
			memcpy(secondsHoriz, &pixel[y * width], width * sizeof(float));

			//then go through each pixel in the line
			for (int x = xstart; x < xend; x++) {
				//test if we need to blur this bit
				if (roughHeatmap[(y / 24) * width / 24 + (x / 24)]) {	//only blur if there's something there
					

					//since we're blurring the rough area above and below might also need blurring
					roughHeatmap[std::max((y / 24) * width / 24 + (x / 24) - width / 24,0)] = 1;
					roughHeatmap[std::min((y / 24) * width / 24 + (x / 24) + width / 24,(int)sizeof(roughHeatmap)-1)] = 1;

					//treat the actual position separately
					coeffposition = 0;
					factor = gaussianMatrix[matrixrow][coeffposition];

					//fprintf(stdout,"factor %i, rad %i, pos %i\n", factor,radius,coeffposition);

					pixel[x + y * width] = factor * secondsHoriz[x];
					totalfactors = factor;

					while (gaussianMatrix[matrixrow][coeffposition]) {//move out laterally left and rightwards
						coeffposition++;
						factor = gaussianMatrix[matrixrow][coeffposition];

						if (x >= coeffposition) {	//don't do it if the pixel offset is lower than zero (this is the same as x-coeffpos>0);
							pixel[x + y * width] += secondsHoriz[x - coeffposition] * factor;
							totalfactors += factor;
						}

						if (x + coeffposition < width) {
							pixel[x + y * width] += secondsHoriz[x + coeffposition] * factor;
							totalfactors += factor;
						}
					}

					pixel[x + y * width] /= (float)totalfactors;
				}
			}

		}
		free(secondsHoriz);

		
		//Blur vertically
		float* secondsVert;
		secondsVert = (float*)malloc(sizeof(float) * height);
		
		for (int x = xstart; x < xend; x++) {
			//copying the vertically line.
			for (int y = 0; y < height; y++) {
				secondsVert[y] = pixel[x + y * width];
			}



			for (int y =ystart; y < yend-1; y++) {
				
				if (roughHeatmap[(y / 24) * width / 24 + (x / 24)]) {	//only blur if there's something there

				//treat the actual position separately
					coeffposition = 0;
					factor = gaussianMatrix[matrixrow][coeffposition];

					//fprintf(stdout,"factor %i, rad %i, pos %i\n", factor,radius,coeffposition);

					pixel[x + y * width] = factor * secondsVert[y];
					totalfactors = factor;

					while (gaussianMatrix[matrixrow][coeffposition]) {//move out laterally left and rightwards
						coeffposition++;
						factor = gaussianMatrix[matrixrow][coeffposition];

						if (y - coeffposition >= ystart) {	//don't do it if the pixel offset is lower than zero
							pixel[x + y * width] += secondsVert[y - coeffposition] * factor;
							totalfactors += factor;
						}

						if (y + coeffposition < yend) {
							pixel[x + y * width] += secondsVert[y + coeffposition] * factor;
							totalfactors += factor;
						}
					}

					//divide at the end
					pixel[x + y * width] /= totalfactors;
				}
			}


		}
		
		//printf("\nH%i W:%i sz:%zi. xend %i, yend:%i\n", height, width, sizeof(secondsVert),xend, yend);
		free(secondsVert);
		
	//}
}

void Heatmap::MakeDirty()
{
	dirty = true;
}

void Heatmap::MakeClean()
{
	dirty = false;
}

bool Heatmap::IsDirty()
{
	if (dirty) return true;
	return false;
}

Heatmap::Heatmap()
{
	height=width = 2048;
	memset(pixel, 0, sizeof(pixel));

	//width = height = 100;

	nswe = new NSWE;
	maxPixel = 0;
	activeheatmap = 0;
	overdrawfactor = 1.0;
}

Heatmap::~Heatmap()
{
	delete nswe;
	return;
}


BackgroundInfo::BackgroundInfo()
{
	vao = 0;
	vbo = 0;

	shader = new Shader;

	worldTexture = 0;
	heatmapTexture = 0;

	worldTextureLocation = 0;
	heatmapTextureLocation = 0;
	highresTextureLocation = 0;
}

BackgroundInfo::~BackgroundInfo()
{
	delete shader;
}

MapPathInfo::MapPathInfo()
{
	vao = 0;
	vbo = 0;

	shader = new Shader;
}
MapPathInfo::~MapPathInfo()
{
	delete shader;
}

MapPointsInfo::MapPointsInfo()
{
	vao = 0;
	vbo = 0;

	shader = new Shader;
}
MapPointsInfo::~MapPointsInfo()
{
	delete shader;
}
