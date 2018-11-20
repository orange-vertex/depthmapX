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

#include "salalib/vgamodules/vgametricshortestpath.h"

#include "genlib/stringutils.h"

bool VGAMetricDepthShortestPath::run(Communicator *comm, const Options &options, PointMap &map, bool) {

    auto& attributes = map.getAttributeTable();
    auto& selection_set = map.getSelSet();

    int col = attributes.insertColumn("Metric Shortest Path");

    for (int i = 0; i < attributes.getRowCount(); i++) {
       PixelRef pix = attributes.getRowKey(i);
       map.getPoint(pix).m_misc = 0;
       map.getPoint(pix).m_dist = -1.0f;
       map.getPoint(pix).m_cumangle = 0.0f;
    }

    // in order to calculate Penn angle, the MetricPair becomes a metric triple...
    std::set<MetricTriple> search_list; // contains root point

    if(selection_set.size() != 2) {
        throw depthmapX::RuntimeException("Two nodes must be selected");
    }
    PixelRef pixelFrom = *selection_set.begin();
    PixelRef pixelTo = *std::next(selection_set.begin());

    search_list.insert(MetricTriple(0.0f,pixelFrom,NoPixel));

    // note that m_misc is used in a different manner to analyseGraph / PointDepth
    // here it marks the node as used in calculation only
    std::map<PixelRef, PixelRef> parents;
    while (search_list.size()) {
       std::set<MetricTriple>::iterator it = search_list.begin();
       MetricTriple here = *it;
       search_list.erase(it);
       Point& p = map.getPoint(here.pixel);
       std::set<MetricTriple> newPixels;
       // nb, the filled check is necessary as diagonals seem to be stored with 'gaps' left in
       if (p.filled() && p.m_misc != ~0) {
          p.getNode().extractMetric(newPixels, &map,here);
          p.m_misc = ~0;
          if (!p.getMergePixel().empty()) {
             Point& p2 = map.getPoint(p.getMergePixel());
             if (p2.m_misc != ~0) {
                p2.m_cumangle = p.m_cumangle;
                p2.getNode().extractMetric(newPixels, &map, MetricTriple(here.dist,p.getMergePixel(),NoPixel));
                p2.m_misc = ~0;
             }
          }
       }
       for (auto& pixel : newPixels) {
           if(pixel.pixel == pixelTo) {
               int counter = 0;
               int row = attributes.getRowid(pixel.pixel);
               attributes.setValue(row, col, counter);
               counter++;
               row = attributes.getRowid(here.pixel);
               attributes.setValue(row, col, counter);
               counter++;
               auto currParent = parents.find(here.pixel);
               while (currParent != parents.end()) {
                   int row = attributes.getRowid(currParent->second);
                   attributes.setValue(row, col, counter);
                   currParent = parents.find(currParent->second);
                   counter++;
               }
               map.setDisplayedAttribute(-2);
               map.setDisplayedAttribute(col);

               return true;
           }
           parents[pixel.pixel] = here.pixel;
       }
       search_list.insert(newPixels.begin(), newPixels.end());
    }

    return false;
}
