
#include "guiatlas.h"

GuiAtlas atlas = { 512,512 };

AtlasEntry::AtlasEntry(GuiAtlas &a, const std::string n, RectDimension s) {
	name = n;
	size = s;
	uv0 = { 0.0,0.0 };
	uv1 = { 1.0,1.0 };
}


void GuiAtlas::FillAtlas() {
	//entry[Icon::open] = AtlasEntry(atlas, "Open", { 32,32 });
	entry.emplace(Icon::open, AtlasEntry(atlas, "Open", { 32,32 }));
}

