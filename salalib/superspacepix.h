#pragma once

#include "salalib/spacepix.h"
#include "salalib/shapemap.h"

// (Currently) two layers of SpacePixeling, the file, and the whole lot (in SuperSpacePixel)
// (I aim to split this into a set of ShapeMaps with the version 9 layer system for each subset)
typedef SpacePixelGroup < ShapeMap> SpacePixelFile;
//typedef SpacePixelGroup<SpacePixel> SpacePixelFile;
typedef SpacePixelGroup<SpacePixelFile > SuperSpacePixel;
