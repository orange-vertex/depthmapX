#pragma once

#include "salalib/axialmap.h"
#include "salalib/axialpolygons.h"

struct ValueTriplet
{
   int value1;
   float value2;
   int index;
};

class AxialMinimiser
{
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
   void removeSubsets(const std::map<int, pvecint> &axsegcuts,
                      const std::map<RadialKey, RadialSegment> &radialsegs,
                      const std::map<RadialKey, pvecint> &rlds,
                      const pqvector<RadialLine> &radial_lines,
                      const prefvec<pvecint> &keyvertexconns,
                      int *keyvertexcounts);
   void fewestLongest(const std::map<int,pvecint>& axsegcuts,
                      const std::map<RadialKey,RadialSegment>& radialsegs,
                      const std::map<RadialKey,pvecint>& rlds,
                      const pqvector<RadialLine>& radial_lines,
                      const prefvec<pvecint>& keyvertexconns,
                      int *keyvertexcounts);
   // advanced topological testing:
   bool checkVital(const int checkindex,
                   const pvecint& axsegcuts,
                   const std::map<RadialKey,RadialSegment>& radialsegs,
                   const std::map<RadialKey,pvecint>& rlds,
                   const pqvector<RadialLine>& radial_lines);
   //
   bool removed(int i) const
   { return m_removed[i]; }
};
