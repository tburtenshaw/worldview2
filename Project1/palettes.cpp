#include <vector>
#include <string>
#include <imgui.h>
#include "header.h"
#include "palettes.h"

Palette::Palette(std::string name_, int id_, int parentId_, unsigned int flags_, int startingPoint_, std::vector<RGBA> colors_)
    : name(std::move(name_)), id(id_), parentId(parentId_), flags(flags_), startingPoint (startingPoint_), colors(std::move(colors_))
{}


Palette::Palette(std::string name_, unsigned int flags_, std::vector<RGBA> colors_)
    : name(std::move(name_)), id(++nextId), parentId(0), flags(flags_), startingPoint(0), colors(std::move(colors_))
{}


void Palette_Handler::AddUserPalette(Palette p)
    {
        palette.push_back(std::move(p));
    }

//unsigned int Palette_Handler::PaletteSize(unsigned int index)
//{
//    if (index < palette.size()) {
//        return palette[index].colors.size();
//    }
// 
//    return 0;
//}

unsigned int Palette_Handler::IndexFromId(int pId) {
    for (int i = 0; i < palette.size(); i++) {
        if (palette[i].id == pId) {
            return i;
        }

    }
    return 0;
}

std::string Palette_Handler::PaletteName(unsigned int index)
{
    return palette[index].name;
}

void Palette_Handler::RotatePaletteLeft(unsigned int currentIndex)
{
    RotatePalette(currentIndex, 1);
}

void Palette_Handler::RotatePaletteRight(unsigned int currentIndex)
{
    RotatePalette(currentIndex, -1);
}

void Palette_Handler::RotatePalette(unsigned int currentIndex, int amount)
{
    if (currentIndex > palette.size()) {
        return;
    }

    palette[currentIndex].startingPoint+=amount;
    if (palette[currentIndex].startingPoint < 0) {
        palette[currentIndex].startingPoint += palette[currentIndex].colors.size();
        return;
    }
    if (palette[currentIndex].startingPoint >= palette[currentIndex].colors.size()) {
        palette[currentIndex].startingPoint %= palette[currentIndex].colors.size();
    }

}

unsigned int Palette_Handler::SetColourImVec4(unsigned int currentIndex, int pos, ImVec4 colour)
{
    if (currentIndex >= palette.size())
        return 0;

    int m = (pos + palette[currentIndex].startingPoint) % palette[currentIndex].colors.size();
    palette[currentIndex].colors[m] = colour;

    return currentIndex; //this may be different if palette is created.
}

ImVec4 Palette_Handler::PaletteColorImVec4(unsigned int index, int c)
{
    int m = (c+ palette[index].startingPoint) % palette[index].colors.size();   //stop overflow
    return { (float)palette[index].colors[m].r / 255.0f, (float)palette[index].colors[m].g / 255.0f, (float)palette[index].colors[m].b / 255.0f, (float)palette[index].colors[m].a / 255.0f };
}

void Palette_Handler::FillShaderPalette(float arrayFourFloats[][4], unsigned int arrayRows, unsigned int currentIndex)
{
    if (currentIndex >= palette.size())
        return;
    

    if (arrayRows > 24)
        arrayRows = 24;

    for (int i = 0; i < arrayRows; i++) {
        int n = (i+palette[currentIndex].startingPoint) % palette[currentIndex].colors.size();
        arrayFourFloats[i][0] = (float)palette[currentIndex].colors[n].r / 255.0f;
        arrayFourFloats[i][1] = (float)palette[currentIndex].colors[n].g / 255.0f;
        arrayFourFloats[i][2] = (float)palette[currentIndex].colors[n].b / 255.0f;
        arrayFourFloats[i][3] = (float)palette[currentIndex].colors[n].a / 255.0f;
    }

}

unsigned int Palette_Handler::MatchingPalette(unsigned int currentIndex, unsigned int flag)
{
    for (int i = currentIndex; i < palette.size(); i++) {
        if (palette[i].flags & flag) return i;
    }
    for (int i = 0; i < currentIndex; i++) {
        if (palette[i].flags & flag) return i;
    }
    return 0;
}

unsigned int Palette_Handler::NextMatchingPalette(unsigned int currentIndex, unsigned int flag)
{
    if ((currentIndex + 1) >= palette.size())
        currentIndex = 0;
    return MatchingPalette(currentIndex+1, flag);
    
}


//RGBA * Palette_Handler::PalettePointer(unsigned int index)
//{
//    if (index < palette.size()) {
//        return palette[index].colors.data();
//    }
//            
//    return palette[0].colors.data();
//}
