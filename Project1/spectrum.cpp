#include "spectrum.h"
#include <iostream>

void Spectrum_Handler::AddSpectrum(Spectrum s)
{
    spectrums.push_back(s);
}

RGBA Spectrum_Handler::GetPointFromSpectrum(int n, float dist)
{
	if (n> spectrums.size())
			return RGBA(0,0,0,0);

	Spectrum s = spectrums[n];
    return s.GetColourAtDistance(dist);

}

size_t Spectrum_Handler::GetNumberOfSpectrums()
{
    return spectrums.size();
}

std::string Spectrum_Handler::GetSpectrumName(int n)
{
    if (n > spectrums.size()) return "Out of bounds";
    return spectrums[n].GetSpectrumName();
}

int Spectrum_Handler::GetSpectrumSize(int n)
{
    if (n > spectrums.size()) return 0;
    return spectrums[n].GetSpectrumSize();
}

std::vector<std::string> Spectrum_Handler::ListSpectrums()
{
    std::vector<std::string> spectrumNames;

    for (const auto& spectrum : spectrums) {
        std::string name = spectrum.GetSpectrumName();
        spectrumNames.push_back(name);
    }
    return spectrumNames;
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

   
    if (firstCP.distance >= secondCP.distance) {  //if they're the same (or swapped, which shouldn't happen)
        return firstCP.colour;
    }

    //lerp between the two points
    float p = (dist - firstCP.distance) / (secondCP.distance - firstCP.distance);

    /*

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
    */
    return RGBA::Lerp(firstCP.colour, secondCP.colour, p);

}

std::string Spectrum::GetSpectrumName() const
{
    return name;
}

int Spectrum::GetSpectrumSize() const
{
    return colourPoints.size();
}

void Spectrum::FillColourPointsFromGradient(const std::vector<RGBA>& gradient, int maxPoints)
{
    int n = gradient.size();
    if (n == 0) return;
    
    colourPoints.clear();
    colourPoints.push_back({ gradient[0],0.0f });

    if (n == 1) return;
    int numPoints = std::min(n, maxPoints);
    float stepSize = 1.0f / (numPoints - 1);
    for (int i = 1; i < numPoints - 1; i++) {
        float t = i * stepSize;
        int index = static_cast<int>(t * (n - 1));
        RGBA c = RGBA::Lerp(gradient[index], gradient[index + 1], (t - index * stepSize) / stepSize);
        
        colourPoints.push_back({ c,t});
    }
    
    colourPoints.push_back({ gradient[n-1],1.0f });

}

float Spectrum::ErrorFromGradient(const std::vector<RGBA>& gradient)
{
    int n = gradient.size();

    float error = 0.0f;

    for (int i = 0; i < n; i++) {
        float gradientPos = static_cast<float>(i) / static_cast<float>(n);
        
        RGBA calcColor = GetColourAtDistance(gradientPos);
        error += RGBA::EuclideanRGBADistanceSquared(calcColor, gradient[i]);
    }
    
    return error/static_cast<float>(n);
}

void Spectrum::FineTuneColourValues(const std::vector<RGBA>& gradient)
{
    

    float bestErr = ErrorFromGradient(gradient);
    float prevErr = 100.f;
    //printf("BestErr before: %f\n", bestErr);
    while (bestErr < prevErr) {
        for (int p = 0; p < colourPoints.size(); p++) {
            prevErr = bestErr;


            float currentErr = bestErr;
            //nudge red up then down
            colourPoints[p].colour.r++;
            currentErr = ErrorFromGradient(gradient);
            if (currentErr > bestErr) { colourPoints[p].colour.r--; } //move it back
            else bestErr = currentErr;

            colourPoints[p].colour.r--;
            currentErr = ErrorFromGradient(gradient);
            if (currentErr > bestErr) { colourPoints[p].colour.r++; } //move it back
            else bestErr = currentErr;

            //green
            colourPoints[p].colour.g++;
            currentErr = ErrorFromGradient(gradient);
            if (currentErr > bestErr) { colourPoints[p].colour.g--; } //move it back
            else bestErr = currentErr;

            colourPoints[p].colour.g--;
            currentErr = ErrorFromGradient(gradient);
            if (currentErr > bestErr) { colourPoints[p].colour.g++; } //move it back
            else bestErr = currentErr;


            //blue
            colourPoints[p].colour.g++;
            currentErr = ErrorFromGradient(gradient);
            if (currentErr > bestErr) { colourPoints[p].colour.g--; } //move it back
            else bestErr = currentErr;

            colourPoints[p].colour.g--;
            currentErr = ErrorFromGradient(gradient);
            if (currentErr > bestErr) { colourPoints[p].colour.g++; } //move it back
            else bestErr = currentErr;

            //alpha
            if (p > 0 && p < colourPoints.size() - 1) { //don't move first an last alpha
                colourPoints[p].colour.a++;
                currentErr = ErrorFromGradient(gradient);
                if (currentErr > bestErr) { colourPoints[p].colour.a-=2;
                    currentErr = ErrorFromGradient(gradient);
                    if (currentErr > bestErr) { colourPoints[p].colour.a++; }
                    else bestErr = currentErr;
                    } 
                else bestErr = currentErr;

           }

        }
        //printf("BestErr after: %f\n", bestErr);
    }


}

Spectrum Spectrum::CreateOptimalEvenSpacedSpectrumFromGradient(std::string name, const std::vector<RGBA>& gradient, int maxPoints, float relTarget, float absTarget)
{

    float prevError = 10.0f;
    bool relativeTargetMet = false;
    bool absoluteTargetMet = false;

    Spectrum testSpectrum = { name,{} };

    for (int n = 3; n < maxPoints; n++) {
        testSpectrum.FillColourPointsFromGradient(gradient, n);
        //Spectrum_Handler::AddSpectrum(plasma);
        testSpectrum.FineTuneColourValues(gradient);
        float e = testSpectrum.ErrorFromGradient(gradient);

        std::cout << name << n << ":" << e << " (diff from prev:" << (((prevError - e) / prevError)) << ")" << std::endl;
        if ((prevError - e) / prevError < relTarget) { relativeTargetMet = true; }
        if (e < absTarget) { absoluteTargetMet = true; }

        if (absoluteTargetMet && relativeTargetMet) {
            if (prevError < e) {    //if the previous one was actually better.
                testSpectrum.FillColourPointsFromGradient(gradient, n - 1);
                std::cout << n - 1 <<std::endl;
                testSpectrum.FineTuneColourValues(gradient);

                return testSpectrum;
            }
            testSpectrum.FineTuneColourValues(gradient);
            return testSpectrum;
        }

        prevError = e;
    }

    testSpectrum.FineTuneColourValues(gradient);
    return testSpectrum;
}

