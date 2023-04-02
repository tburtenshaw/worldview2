#include "spectrum.h"

RGBA Spectrum_Handler::GetPointFromSpectrum(int n, float dist)
{
	if (n> spectrums.size())
			return RGBA(0,0,0,0);

	Spectrum s = spectrums[n];
    return s.GetColourAtDistance(dist);

}

RGBA Spectrum::GetColourAtDistance(float dist) const
{
    //find the two points    
    ColourPoint firstCP(colourPoints.front());
    ColourPoint secondCP(colourPoints.back());

    for (const ColourPoint& colourPoint : colourPoints) {
        if (colourPoint.distance > dist) {
            secondCP = colourPoint;
            break;
        }
        firstCP = colourPoint;
    }

    printf("%f %f\n", firstCP.distance, secondCP.distance);
    
    if (firstCP.distance >= secondCP.distance) {  //if they're the same (or swapped, which shouldn't happen)
        return firstCP.colour;
    }

    //lerp between the two points
    float p = 1.0f - (dist - firstCP.distance) / (secondCP.distance - firstCP.distance);

    RGBA returnColour{};

    returnColour.r = static_cast<unsigned char>((1.0f - p) * static_cast<float>(secondCP.colour.r) +
        p * static_cast<float>(firstCP.colour.r));
    returnColour.g = static_cast<unsigned char>((1.0f - p) * static_cast<float>(secondCP.colour.g) +
        p * static_cast<float>(firstCP.colour.g));
    returnColour.b = static_cast<unsigned char>((1.0f - p) * static_cast<float>(secondCP.colour.b) +
        p * static_cast<float>(firstCP.colour.b));
    returnColour.a = static_cast<unsigned char>((1.0f - p) * static_cast<float>(secondCP.colour.a) +
        p * static_cast<float>(firstCP.colour.a));


    return returnColour;
}
