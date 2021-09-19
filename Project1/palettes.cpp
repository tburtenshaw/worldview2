#include <vector>
#include <string>
#include "header.h"
#include "palettes.h"

Palette::Palette(std::string name_, int id_, int parentId_, unsigned int type_, int startingPoint_, std::vector<RGBA> colors_)
    : name(std::move(name_)), id(id_), parentId(parentId_), type(type_), startingPoint (startingPoint_), colors(std::move(colors_))
{}


Palette::Palette(std::string name_, std::vector<RGBA> colors_)
    : name(std::move(name_)), id(++nextId), parentId(0), type(paletteType::DEFAULT), startingPoint(0), colors(std::move(colors_))
{}


void Palette_Handler::AddUserPalette(Palette p)
    {
        palette.push_back(std::move(p));
    }

unsigned int Palette_Handler::PaletteSize(int pId)
{
    for (int i = 0; i < palette.size(); i++) {
        if (palette[i].id == pId) {
            return palette[i].colors.size();
        }

    }
    
    return 0;
}

RGBA * Palette_Handler::PalettePointer(int pId)
{
    for (int i = 0; i < palette.size(); i++) {
        if (palette[i].id == pId) {
            return palette[i].colors.data();
        }
    }
    
    return palette[0].colors.data();
}
