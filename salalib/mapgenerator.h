// sala - a component of the depthmapX - spatial network analysis platform
// Copyright (C) 2018, Petros Koutsolampros

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

#include "salalib/axialmap.h"
#include "salalib/spacepixfile.h"
#include "salalib/axialpolygons.h"
#include "salalib/axialminimiser.h"
#include "genlib/p2dpoly.h"
#include "genlib/comm.h"

namespace MapGenerator {

AxialPolygons getAxialPolygons(
        const std::vector<SpacePixelFile> &drawingLayers);

AxialVertex getPolygonsSeedVertex(
        const AxialPolygons& polygons,
        const Point2f &seed);

std::tuple<prefvec<Line>, prefvec<pvecint>, prefvec<PolyConnector>, pqvector<RadialLine>> makeAxialLines(Communicator *comm,
        AxialPolygons &polygons,
        const AxialVertex& seedVertex);

ShapeGraph makeAllLineMap(
        const prefvec<Line> &axiallines,
        const prefvec<pvecint> &preaxialdata,
        const AxialPolygons &polygons);

std::tuple<std::map<RadialKey,pvecint>, std::map<int,pvecint>> makeDivisions(
        Communicator *comm,
        const ShapeGraph &allLineMap,
        const prefvec<PolyConnector>& polyconnections,
        const pqvector<RadialLine>& radiallines);

std::tuple<std::map<int, pvecint>, std::map<RadialKey, pvecint> > prepFewestLineMaps(Communicator *comm,
        const ShapeGraph &allLineMap,
        const prefvec<PolyConnector>& poly_connections,
        const pqvector<RadialLine>& radial_lines,
        std::map<RadialKey, RadialSegment> &radialsegs);

std::map<RadialKey,RadialSegment> getRadialSegs(
        const pqvector<RadialLine>& radial_lines);

std::tuple<prefvec<pvecint>, int *> getVertexConns(
        const ShapeGraph &allLineMap);

ShapeGraph makeFewestSubsetsLineMap(
        const ShapeGraph& allLineMap,
        const QtRegion& polygonsRegion,
        const AxialMinimiser& minimiser);

ShapeGraph makeFewestMinimalLineMap(
        const ShapeGraph& allLineMap,
        const QtRegion& polygonsRegion,
        const AxialMinimiser &minimiser);

std::tuple<std::unique_ptr<ShapeGraph>,
           std::unique_ptr<ShapeGraph>,
           std::unique_ptr<ShapeGraph> > makeAllFewestLineMaps(
        Communicator *comm,
        const std::vector<SpacePixelFile> &drawingLayers,
        const Point2f& seed,
        bool all_line,
        bool fewest_line_subsets,
        bool fewest_line_minimal);

}
