#include "mapgenerator.h"

AxialMinimiser::AxialMinimiser(const ShapeGraph& alllinemap, int no_of_axsegcuts, int no_of_radialsegs)
{
   m_alllinemap = (ShapeGraph *) &alllinemap;

   m_vps = new ValueTriplet[no_of_axsegcuts];
   m_removed = new bool [no_of_axsegcuts];
   m_affected = new bool [no_of_axsegcuts];
   m_vital = new bool [no_of_axsegcuts];
   m_radialsegcounts = new int [no_of_radialsegs];
}

AxialMinimiser::~AxialMinimiser()
{
   delete [] m_vital;
   delete [] m_affected;
   delete [] m_radialsegcounts;
   delete [] m_vps;
   delete [] m_removed;
}

AllLineMap MapGenerator::makeAllLineMap(Communicator *comm, std::vector<SpacePixelFile> &drawingLayers, const Point2f& seed)
{
   if (comm) {
      comm->CommPostMessage( Communicator::NUM_STEPS, 3 );
      comm->CommPostMessage( Communicator::CURRENT_STEP, 1 );
   }

   AllLineMap alllinemap("All-Line Map");

   // For all line map work:
   AxialPolygons m_polygons;
//   prefvec<PolyConnector> m_poly_connections;
//   pqvector<RadialLine> m_radial_lines;
   // this has a nasty habit of crashing if reused...
   // reset everything at the top level, including any existing all-line map:
   m_polygons.clear();
   alllinemap.m_poly_connections.clear();
   alllinemap.m_radial_lines.clear();

   // starting off... finding a polygon...
   // for ease, I'm just going to make a construction line set from all the visible lines...

   QtRegion region;
   int size = 0;

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
   m_polygons.init(lines, region);
   m_polygons.m_handled_list.clear();

   // find a corner visible from the seed:
   AxialVertexKey seedvertex = m_polygons.seedVertex( seed );

   if (seedvertex == NoVertex) {
      // oops... can't find a visible vertex
      throw RuntimeException("No visible vertex found");
   }

   // okay, we've got as far as finding a seed corner, now the real fun begins...
   // test outwards from corner, add other corners to
   // test set...
   prefvec<Line> axiallines;
   prefvec<pvecint> preaxialdata;
   // also poly_connections used in fewest line axial map construction:
   alllinemap.m_poly_connections.clear();
   alllinemap.m_radial_lines.clear();

   AxialVertex vertex = m_polygons.makeVertex(seedvertex, seed);
   if (!vertex.m_initialised) {
      // oops... can't init for some reason
      throw depthmapX::RuntimeException("Failure to initialise");
   }


   time_t atime = 0;
   int count = 0;
   if (comm) {
      qtimer( atime, 0 );
      comm->CommPostMessage( Communicator::CURRENT_STEP, 2 );
      comm->CommPostMessage( Communicator::NUM_RECORDS, m_polygons.m_vertex_possibles.size() );
   }

   pqvector<AxialVertex> openvertices;
   openvertices.add(vertex);
   while (openvertices.size()) {
      m_polygons.makeAxialLines(openvertices, axiallines, preaxialdata, alllinemap.m_poly_connections, alllinemap.m_radial_lines);
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
         if (approxeq(axiallines[j].start(), axiallines[k].start(), maxdim * TOLERANCE_B) && approxeq(axiallines[j].end(), axiallines[k].end(), maxdim * TOLERANCE_B)) {
            for (size_t m = 0; m < preaxialdata[k].size(); m++) {
               preaxialdata[j].add(preaxialdata[k][m]);
            }
            preaxialdata.remove_at(k);
            axiallines.remove_at(k);
            removed++;
         }
      }
   }

   // create the all line map layer...

   region.grow(0.99); // <- this paired with crop code below to prevent error
   alllinemap.init(axiallines.size(),m_polygons.m_region);  // used to be double density here
   alllinemap.initialiseAttributesAxial();
   for (size_t k = 0; k < axiallines.size(); k++) {
      axiallines[k].crop(region); // <- should be cropped anyway, but causing an error
      alllinemap.makeLineShape(axiallines[k]);
   }

   // n.b. make connections also initialises attributes
   // -> don't know what this was for: alllinemap.sortBins(m_poly_connections);
   alllinemap.makeConnections(preaxialdata);

   alllinemap.m_keyvertexcount = m_polygons.m_vertex_possibles.size();

   // we can stop here for all line axial map!
   setDisplayedMapRef(m_all_line_map);

   return alllinemap;
}

bool MapGenerator::makeFewestLineMap(Communicator *comm, bool replace_existing)
{
   // no all line map
   if (m_all_line_map == -1) {
      return false;
   }

   if (comm) {
      comm->CommPostMessage( Communicator::NUM_STEPS, 2 );
      comm->CommPostMessage( Communicator::CURRENT_STEP, 1 );
   }

   pafsrand((unsigned int)time(NULL));

   // make one rld for each radial line...
   std::map<RadialKey,pvecint> radialdivisions;
   size_t i;
   for (i = 0; i < m_radial_lines.size(); i++) {
      radialdivisions.insert(std::make_pair( (RadialKey) m_radial_lines[i], pvecint() ));
   }

   // also, a list of radial lines cut by each axial line
   std::map<int,pvecint> ax_radial_cuts;
   std::map<int,pvecint> ax_seg_cuts;
   for (auto shape: at(m_all_line_map).m_shapes) {
      ax_radial_cuts.insert(std::make_pair(shape.first, pvecint()));
      ax_seg_cuts.insert(std::make_pair(shape.first, pvecint()));
   }

   // make divisions -- this is the slow part and the comm updates
   at(m_all_line_map).makeDivisions(m_poly_connections, m_radial_lines, radialdivisions, ax_radial_cuts, comm);

   // the slow part is over, we're into the final straight... reset the current record flag:
   if (comm) {
      comm->CommPostMessage( Communicator::CURRENT_STEP, 2 );
      comm->CommPostMessage( Communicator::CURRENT_RECORD, 0 );
   }

   // a little further setting up is still required...
   std::map<RadialKey,RadialSegment> radialsegs;

   // now make radial segments from the radial lines... (note, start at 1)
   for (i = 1; i < m_radial_lines.size(); i++) {
      if (m_radial_lines[i].vertex == m_radial_lines[i-1].vertex) {
         if (m_radial_lines[i].ang == m_radial_lines[i-1].ang) {
            continue;
         }
         else {
            radialsegs.insert(std::make_pair( (RadialKey)m_radial_lines[i], (RadialSegment)m_radial_lines[i-1]));
         }
      }
   }

   // and segment divisors from the axial lines...
   // TODO: (CS) Restructure this to get rid of all those brittle parallel data structure
   auto axIter = ax_radial_cuts.begin();
   auto axSeg = ax_seg_cuts.begin();
   for (i = 0; i < at(m_all_line_map).m_shapes.size(); i++) {
      for (size_t j = 1; j < axIter->second.size(); j++) {
         // note similarity to loop above
         RadialKey rk_end = m_radial_lines[axIter->second[j]];
         RadialKey rk_start = m_radial_lines[axIter->second[j-1]];
         if (rk_start.vertex == rk_end.vertex) {
            auto radialSegIter = radialsegs.find(rk_end);
            if (radialSegIter != radialsegs.end() && rk_start == radialSegIter->second.radial_b) {
               radialSegIter->second.add(axIter->first);
               axSeg->second.add(std::distance(radialsegs.begin(), radialSegIter));
            }
         }
      }
      axIter++;
      axSeg++;
   }

   // and a little more setting up: key vertex relationships
   prefvec<pvecint> keyvertexconns;
   int *keyvertexcounts = new int [at(m_all_line_map).m_keyvertexcount];
   for (int x = 0; x < at(m_all_line_map).m_keyvertexcount; x++) {
      keyvertexcounts[x] = 0;
   }
   // this sets up a two step relationship: looks for the key vertices for all lines connected to you
   for (size_t y = 0; y < at(m_all_line_map).m_connectors.size(); y++) {
      keyvertexconns.push_back(pvecint());
      Connector& axa = at(m_all_line_map).m_connectors[y];
      for (size_t z = 0; z < axa.m_connections.size(); z++) {
         pvecint& axb = at(m_all_line_map).m_keyvertices[axa.m_connections[z]];
         for (size_t zz = 0; zz < axb.size(); zz++) {
            if (keyvertexconns[y].searchindex(axb[zz]) == paftl::npos) {
               keyvertexconns[y].add(axb[zz],paftl::ADD_HERE);
               keyvertexcounts[axb[zz]] += 1;
            }
         }
      }
   }

   // ok, after this fairly tedious set up, we are ready to go...
   // note axradialcuts aren't required anymore...

   AxialMinimiser minimiser(at(m_all_line_map), ax_seg_cuts.size(), radialsegs.size());

   prefvec<Line> lines_s, lines_m;

   minimiser.removeSubsets(ax_seg_cuts, radialsegs, radialdivisions, m_radial_lines, keyvertexconns, keyvertexcounts);

   // make new lines here (assumes line map has only lines)
   int k = -1;
   for (auto& shape: at(m_all_line_map).m_shapes) {
      k++;
      if (!minimiser.removed(k)) {
         lines_s.push_back( shape.second.getLine() );
      }
   }

   minimiser.fewestLongest(ax_seg_cuts, radialsegs, radialdivisions, m_radial_lines, keyvertexconns, keyvertexcounts);

   // make new lines here (assumes line map has only lines
   for (k = 0; k < at(m_all_line_map).m_shapes.size(); k++) {
      if (!minimiser.removed(k)) {
         lines_m.push_back( depthmapX::getMapAtIndex(at(m_all_line_map).m_shapes, k)->second.getLine() );
      }
   }

   delete [] keyvertexcounts;

   int subsetmapindex = getMapRef("Fewest-Line Map (Subsets)");
   if (subsetmapindex == -1) {
      // didn't used to have hyphenation, try once more:
      subsetmapindex = getMapRef("Fewest Line Map (Subsets)");
   }
   if (subsetmapindex == -1) {
      // create the fewest line map layer...
      subsetmapindex = addMap("Fewest-Line Map (Subsets)",ShapeMap::AXIALMAP);
      // note: new map has replace_existing set to true to ensure "init"
      replace_existing = true;
   }
   ShapeGraph& fewestlinemap_subsets = at(subsetmapindex);
   // note: new map has replace_existing set to true to ensure "init"
   if (replace_existing) {
      fewestlinemap_subsets.clearAll();
      fewestlinemap_subsets.init(lines_s.size(),m_polygons.m_region); // used to have a '2' for double pixel density
   }
   fewestlinemap_subsets.initialiseAttributesAxial();
   for (k = 0; k < lines_s.size(); k++) {
      fewestlinemap_subsets.makeLineShape(lines_s[k]);
   }
   fewestlinemap_subsets.makeConnections();

   int minimalmapindex = getMapRef("Fewest-Line Map (Minimal)");
   if (minimalmapindex == -1) {
      // didn't used to have hyphenation, try once more:
      minimalmapindex = getMapRef("Fewest Line Map (Minimal)");
   }
   if (minimalmapindex == -1) {
      // create the fewest line map layer...
      minimalmapindex = addMap("Fewest-Line Map (Minimal)",ShapeMap::AXIALMAP);
      // note: new map has replace_existing set to true to ensure "init"
      replace_existing = true;
   }
   ShapeGraph& fewestlinemap_minimal = at(minimalmapindex);
   // note: new map has replace_existing set to true to ensure "init"
   if (replace_existing) {
      fewestlinemap_minimal.clearAll();
      fewestlinemap_minimal.init(lines_m.size(),m_polygons.m_region); // used to have a '2' for double pixel density
   }
   fewestlinemap_minimal.initialiseAttributesAxial();
   for (k = 0; k < lines_m.size(); k++) {
      fewestlinemap_minimal.makeLineShape(lines_m[k]);
   }
   fewestlinemap_minimal.makeConnections();

   setDisplayedMapRef(subsetmapindex);

   return true;
}


// Alan and Bill's algo...

void AxialMinimiser::removeSubsets(std::map<int,pvecint>& axsegcuts, std::map<RadialKey,RadialSegment>& radialsegs, std::map<RadialKey,pvecint>& rlds,  pqvector<RadialLine>& radial_lines, prefvec<pvecint>& keyvertexconns, int *keyvertexcounts)
{
   bool removedflag = true;
   int counterrors = 0;

   m_axialconns = m_alllinemap->m_connectors;

   for (size_t x = 0; x < radialsegs.size(); x++) {
      m_radialsegcounts[x] = 0;
   }
   int y = -1;
   for (auto axSegCut: axsegcuts) {
      y++;
      for (size_t z = 0; z < axSegCut.second.size(); z++) {
         m_radialsegcounts[axSegCut.second[z]] += 1;
      }
      m_removed[y] = false;
      m_vital[y] = false;
      m_affected[y] = true;
      m_vps[y].index = y;
      double length = m_axialconns[y].m_connections.size();
      m_vps[y].value1 = (int) length;
      length = depthmapX::getMapAtIndex(m_alllinemap->m_shapes, y)->second.getLine().length();
      m_vps[y].value2 = (float) length;
   }

   // sort according to number of connections then length
   qsort(m_vps,m_axialconns.size(),sizeof(ValueTriplet),compareValueTriplet);

   while (removedflag) {

      removedflag = false;
      for (size_t i = 0; i < m_axialconns.size(); i++) {
         int ii = m_vps[i].index;
         if (m_removed[ii] || !m_affected[ii] || m_vital[ii]) {
            continue;
         }
         // vital connections code (uses original unaltered connections)
         {
            bool vitalconn = false;
            for (size_t j = 0; j < keyvertexconns[ii].size(); j++) {
               // first check to see if removing this line will cause elimination of a vital connection
               if (keyvertexcounts[keyvertexconns[ii][j]] <= 1) {
                  // connect vital... just go on to the next one:
                  vitalconn = true;
                  break;
               }
            }
            if (vitalconn) {
               m_vital[ii] = true;
               continue;
            }
         }
         //
         Connector& axa = m_axialconns[ii];
         m_affected[ii] = false;
         bool subset = false;
         for (size_t j = 0; j < axa.m_connections.size(); j++) {
            int indextob = axa.m_connections[j];
            if (indextob == ii || m_removed[indextob]) { // <- removed[indextob] should never happen as it should have been removed below
               continue;
            }
            Connector& axb = m_axialconns[indextob];
            if (axa.m_connections.size() <= axb.m_connections.size()) {
               // change to 10.08, coconnecting is 1 -> connection to other line is implicitly handled
               int coconnecting = 1;
               // first check it's a connection subset
               // note that changes in 10.08 mean that lines no longer connect to themselves
               // this means that the subset 1 connects {2,3} and 2 connects {1,3} are equivalent
               for (size_t axai = 0, axbi = 0; axai < axa.m_connections.size() && axbi < axb.m_connections.size(); axai++, axbi++) {
                  // extra 10.08 -> step over connection to b
                  if (axa.m_connections[axai] == indextob) {
                     axai++;
                  }
                  // extra 10.08 add axb.m_connections[axbi] == ii -> step over connection to a
                  while (axbi < axb.m_connections.size() && (axb.m_connections[axbi] == ii || axa.m_connections[axai] > axb.m_connections[axbi])) {
                     axbi++;
                  }
                  if (axbi >= axb.m_connections.size()) {
                     break;
                  }
                  else if (axa.m_connections[axai] == axb.m_connections[axbi]) {
                     coconnecting++;
                  }
                  else if (axa.m_connections[axai] < axb.m_connections[axbi]) {
                     break;
                  }
               }
               if (coconnecting >= (int)axa.m_connections.size()) {
                  subset = true;
                  break;
               }
            }
         }
         if (subset) {
            size_t removeindex = ii;
            // now check removing it won't break any topological loops
            bool presumedvital = false;
            auto& axSegCut = depthmapX::getMapAtIndex(axsegcuts, removeindex)->second;
            for (size_t k = 0; k < axSegCut.size(); k++) {
               if (m_radialsegcounts[axSegCut[k]] <= 1) {
                  presumedvital = true;
                  break;
               }
            }
            if (presumedvital) {
               presumedvital = checkVital(removeindex,axSegCut,radialsegs,rlds,radial_lines);
            }
            if (presumedvital) {
               m_vital[removeindex] = true;
            }
            // if not, remove it...
            if (!m_vital[removeindex]) {
               m_removed[removeindex] = true;
               pvecint& affectedconnections = m_axialconns[removeindex].m_connections;
               size_t k;
               for (k = 0; k < affectedconnections.size(); k++) {
                  if (!m_removed[affectedconnections[k]]) {
                     pvecint& connections = m_axialconns[affectedconnections[k]].m_connections;
                     size_t index = connections.searchindex(removeindex);
                     if (index != paftl::npos) {
                        connections.remove_at(index);
                     }
                     m_affected[affectedconnections[k]] = true;
                  }
               }
               removedflag = true;
               for (k = 0; k < axSegCut.size(); k++) {
                  m_radialsegcounts[axSegCut[k]] -= 1;
               }
               // vital connections
               for (k = 0; k < keyvertexconns[removeindex].size(); k++) {
                  keyvertexcounts[keyvertexconns[removeindex][k]] -= 1;
               }
            }
         }
      }
   }
}

///////////////////////////////////////////////////////////////////////////////////////////

// My algo... v. simple... fewest longest

void AxialMinimiser::fewestLongest(std::map<int,pvecint>& axsegcuts, std::map<RadialKey,RadialSegment>& radialsegs, std::map<RadialKey, pvecint> &rlds, pqvector<RadialLine>& radial_lines, prefvec<pvecint>& keyvertexconns, int *keyvertexcounts)
{
   //m_axialconns = m_alllinemap->m_connectors;
   int livecount = 0;

   for (size_t y = 0; y < m_axialconns.size(); y++) {
      if (!m_removed[y] && !m_vital[y]) {
         m_vps[livecount].index = (int) y;
         m_vps[livecount].value1 = (int) m_axialconns[y].m_connections.size();
         m_vps[livecount].value2 = (float) depthmapX::getMapAtIndex(m_alllinemap->m_shapes, y)->second.getLine().length();
         livecount++;
      }
   }

   qsort(m_vps,livecount,sizeof(ValueTriplet),compareValueTriplet);

   for (int i = 0; i < livecount; i++) {

      int j = m_vps[i].index;
      // vital connections code (uses original unaltered connections)
      bool vitalconn = false;
      size_t k;
      for (k = 0; k < keyvertexconns[j].size(); k++) {
         // first check to see if removing this line will cause elimination of a vital connection
         if (keyvertexcounts[keyvertexconns[j][k]] <= 1) {
            // connect vital... just go on to the next one:
            vitalconn = true;
            break;
         }
      }
      if (vitalconn) {
         continue;
      }
      //
      bool presumedvital = false;
      auto &axSegCut = depthmapX::getMapAtIndex(axsegcuts, j)->second;
      for (k = 0; k < axSegCut.size(); k++) {
         if (m_radialsegcounts[axSegCut[k]] <= 1) {
            presumedvital = true;
            break;
         }
      }
      if (presumedvital) {
         presumedvital = checkVital(j,axSegCut,radialsegs,rlds,radial_lines);
      }
      if (!presumedvital) {
         // don't let anything this is connected to go down to zero connections
         pvecint& affectedconnections = m_axialconns[j].m_connections;
         for (size_t k = 0; k < affectedconnections.size(); k++) {
            if (!m_removed[affectedconnections[k]]) {
               pvecint& connections = m_axialconns[affectedconnections[k]].m_connections;
               if (connections.size() <= 2) { // <- note number of connections includes itself... so you and one other
                  presumedvital = true;
                  break;
               }
            }
         }
      }
      if (!presumedvital) {
         m_removed[j] = true;
         pvecint& affectedconnections = m_axialconns[j].m_connections;
         size_t k;
         for (k = 0; k < affectedconnections.size(); k++) {
            if (!m_removed[affectedconnections[k]]) {
               pvecint& connections = m_axialconns[affectedconnections[k]].m_connections;
               size_t index = connections.searchindex(j);
               if (index != paftl::npos) {
                  connections.remove_at(index);
               }
               m_affected[affectedconnections[k]] = true;
            }
         }
         for (k = 0; k < axSegCut.size(); k++) {
            m_radialsegcounts[axSegCut[k]] -= 1;
         }
         // vital connections
         for (k = 0; k < keyvertexconns[j].size(); k++) {
            keyvertexcounts[keyvertexconns[j][k]] -= 1;
         }
      }
   }
}

///////////////////////////////////////////////////////////////////////////////////////////

bool AxialMinimiser::checkVital(int checkindex, pvecint& axsegcuts, std::map<RadialKey,RadialSegment>& radialsegs, std::map<RadialKey, pvecint> &rlds, pqvector<RadialLine>& radial_lines)
{
   std::map<int,SalaShape>& axiallines = m_alllinemap->m_shapes;

   bool presumedvital = true;
   int nonvitalcount = 0, vitalsegs = 0;
   // again, this time more rigourously... check any connected pairs don't cover the link...
   for (size_t k = 0; k < axsegcuts.size(); k++) {
      if (m_radialsegcounts[axsegcuts[k]] <= 1) {
         bool nonvitalseg = false;
         vitalsegs++;
         auto radialSegIter = depthmapX::getMapAtIndex(radialsegs, axsegcuts[k]);
         const RadialKey& key = radialSegIter->first;
         RadialSegment& seg = radialSegIter->second;
         pvecint& divisorsa = rlds.find(key)->second;
         pvecint& divisorsb = rlds.find(seg.radial_b)->second;
         RadialLine& rlinea = radial_lines.search(key);
         RadialLine& rlineb = radial_lines.search(seg.radial_b);
         for (size_t divi = 0; divi < divisorsa.size(); divi++) {
            if (divisorsa[divi] == checkindex || m_removed[divisorsa[divi]]) {
               continue;
            }
            for (size_t divj = 0; divj < divisorsb.size(); divj++) {
               if (divisorsb[divj] == checkindex || m_removed[divisorsb[divj]]) {
                  continue;
               }
               if (m_axialconns[divisorsa[divi]].m_connections.searchindex(divisorsb[divj]) != paftl::npos) {
                  // as a further challenge, they must link within in the zone of interest, not on the far side of it... arg!
                  Point2f p = intersection_point(axiallines[divisorsa[divi]].getLine(),axiallines[divisorsb[divj]].getLine(),TOLERANCE_A);
                  if (p.insegment(rlinea.keyvertex,rlinea.openspace,rlineb.openspace,TOLERANCE_A)) {
                     nonvitalseg = true;
                  }
               }
            }
         }
         if (nonvitalseg) {
            nonvitalcount++;
         }
      }
   }
   if (nonvitalcount == vitalsegs) {
      presumedvital = false;
   }
   return presumedvital;
}


///////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

// helper -- a little class to tidy up a set of lines

void TidyLines::tidy(std::vector<Line>& lines, const QtRegion& region)
{
   m_region = region;
   double maxdim = __max(m_region.width(),m_region.height());

   // simple first pass -- remove very short lines
   lines.erase(
               std::remove_if(lines.begin(), lines.end(),
                              [maxdim](const Line& line)
   {return line.length() < maxdim * TOLERANCE_B;}), lines.end());

   // now load up m_lines...
   initLines(lines.size(),m_region.bottom_left,m_region.top_right);
   for (auto& line: lines) {
      addLine(line);
   }
   sortPixelLines();

   std::vector<int> removelist;
   for (size_t i = 0; i < lines.size(); i++) {
      // n.b., as m_lines have just been made, note that what's in m_lines matches whats in lines
      // we will use this later!
      m_test++;
      m_lines[i].test = m_test;
      PixelRefVector list = pixelateLine( m_lines[i].line );
      for (size_t a = 0; a < list.size(); a++) {
         for (size_t b = 0; b < m_pixel_lines[ list[a].x ][ list[a].y ].size(); b++) {
            int j = m_pixel_lines[ list[a].x ][ list[a].y ][b];
            if (m_lines[j].test != m_test && j > (int)i && intersect_region(lines[i],lines[j],TOLERANCE_B * maxdim)) {
               m_lines[j].test = m_test;
               int axis_i = (lines[i].width() >= lines[i].height()) ? XAXIS : YAXIS;
               int axis_j = (lines[j].width() >= lines[j].height()) ? XAXIS : YAXIS;
               int axis_reverse = (axis_i == XAXIS) ? YAXIS : XAXIS;
               if (axis_i == axis_j && fabs(lines[i].grad(axis_reverse) - lines[j].grad(axis_reverse)) < TOLERANCE_A
                                    && fabs(lines[i].constant(axis_reverse) - lines[j].constant(axis_reverse)) < (TOLERANCE_B * maxdim)) {
                  // check for overlap and merge
                  int parity = (axis_i == XAXIS) ? 1 : lines[i].sign();
                  if ((lines[i].start()[axis_i] * parity + TOLERANCE_B * maxdim) > (lines[j].start()[axis_j] * parity) &&
                      (lines[i].start()[axis_i] * parity) < (lines[j].end()[axis_j] * parity + TOLERANCE_B * maxdim)) {
                     int end = ((lines[i].end()[axis_i] * parity) > (lines[j].end()[axis_j] * parity)) ? i : j;
                     lines[j].bx() = lines[end].bx();
                     lines[j].by() = lines[end].by();
                     removelist.push_back(i);
                     continue; // <- don't do this any more, we've zapped it and replaced it with the later line
                  }
                  if ((lines[j].start()[axis_j] * parity + TOLERANCE_B * maxdim) > (lines[i].start()[axis_i] * parity) &&
                      (lines[j].start()[axis_j] * parity) < (lines[i].end()[axis_i]  * parity + TOLERANCE_B * maxdim)) {
                     int end = ((lines[i].end()[axis_i] * parity) > (lines[j].end()[axis_j] * parity)) ? i : j;
                     lines[j].ax() = lines[i].ax();
                     lines[j].ay() = lines[i].ay();
                     lines[j].bx() = lines[end].bx();
                     lines[j].by() = lines[end].by();
                     removelist.push_back(i);
                     continue; // <- don't do this any more, we've zapped it and replaced it with the later line
                  }
               }
            }
         }
      }
   }

   // comes out sorted, remove duplicates just in case
   removelist.erase(std::unique(removelist.begin(), removelist.end()), removelist.end());

   for(auto iter = removelist.rbegin(); iter != removelist.rend(); ++iter)
       lines.erase(lines.begin() + *iter);
   removelist.clear();  // always clear this list, it's reused
}

void TidyLines::quicktidy(std::map<int,Line>& lines, const QtRegion& region)
{
   m_region = region;

   double avglen = 0.0;

   for (auto line: lines) {
      avglen += line.second.length();
   }
   avglen /= lines.size();

   double tolerance = avglen * 10e-6;

   auto iter = lines.begin(), end = lines.end();
   for(; iter != end; ) {
       if (iter->second.length() < tolerance) {
           iter = lines.erase(iter);
       } else {
           ++iter;
       }
   }

   // now load up m_lines...
   initLines(lines.size(),m_region.bottom_left,m_region.top_right);
   for (auto line: lines) {
      addLine(line.second);
   }
   sortPixelLines();

   // and chop duplicate lines:
   std::vector<int> removelist;
   int i = -1;
   for (auto line: lines) {
      i++;
      PixelRef start = pixelate(line.second.start());
      for (size_t j = 0; j < m_pixel_lines[start.x][start.y].size(); j++) {
         int k = m_pixel_lines[start.x][start.y][j];
         if (k > (int)i && approxeq(m_lines[i].line.start(),m_lines[k].line.start(),tolerance)) {
            if (approxeq(m_lines[i].line.end(),m_lines[k].line.end(),tolerance)) {
               removelist.push_back(line.first);
               break;
            }
         }
      }
   }
   for(int remove: removelist) {
       lines.erase(remove);
   }
   removelist.clear(); // always clear this list, it's reused}
}

/////////////////////////////////////////////////////
