#include <iostream>
#include <vector>
#include <string>

struct RGBA {
    unsigned char r, g, b, a;
};

class Palette {
public:
    std::string GetDisplayName();
    void SetDisplayName(std::string newName); //doesn't need to be unique

    void SetColor(int colorNumber, RGBA color);
    RGBA GetColor(int colorNumber);

    bool SetPalette((RGBA*)sourcePalette, int numberToSet);


private:
    std::string displayName;
    int id;     //this doesn't need to be unique
    int paletteType;
    std::vector <RGBA> colors;

};


class PaletteHandler {  //this will be a global with only one instance, potentially made in main.cpp or something
public:
    void LoadDefaultPalettes() {
        palette.
    }
    void AddUserPalette((RGBA*)sourcePalette, int numberToSet);

private:
    std::vector <Palette> palette;

    //have the defaults in here
    const RGBA rainbow[7]
    { {0x31,0x52,0xA6,0xff },
      {0x95,0x53,0xa7,0xff },
      {0xc1,0x42,0x55,0xff },
      {0xfe,0x62,0x3e,0xff },
      {0xe5,0xb5,0x4b,0xff },
      {0xa2,0xfd,0x56,0xff },
      {0x01,0x81,0x93,0xff }
    };

    const RGBA dawn[12]; //etc
    const RGBA ega[16]; //etc

}

void main() {
    PaletteHandler p;
    p.LoadDefaultPalettes();

    //p.AddUserPalette(//e.g. from UI somehow)
    //more code
}