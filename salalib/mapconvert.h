#pragma once

#include "salalib/axialmap.h"
#include "salalib/spacepixfile.h"
#include "salalib/shapemap.h"
#include "genlib/comm.h"

namespace MapConvert {
ShapeGraph &convertDrawingToAxial(Communicator *comm, const std::string& name, std::vector<SpacePixelFile> &drawingFiles);
int convertDataToAxial(Communicator *comm, const std::string& name, ShapeMap& shapemap, bool copydata = false);
int convertDrawingToConvex(Communicator *comm, const std::string& name, std::vector<SpacePixelFile> &drawingFiles);
int convertDataToConvex(Communicator *comm, const std::string& name, ShapeMap& shapemap, bool copydata = false);
int convertDrawingToSegment(Communicator *comm, const std::string& name, std::vector<SpacePixelFile> &drawingFiles);
int convertDataToSegment(Communicator *comm, const std::string& name, ShapeMap& shapemap, bool copydata = false);
int convertAxialToSegment(Communicator *comm, const std::string& name, bool keeporiginal = true, bool pushvalues = false, double stubremoval = 0.0);
}
