// sala - a component of the depthmapX - spatial network analysis platform
// Copyright (C) 2011-2012, Tasos Varoudis

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include "genlib/comm.h"
#include "salalib/axialmap.h"
#include "salalib/shapemap.h"
#include "salalib/spacepixfile.h"

namespace MapConverter {

    std::unique_ptr<ShapeGraph> convertDrawingToAxial(Communicator *comm, const std::string &name,
                                                      const std::vector<SpacePixelFile> &drawingFiles);
    std::unique_ptr<ShapeGraph> convertDataToAxial(Communicator *comm, const std::string &name, ShapeMap &shapemap,
                                                   bool copydata = false);
    std::unique_ptr<ShapeGraph> convertDrawingToConvex(Communicator *, const std::string &name,
                                                       const std::vector<SpacePixelFile> &drawingFiles);
    std::unique_ptr<ShapeGraph> convertDataToConvex(Communicator *, const std::string &name, ShapeMap &shapemap,
                                                    bool copydata = false);
    std::unique_ptr<ShapeGraph> convertDrawingToSegment(Communicator *comm, const std::string &name,
                                                        const std::vector<SpacePixelFile> &drawingFiles);
    std::unique_ptr<ShapeGraph> convertDataToSegment(Communicator *comm, const std::string &name, ShapeMap &shapemap,
                                                     bool copydata = false);
    std::unique_ptr<ShapeGraph> convertAxialToSegment(Communicator *, ShapeGraph &axialMap, const std::string &name,
                                                      bool keeporiginal = true, bool pushvalues = false,
                                                      double stubremoval = 0.0);

} // namespace MapConverter
