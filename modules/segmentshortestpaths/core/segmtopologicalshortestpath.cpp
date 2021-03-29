// sala - a component of the depthmapX - spatial network analysis platform
// Copyright (C) 2000-2010, University College London, Alasdair Turner
// Copyright (C) 2011-2012, Tasos Varoudis
// Copyright (C) 2017-2018, Petros Koutsolampros

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

#include "segmtopologicalshortestpath.h"

#include "genlib/stringutils.h"

bool SegmentTopologicalShortestPath::run(Communicator *) {

    struct TopologicalSegmentRef {
        int ref;
        double dist;
        bool done;
        TopologicalSegmentRef(int r = -1, double di = 0.0) {
            ref = r;
            dist = di;
            done = false;
        }
    };

    AttributeTable &attributes = m_map.getAttributeTable();

    bool retvar = true;

    int depth_col = attributes.insertOrResetColumn("Topological Shortest Path Depth");
    int path_col = attributes.insertOrResetColumn("Topological Shortest Path Order");

    // record axial line refs for topological analysis
    std::vector<int> axialrefs;

    size_t axialRefColIdx = attributes.getColumnIndex("Axial Line Ref");
    for (auto rowIt = attributes.begin(); rowIt != attributes.end(); rowIt++) {
        axialrefs.push_back(rowIt->getRow().getValue(axialRefColIdx));
    }

    int maxbin = 2;

    std::vector<unsigned int> seen(m_map.getShapeCount(), 0xffffffff);
    std::vector<TopologicalSegmentRef> audittrail(m_map.getShapeCount());
    std::vector<int> list[512]; // 512 bins!
    int open = 0;

    auto &selected = m_map.getSelSet();
    if (selected.size() != 2) {
        return false;
    }
    int refFrom = *selected.begin();
    int refTo = *selected.rbegin();

    seen[refFrom] = 0;
    open++;
    audittrail[refFrom] = TopologicalSegmentRef(refFrom, 0);
    list[0].push_back(refFrom);
    m_map.getAttributeRowFromShapeIndex(refFrom).setValue(depth_col, 0);

    unsigned int segdepth = 0;
    int bin = 0;

    std::map<unsigned int, unsigned int> parents;
    bool refFound = false;

    while (open != 0 && !refFound) {
        while (list[bin].size() == 0) {
            bin++;
            segdepth += 1;
            if (bin == maxbin) {
                bin = 0;
            }
        }
        //
        TopologicalSegmentRef &here = audittrail[list[bin].back()];
        list[bin].pop_back();
        open--;
        // this is necessary using unsigned ints for "seen", as it is possible to add a node twice
        if (here.done) {
            continue;
        } else {
            here.done = true;
        }

        Connector &axline = m_map.getConnections().at(here.ref);
        int connected_cursor = -2;

        auto iter = axline.m_back_segconns.begin();
        bool backsegs = true;

        while (connected_cursor != -1) {
            if (backsegs && iter == axline.m_back_segconns.end()) {
                iter = axline.m_forward_segconns.begin();
                backsegs = false;
            }
            if (!backsegs && iter == axline.m_forward_segconns.end()) {
                break;
            }

            connected_cursor = iter->first.ref;
            AttributeRow &row = m_map.getAttributeRowFromShapeIndex(connected_cursor);
            if (seen[connected_cursor] > segdepth) {
                int axialref = axialrefs[connected_cursor];
                seen[connected_cursor] = segdepth;
                audittrail[connected_cursor] = TopologicalSegmentRef(connected_cursor, 0);
                // puts in a suitable bin ahead of us...
                open++;
                //
                if (axialrefs[here.ref] == axialref) {
                    list[bin].push_back(connected_cursor);
                    row.setValue(depth_col, segdepth);
                } else {
                    list[(bin + 1) % 2].push_back(connected_cursor);
                    seen[connected_cursor] =
                        segdepth +
                        1; // this is so if another node is connected directly to this one but is found later it is
                           // still handled -- note it can result in the connected cursor being added twice
                    row.setValue(depth_col, segdepth + 1);
                }
                if (parents.find(connected_cursor) == parents.end()) {
                    parents[connected_cursor] = here.ref;
                }
            }
            if (connected_cursor == refTo) {
                refFound = true;
                break;
            }
            iter++;
        }
    }

    auto refToParent = parents.find(refTo);
    int counter = 0;
    while (refToParent != parents.end()) {
        AttributeRow &row = m_map.getAttributeRowFromShapeIndex(refToParent->first);
        row.setValue(path_col, counter);
        counter++;
        refToParent = parents.find(refToParent->second);
    }
    m_map.getAttributeRowFromShapeIndex(refFrom).setValue(path_col, counter);

    for (auto iter = attributes.begin(); iter != attributes.end(); iter++) {
        AttributeRow &row = iter->getRow();
        if (row.getValue(path_col) < 0) {
            row.setValue(depth_col, -1);
        } else {
            row.setValue(path_col, counter - row.getValue(path_col));
        }
    }

    m_map.overrideDisplayedAttribute(-2); // <- override if it's already showing
    m_map.setDisplayedAttribute(depth_col);

    return retvar;
}
