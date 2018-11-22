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

bool VGAMetricShortestPath::run(Communicator *comm, const Options &options, PointMap &map, bool) {

    auto &attributes = map.getAttributeTable();
    auto &selection_set = map.getSelSet();

    int path_col = attributes.insertColumn("Metric Shortest Path");
    int linked_col = attributes.insertColumn("Metric Shortest Path Linked");
    int order_col = attributes.insertColumn("Metric Shortest Path Order");
    int zone_col = attributes.insertColumn("Metric Shortest Path Zone");
    int zone_3m_col = attributes.insertColumn("Metric Shortest Path Zone 3m");

    for (int i = 0; i < attributes.getRowCount(); i++) {
        PixelRef pix = attributes.getRowKey(i);
        map.getPoint(pix).m_misc = 0;
        map.getPoint(pix).m_dist = -1.0f;
        map.getPoint(pix).m_cumangle = 0.0f;
    }

    // in order to calculate Penn angle, the MetricPair becomes a metric triple...
    std::set<MetricTriple> search_list; // contains root point

    if (selection_set.size() != 2) {
        throw depthmapX::RuntimeException("Two nodes must be selected");
    }
    PixelRef pixelFrom = *selection_set.begin();
    PixelRef pixelTo = *std::next(selection_set.begin());

    search_list.insert(MetricTriple(0.0f, pixelFrom, NoPixel));

    // note that m_misc is used in a different manner to analyseGraph / PointDepth
    // here it marks the node as used in calculation only
    std::map<PixelRef, PixelRef> parents;
    while (search_list.size()) {
        std::set<MetricTriple>::iterator it = search_list.begin();
        MetricTriple here = *it;
        search_list.erase(it);
        Point &p = map.getPoint(here.pixel);
        std::set<MetricTriple> newPixels;
        std::set<MetricTriple> mergePixels;
        // nb, the filled check is necessary as diagonals seem to be stored with 'gaps' left in
        if (p.filled() && p.m_misc != ~0) {
            p.getNode().extractMetric(newPixels, &map, here);
            p.m_misc = ~0;
            if (!p.getMergePixel().empty()) {
                Point &p2 = map.getPoint(p.getMergePixel());
                if (p2.m_misc != ~0) {
                    auto newTripleIter = newPixels.insert(MetricTriple(here.dist, p.getMergePixel(), NoPixel));
                    p2.m_cumangle = p.m_cumangle;
                    p2.getNode().extractMetric(mergePixels, &map, *newTripleIter.first);
                    for (auto &pixel : mergePixels) {
                        parents[pixel.pixel] = p.getMergePixel();
                    }
                    p2.m_misc = ~0;
                }
            }
        }
        for (auto &pixel : newPixels) {
            parents[pixel.pixel] = here.pixel;
        }
        newPixels.insert(mergePixels.begin(), mergePixels.end());
        int linePixelCounter = 0;
        for (auto &pixel : newPixels) {
            if (pixel.pixel == pixelTo) {



                for (int i = 0; i < attributes.getRowCount(); i++) {
                    PixelRef pix = attributes.getRowKey(i);
                    map.getPoint(pix).m_misc = 0;
                    map.getPoint(pix).m_dist = -1.0f;
                    map.getPoint(pix).m_cumangle = 0.0f;
                }




                int counter = 0;
                int row = attributes.getRowid(pixel.pixel);
                attributes.setValue(row, order_col, counter);
                counter++;
                int lastPixelRow = row;
                auto currParent = parents.find(pixel.pixel);
                row = attributes.getRowid(currParent->second);
                attributes.setValue(row, order_col, counter);

                if (!p.getMergePixel().empty() && p.getMergePixel() == currParent->first) {
                    attributes.setValue(row, linked_col, 1);
                    attributes.setValue(lastPixelRow, linked_col, 1);
                } else {
                    // apparently we can't just have 1 number in the whole column
                    attributes.setValue(row, linked_col, 0);
                    auto pixelated = map.quickPixelateLine(currParent->first, currParent->second);
                    for (auto &linePixel : pixelated) {
                        int linePixelRow = attributes.getRowid(linePixel);
                        if (linePixelRow != -1) {
                            attributes.setValue(linePixelRow, path_col, linePixelCounter++);

                            std::set<MetricTriple> newPixels;
                            Point &p = map.getPoint(linePixel);
                            p.getNode().extractMetric(newPixels, &map, MetricTriple(0.0f, linePixel, NoPixel));
                            for (auto &zonePixel: newPixels) {
                                int zonePixelRow = attributes.getRowid(zonePixel.pixel);
                                if (zonePixelRow != -1) {
                                    if(attributes.getValue(zonePixelRow, zone_col) == -1) {
                                        attributes.setValue(zonePixelRow, zone_col, linePixelCounter);
                                    }
                                    if(dist(linePixel, zonePixel.pixel)*map.getSpacing() < 3000) {
                                        attributes.setValue(zonePixelRow, zone_3m_col, linePixelCounter);
                                    } else {
                                        map.getPoint(zonePixel.pixel).m_misc = 0;
                                        map.getPoint(zonePixel.pixel).m_extent = zonePixel.pixel;
                                    }
                                }
                            }

                        }
                    }
                }

                lastPixelRow = row;
                currParent = parents.find(currParent->second);
                counter++;
                if (currParent->first != here.pixel) {
                    row = attributes.getRowid(here.pixel);
                    attributes.setValue(row, order_col, counter);

                    if (!p.getMergePixel().empty() && p.getMergePixel() == currParent->first) {
                        attributes.setValue(row, linked_col, 1);
                        attributes.setValue(lastPixelRow, linked_col, 1);
                    } else {
                        // apparently we can't just have 1 number in the whole column
                        attributes.setValue(row, linked_col, 0);
                        auto pixelated = map.quickPixelateLine(currParent->first, currParent->second);
                        for (auto &linePixel : pixelated) {
                            int linePixelRow = attributes.getRowid(linePixel);
                            if (linePixelRow != -1) {
                                attributes.setValue(linePixelRow, path_col, linePixelCounter++);

                                std::set<MetricTriple> newPixels;
                                Point &p = map.getPoint(linePixel);
                                p.getNode().extractMetric(newPixels, &map, MetricTriple(0.0f, linePixel, NoPixel));
                                for (auto &zonePixel: newPixels) {
                                    int zonePixelRow = attributes.getRowid(zonePixel.pixel);
                                    if (zonePixelRow != -1) {
                                        if(attributes.getValue(zonePixelRow, zone_col) == -1) {
                                            attributes.setValue(zonePixelRow, zone_col, linePixelCounter);
                                        }
                                        if(dist(linePixel, zonePixel.pixel)*map.getSpacing() < 3000) {
                                            attributes.setValue(zonePixelRow, zone_3m_col, linePixelCounter);
                                        } else {
                                            map.getPoint(zonePixel.pixel).m_misc = 0;
                                            map.getPoint(zonePixel.pixel).m_extent = zonePixel.pixel;
                                        }
                                    }
                                }

                            }
                        }
                    }

                    lastPixelRow = row;
                    currParent = parents.find(here.pixel);
                    counter++;
                }
                while (currParent != parents.end()) {
                    Point &p = map.getPoint(currParent->second);
                    int row = attributes.getRowid(currParent->second);
                    attributes.setValue(row, order_col, counter);

                    if (!p.getMergePixel().empty() && p.getMergePixel() == currParent->first) {
                        attributes.setValue(row, linked_col, 1);
                        attributes.setValue(lastPixelRow, linked_col, 1);
                    } else {
                        // apparently we can't just have 1 number in the whole column
                        attributes.setValue(row, linked_col, 0);
                        auto pixelated = map.quickPixelateLine(currParent->first, currParent->second);
                        for (auto &linePixel : pixelated) {
                            int linePixelRow = attributes.getRowid(linePixel);
                            if (linePixelRow != -1) {
                                attributes.setValue(linePixelRow, path_col, linePixelCounter++);

                                std::set<MetricTriple> newPixels;
                                Point &p = map.getPoint(linePixel);
                                p.getNode().extractMetric(newPixels, &map, MetricTriple(0.0f, linePixel, NoPixel));
                                for (auto &zonePixel: newPixels) {
                                    int zonePixelRow = attributes.getRowid(zonePixel.pixel);
                                    if (zonePixelRow != -1) {
                                        if(attributes.getValue(zonePixelRow, zone_col) == -1) {
                                            attributes.setValue(zonePixelRow, zone_col, linePixelCounter);
                                        }
                                        if(dist(linePixel, zonePixel.pixel)*map.getSpacing() < 3000) {
                                            attributes.setValue(zonePixelRow, zone_3m_col, linePixelCounter);
                                        } else {
                                            map.getPoint(zonePixel.pixel).m_misc = 0;
                                            map.getPoint(zonePixel.pixel).m_extent = zonePixel.pixel;
                                        }
                                    }
                                }

                            }
                        }
                    }

                    lastPixelRow = row;
                    currParent = parents.find(currParent->second);
                    counter++;
                }

                map.overrideDisplayedAttribute(-2);
                map.setDisplayedAttribute(order_col);

                return true;
            }
        }
        search_list.insert(newPixels.begin(), newPixels.end());
    }

    return false;
}
