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

#include "salalib/alllinemap.h"

struct ValueTriplet {
    int value1;
    float value2;
    int index;
};

class AxialMinimiser {
  protected:
    AllLineMap *m_alllinemap;
    //
    ValueTriplet *m_vps;
    bool *m_removed;
    bool *m_affected;
    bool *m_vital;
    int *m_radialsegcounts;
    int *m_keyvertexcounts;
    std::vector<Connector> m_axialconns; // <- uses a copy of axial lines as it will remove connections
  public:
    AxialMinimiser(const AllLineMap &alllinemap, int no_of_axsegcuts, int no_of_radialsegs);
    ~AxialMinimiser();
    void removeSubsets(std::map<int, std::set<int>> &axsegcuts, std::map<RadialKey, RadialSegment> &radialsegs,
                       std::map<RadialKey, std::set<int>> &rlds, std::vector<RadialLine> &radial_lines,
                       std::vector<std::vector<int>> &keyvertexconns, std::vector<int> &keyvertexcounts);
    void fewestLongest(std::map<int, std::set<int>> &axsegcuts, std::map<RadialKey, RadialSegment> &radialsegs,
                       std::map<RadialKey, std::set<int>> &rlds, std::vector<RadialLine> &radial_lines,
                       std::vector<std::vector<int>> &keyvertexconns, std::vector<int> &keyvertexcounts);
    // advanced topological testing:
    bool checkVital(int checkindex, std::set<int> &axSegCut, std::map<RadialKey, RadialSegment> &radialsegs,
                    std::map<RadialKey, std::set<int>> &rlds, std::vector<RadialLine> &radial_lines);
    //
    bool removed(int i) const { return m_removed[i]; }
};
