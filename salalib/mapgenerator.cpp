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

#include "salalib/mapgenerator.h"
#include "salalib/tolerances.h"
#include "genlib/exceptions.h"

AxialPolygons MapGenerator::getAxialPolygons(const std::vector<SpacePixelFile> &drawingLayers) {

    // this has a nasty habit of crashing if reused...
    // reset everything at the top level, including any existing all-line map:
    AxialPolygons polygons;

    // starting off... finding a polygon...
    // for ease, I'm just going to make a construction line set from all the visible lines...

    QtRegion region;

    std::vector<Line> lines;

    // add all visible layers to the set of polygon lines...
    for (const auto& pixelGroup: drawingLayers) {
       for (const auto& pixel: pixelGroup.m_spacePixels) {
          if (pixel.isShown()) {
             if (region.atZero()) {
                region = pixel.getRegion();
             }
             else {
                region = runion(region, pixel.getRegion());
             }
             std::vector<SimpleLine> newLines = pixel.getAllShapesAsLines();
             for (const auto& line: newLines) {
                lines.push_back(Line(line.start(), line.end()));
             }
          }
       }
    }

    region.grow(1.30);
    polygons.init(lines, region);
    polygons.m_handled_list.clear();
    return(polygons);
}
AxialVertex MapGenerator::getPolygonsSeedVertex(const AxialPolygons &polygons, const Point2f& seed) {
    // find a corner visible from the seed:
    AxialVertexKey seedvertex = polygons.seedVertex( seed );

    if (seedvertex == NoVertex) {
       // oops... can't find a visible vertex
        throw depthmapX::RuntimeException("No visible vertices found");
    }

    // okay, we've got as far as finding a seed corner, now the real fun begins...
    // test outwards from corner, add other corners to
    // test set...
    // also poly_connections used in fewest line axial map construction:


    AxialVertex vertex = polygons.makeVertex(seedvertex, seed);
    if (!vertex.m_initialised) {
       // oops... can't init for some reason
       throw depthmapX::RuntimeException("Failed to initialise axial vertices");
    }

    return(vertex);
}

std::tuple<prefvec<Line>, prefvec<pvecint>, prefvec<PolyConnector>, pqvector<RadialLine>> MapGenerator::makeAxialLines(
        Communicator *comm,
        AxialPolygons& polygons,
        const AxialVertex& seedVertex) {
    if (comm) {
       comm->CommPostMessage( Communicator::NUM_STEPS, 3 );
       comm->CommPostMessage( Communicator::CURRENT_STEP, 1 );
    }

    QtRegion region = polygons.getRegion();


    time_t atime = 0;
    int count = 0;
    if (comm) {
       qtimer( atime, 0 );
       comm->CommPostMessage( Communicator::CURRENT_STEP, 2 );
       comm->CommPostMessage( Communicator::NUM_RECORDS, int(polygons.m_vertex_possibles.size()) );
    }

    prefvec<Line> axiallines;
    prefvec<pvecint> preaxialdata;

    prefvec<PolyConnector> poly_connections;
    pqvector<RadialLine> radial_lines;

    pqvector<AxialVertex> openvertices;
    openvertices.add(seedVertex);
    while (openvertices.size()) {
       polygons.makeAxialLines(openvertices, axiallines, preaxialdata, poly_connections, radial_lines);
       count++;
       //
       if (comm) {
          if (qtimer( atime, 500 )) {
             if (comm->IsCancelled()) {
                throw Communicator::CancelledException();
             }
             comm->CommPostMessage( Communicator::CURRENT_RECORD, count );
          }
       }

    }

    if (comm) {
       comm->CommPostMessage( Communicator::CURRENT_STEP, 3 );
       comm->CommPostMessage( Communicator::CURRENT_RECORD, 0 );
    }

    // cut out duplicates:
    int removed = 0;  // for testing purposes
    for (size_t j = 0; j < axiallines.size(); j++) {
       for (size_t k = axiallines.size() - 1; k > j; k--) {
          double maxdim = __max(region.width(),region.height());
          if (approxeq(axiallines[j].start(), axiallines[k].start(), maxdim * TOLERANCE_B) &&
                  approxeq(axiallines[j].end(), axiallines[k].end(), maxdim * TOLERANCE_B)) {
             for (size_t m = 0; m < preaxialdata[k].size(); m++) {
                preaxialdata[j].add(preaxialdata[k][m]);
             }
             preaxialdata.remove_at(k);
             axiallines.remove_at(k);
             removed++;
          }
       }
    }

    region.grow(0.99); // <- this paired with crop code below to prevent error
    for (size_t k = 0; k < axiallines.size(); k++) {
       axiallines[k].crop(region); // <- should be cropped anyway, but causing an error
    }

    return std::make_tuple( axiallines, preaxialdata, poly_connections, radial_lines);
}

ShapeGraph MapGenerator::makeAllLineMap(const prefvec<Line>& axiallines,
                                        const prefvec<pvecint>& preaxialdata,
                                        const AxialPolygons& polygons) {
    ShapeGraph allLineMap("All-Line Map", ShapeMap::ALLLINEMAP);
    allLineMap.init(int(axiallines.size()), polygons.getRegion());  // used to be double density here
    allLineMap.initialiseAttributesAxial();
    for (size_t k = 0; k < axiallines.size(); k++) {
       allLineMap.makeLineShape(axiallines[k]);
    }

    // n.b. make connections also initialises attributes
    // -> don't know what this was for: alllinemap.sortBins(m_poly_connections);
    allLineMap.makeConnections(preaxialdata);

    allLineMap.setKeyVertexCount(int(polygons.m_vertex_possibles.size()));

    return allLineMap;
}

std::tuple<std::map<RadialKey,pvecint>, std::map<int,pvecint>> MapGenerator::makeDivisions(
        Communicator *comm,
        const ShapeGraph &allLineMap,
        const prefvec<PolyConnector>& polyconnections,
        const pqvector<RadialLine>& radial_lines)
{
   time_t atime = 0;
   if (comm) {
      qtimer( atime, 0 );
      comm->CommPostMessage( Communicator::NUM_RECORDS, int(polyconnections.size()) );
   }

   // make one rld for each radial line...
   std::map<RadialKey,pvecint> radialdivisions;
   for (size_t i = 0; i < radial_lines.size(); i++) {
      radialdivisions.insert(std::make_pair( (RadialKey) radial_lines[i], pvecint() ));
   }

   // also, a list of radial lines cut by each axial line
   std::map<int,pvecint> ax_radial_cuts;
   for (auto shape: allLineMap.getAllShapes()) {
      ax_radial_cuts.insert(std::make_pair(shape.first, pvecint()));
   }

   for (size_t i = 0; i < polyconnections.size(); i++) {
      PixelRefVector pixels = allLineMap.pixelateLine(polyconnections[i].line);
      pvecint testedshapes;
      auto connIter = radialdivisions.find(polyconnections[i].key);
      size_t connindex = size_t(std::distance(radialdivisions.begin(), connIter));
      double tolerance = sqrt(TOLERANCE_A);// * polyconnections[i].line.length();
      for (size_t j = 0; j < pixels.size(); j++) {
         PixelRef pix = pixels[j];
         std::vector<ShapeRef> &shapes = allLineMap.getPixelShapes()[size_t(pix.x + pix.y*allLineMap.getCols())];
         for (const ShapeRef& shape: shapes) {
            if (testedshapes.searchindex(int(shape.m_shape_ref)) != paftl::npos) {
               continue;
            }
            testedshapes.add(int(shape.m_shape_ref));
            const Line& line = allLineMap.getAllShapes().find(int(shape.m_shape_ref))->second.getLine();
            //
            if (intersect_region(line, polyconnections[i].line, tolerance * line.length()) ) {
               switch ( intersect_line_distinguish(line, polyconnections[i].line, tolerance * line.length()) ) {
               case 0:
                  break;
               case 2:
                  {
                     size_t index = size_t(depthmapX::findIndexFromKey(ax_radial_cuts, int(shape.m_shape_ref)));
                     if (index != shape.m_shape_ref) {
                        throw 1; // for the code to work later this can't be true!
                     }
                     ax_radial_cuts[int(index)].add(int(connindex));
                     connIter->second.add(int(shape.m_shape_ref));
                  }
                  break;
               case 1:
                  {
                     size_t index = size_t(depthmapX::findIndexFromKey(ax_radial_cuts, int(shape.m_shape_ref)));
                     if (index != shape.m_shape_ref) {
                        throw 1; // for the code to work later this can't be true!
                     }
                     //
                     // this makes sure actually crosses between the line and the openspace properly
                     if (radial_lines[connindex].cuts(line)) {
                        ax_radial_cuts[int(index)].add(int(connindex));
                        connIter->second.add(int(shape.m_shape_ref));
                     }
                  }
                  break;
               default:
                  break;
               }
            }
         }
      }
      if (comm) {
         if (qtimer( atime, 500 )) {
            if (comm->IsCancelled()) {
               throw Communicator::CancelledException();
            }
            comm->CommPostMessage( Communicator::CURRENT_RECORD, int(i) );
         }
      }
   }
   return std::make_tuple(radialdivisions, ax_radial_cuts);
}

std::tuple<std::map<int,pvecint>, std::map<RadialKey,pvecint>> MapGenerator::prepFewestLineMaps(
        Communicator *comm,
        const ShapeGraph& allLineMap,
        const prefvec<PolyConnector>& poly_connections,
        const pqvector<RadialLine> &radial_lines,
        std::map<RadialKey,RadialSegment>& radialsegs) {
    if (comm) {
       comm->CommPostMessage( Communicator::NUM_STEPS, 2 );
       comm->CommPostMessage( Communicator::CURRENT_STEP, 1 );
    }
    pafsrand((unsigned int)(time(nullptr)));

    std::map<RadialKey,pvecint> radialdivisions;
    std::map<int,pvecint> ax_radial_cuts;

    // make divisions -- this is the slow part and the comm updates
    std::tie(radialdivisions, ax_radial_cuts) = makeDivisions(comm, allLineMap, poly_connections, radial_lines);

    // the slow part is over, we're into the final straight... reset the current record flag:
    if (comm) {
       comm->CommPostMessage( Communicator::CURRENT_STEP, 2 );
       comm->CommPostMessage( Communicator::CURRENT_RECORD, 0 );
    }

    // a little further setting up is still required...

    std::map<int,pvecint> ax_seg_cuts;
    for (auto shape: allLineMap.getAllShapes()) {
        ax_seg_cuts.insert(std::make_pair(shape.first, pvecint()));
    }
    // and segment divisors from the axial lines...
    // TODO: (CS) Restructure this to get rid of all those brittle parallel data structure
    auto axIter = ax_radial_cuts.begin();
    auto axSeg = ax_seg_cuts.begin();
    for (size_t i = 0; i < allLineMap.getAllShapes().size(); i++) {
       for (size_t j = 1; j < axIter->second.size(); j++) {
          // note similarity to loop above
          RadialKey rk_end = radial_lines[size_t(axIter->second[j])];
          RadialKey rk_start = radial_lines[size_t(axIter->second[j-1])];
          if (rk_start.vertex == rk_end.vertex) {
             auto radialSegIter = radialsegs.find(rk_end);
             if (radialSegIter != radialsegs.end() && rk_start == radialSegIter->second.radial_b) {
                radialSegIter->second.add(axIter->first);
                axSeg->second.add(int(std::distance(radialsegs.begin(), radialSegIter)));
             }
          }
       }
       axIter++;
       axSeg++;
    }
    return std::make_tuple(ax_seg_cuts, radialdivisions);
}

std::map<RadialKey,RadialSegment> MapGenerator::getRadialSegs(const pqvector<RadialLine>& radial_lines) {
    // now make radial segments from the radial lines... (note, start at 1)
    std::map<RadialKey,RadialSegment> radialsegs;
    for (size_t i = 1; i < radial_lines.size(); i++) {
        if (radial_lines[i].vertex == radial_lines[i-1].vertex) {
            if (radial_lines[i].ang == radial_lines[i-1].ang) {
                continue;
            }
            else {
                RadialLine prevRadialLine = radial_lines[i-1];
                radialsegs.insert(std::make_pair( (RadialKey) radial_lines[i],
                                                  (RadialSegment) prevRadialLine));
            }
        }
    }
    return radialsegs;
}

std::tuple<prefvec<pvecint>, int*> MapGenerator::getVertexConns(const ShapeGraph& allLineMap) {
    // and a little more setting up: key vertex relationships
    prefvec<pvecint> keyvertexconns;
    int * keyvertexcounts =  new int [allLineMap.getKeyVertexCount()];
    for (int x = 0; x < allLineMap.getKeyVertexCount(); x++) {
       keyvertexcounts[x] = 0;
    }
    // this sets up a two step relationship: looks for the key vertices for all lines connected to you
    auto& connectors = allLineMap.getConnections();
    auto& keyvertices = allLineMap.getKeyVertices();
    for (size_t y = 0; y < connectors.size(); y++) {
       keyvertexconns.push_back(pvecint());
       const Connector& axa = connectors[y];
       for (size_t z = 0; z < axa.m_connections.size(); z++) {
          const pvecint& axb = keyvertices[size_t(axa.m_connections[z])];
          for (size_t zz = 0; zz < axb.size(); zz++) {
             if (keyvertexconns[y].searchindex(axb[zz]) == paftl::npos) {
                keyvertexconns[y].add(axb[zz],paftl::ADD_HERE);
                keyvertexcounts[axb[zz]] += 1;
             }
          }
       }
    }
    return std::make_tuple(keyvertexconns, keyvertexcounts);
}

ShapeGraph MapGenerator::makeFewestSubsetsLineMap(const ShapeGraph& allLineMap,
                                                  const QtRegion& polygonsRegion,
                                                  const AxialMinimiser& minimiser) {
    prefvec<Line> lines_s;

    // make new lines here (assumes line map has only lines)
    int k = -1;
    auto& allLineMapShapes = allLineMap.getAllShapes();
    for (auto& shape: allLineMapShapes) {
       k++;
       if (!minimiser.removed(k)) {
          lines_s.push_back( shape.second.getLine() );
       }
    }

    ShapeGraph fewestlinemap_subsets("Fewest-Line Map (Subsets)", ShapeMap::AXIALMAP);
    fewestlinemap_subsets.clearAll();
    fewestlinemap_subsets.init(int(lines_s.size()), polygonsRegion);

    fewestlinemap_subsets.initialiseAttributesAxial();
    for (size_t k = 0; k < lines_s.size(); k++) {
       fewestlinemap_subsets.makeLineShape(lines_s[k]);
    }
    fewestlinemap_subsets.makeConnections();

    return(fewestlinemap_subsets);
}

ShapeGraph MapGenerator::makeFewestMinimalLineMap(const ShapeGraph &allLineMap,
                                                  const QtRegion &polygonsRegion,
                                                  const AxialMinimiser& minimiser) {



    prefvec<Line> lines_m;

    // make new lines here (assumes line map has only lines
    for (int k = 0; k < int(allLineMap.getAllShapes().size()); k++) {
       if (!minimiser.removed(k)) {
          lines_m.push_back( depthmapX::getMapAtIndex(allLineMap.getAllShapes(), k)->second.getLine() );
       }
    }

    ShapeGraph fewestlinemap_minimal("Fewest-Line Map (Minimal)", ShapeMap::AXIALMAP);
    fewestlinemap_minimal.clearAll();
    fewestlinemap_minimal.init(int(lines_m.size()), polygonsRegion); // used to have a '2' for double pixel density

    fewestlinemap_minimal.initialiseAttributesAxial();
    for (size_t k = 0; k < lines_m.size(); k++) {
       fewestlinemap_minimal.makeLineShape(lines_m[k]);
    }
    fewestlinemap_minimal.makeConnections();

    return fewestlinemap_minimal;

}

std::tuple<std::unique_ptr<ShapeGraph>,
           std::unique_ptr<ShapeGraph>,
           std::unique_ptr<ShapeGraph> > MapGenerator::makeAllFewestLineMaps(
        Communicator *comm,
        const std::vector<SpacePixelFile> &drawingLayers,
        const Point2f& seed,
        bool all_line,
        bool fewest_line_subsets,
        bool fewest_line_minimal) {

    if(!all_line && !fewest_line_subsets && !fewest_line_minimal) {
        throw depthmapX::RuntimeException("No All/Fewest line map type given");
    }

    AxialPolygons polygons = getAxialPolygons(drawingLayers);
    AxialVertex seedVertex = getPolygonsSeedVertex(polygons, seed);

    prefvec<Line> axiallines;
    prefvec<pvecint> preaxialdata;
    prefvec<PolyConnector> poly_connections;
    pqvector<RadialLine> radial_lines;
    std::tie(axiallines, preaxialdata, poly_connections, radial_lines) =
            makeAxialLines(comm, polygons, seedVertex);

    std::unique_ptr<ShapeGraph> allLineMap = std::unique_ptr<ShapeGraph>(
                new ShapeGraph(std::move(makeAllLineMap(axiallines, preaxialdata, polygons))));

    std::unique_ptr<ShapeGraph> fewestlinemap_subsets(nullptr);
    std::unique_ptr<ShapeGraph> fewestlinemap_minimal(nullptr);

    if(fewest_line_subsets || fewest_line_minimal) {
        std::map<RadialKey,RadialSegment> radialsegs = getRadialSegs(radial_lines);
        std::map<int,pvecint> ax_seg_cuts;
        std::map<RadialKey,pvecint> radialdivisions;
        std::tie(ax_seg_cuts, radialdivisions) = prepFewestLineMaps(comm, *allLineMap, poly_connections,
                                                                    radial_lines, radialsegs);

        prefvec<pvecint> keyvertexconns;
        int *keyvertexcounts = nullptr;
        std::tie(keyvertexconns, keyvertexcounts) = getVertexConns(*allLineMap);

        // ok, after this fairly tedious set up, we are ready to go...
        // note axradialcuts aren't required anymore...

        AxialMinimiser minimiser(*allLineMap, int(ax_seg_cuts.size()), int(radialsegs.size()));

        minimiser.removeSubsets(ax_seg_cuts, radialsegs, radialdivisions, radial_lines, keyvertexconns, keyvertexcounts);

        if(fewest_line_subsets) {
            fewestlinemap_subsets = std::unique_ptr<ShapeGraph>(
                        new ShapeGraph(std::move(makeFewestSubsetsLineMap(*allLineMap, polygons.getRegion(), minimiser))));
        }

        if(fewest_line_minimal) {
            minimiser.fewestLongest(ax_seg_cuts, radialsegs, radialdivisions, radial_lines, keyvertexconns, keyvertexcounts);
            fewestlinemap_minimal = std::unique_ptr<ShapeGraph>(
                        new ShapeGraph(std::move(makeFewestMinimalLineMap(*allLineMap, polygons.getRegion(), minimiser))));
        }
        delete [] keyvertexcounts;
    }

    return(std::make_tuple(std::move(allLineMap), std::move(fewestlinemap_subsets), std::move(fewestlinemap_minimal)));
}
