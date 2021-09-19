#pragma once
#include "header.h"

class Palette_Handler;

class Palette
{
    friend class Palette_Handler;

public:
    enum paletteType :unsigned int {
        NONE = 0,
        DEFAULT = 1 << 0,
        MONTH = 1 << 1,
        WEEKDAY = 1 << 2,
        HOUR = 1 << 3,
        HIDDEN = 1<<4
    };


    Palette(std::string name_, int id_, int parentId_, unsigned int type_, int startingPoint_, std::vector<RGBA> colors_);
    Palette(std::string name_, std::vector<RGBA> colors_);


private:
    int id;
    int parentId;
    std::string name;
    unsigned int type;
    int startingPoint;
    std::vector <RGBA> colors;

    static inline int nextId=0;

};

class Palette_Handler
{
public:
    static void AddUserPalette(Palette p);
    static RGBA* PalettePointer(int pId);
    static unsigned int PaletteSize(int pId);

private:
    static inline std::vector <Palette> palette
    {
        {
            "Rainbow", 1,1, Palette::paletteType::DEFAULT| Palette::paletteType::WEEKDAY,0,
            { {0x31,0x52,0xA6,0xff },
                {0x95,0x53,0xa7,0xff },
                {0xc1,0x42,0x55,0xff },
                {0xfe,0x62,0x3e,0xff },
                {0xe5,0xb5,0x4b,0xff },
                {0xa2,0xfd,0x56,0xff },
                {0x01,0x81,0x93,0xff }
            }
        },
        {
            "dawn",2,2, Palette::paletteType::DEFAULT,0,
            { {0x31, 0x52, 0xA6, 0xff } }
        },
        {
            "ega",3,3, Palette::paletteType::DEFAULT,0,
            { {0x31,0x52,0xA6,0xff } ,{0x31,0x52,0xA6,0xff } }
        }
    };
};