#pragma once

#include "salalib/spacepixfile.h"
#include "salalib/alllinemap.h"
#include "genlib/comm.h"


// a class to reduce all line maps to fewest line maps

class MapGenerator
{
public:
    AllLineMap makeAllLineMap(Communicator *comm, std::vector<SpacePixelFile> &drawingLayers, const Point2f& seed);
    bool makeFewestLineMap(Communicator *comm, bool replace_existing);

};

class AxialMinimiser {
protected:
   ShapeGraph *m_alllinemap;
   //
   ValueTriplet *m_vps;
   bool *m_removed;
   bool *m_affected;
   bool *m_vital;
   int *m_radialsegcounts;
   int *m_keyvertexcounts;
   std::vector<Connector> m_axialconns; // <- uses a copy of axial lines as it will remove connections
public:
   AxialMinimiser(const ShapeGraph& alllinemap, int no_of_axsegcuts, int no_of_radialsegs);
   ~AxialMinimiser();
   void removeSubsets(std::map<int,pvecint>& axsegcuts, std::map<RadialKey,RadialSegment>& radialsegs, std::map<RadialKey,pvecint>& rlds, pqvector<RadialLine>& radial_lines, prefvec<pvecint>& keyvertexconns, int *keyvertexcounts);
   void fewestLongest(std::map<int,pvecint>& axsegcuts, std::map<RadialKey,RadialSegment>& radialsegs, std::map<RadialKey,pvecint>& rlds, pqvector<RadialLine>& radial_lines, prefvec<pvecint>& keyvertexconns, int *keyvertexcounts);
   // advanced topological testing:
   bool checkVital(int checkindex,pvecint& axsegcuts, std::map<RadialKey,RadialSegment>& radialsegs, std::map<RadialKey,pvecint>& rlds, pqvector<RadialLine>& radial_lines);
   //
   bool removed(int i) const
   { return m_removed[i]; }
};

class TidyLines : public SpacePixel
{
public:
   TidyLines() {;}
   virtual ~TidyLines() {;}
   void tidy(std::vector<Line> &lines, const QtRegion& region);
   void quicktidy(std::map<int, Line> &lines, const QtRegion& region);
};
