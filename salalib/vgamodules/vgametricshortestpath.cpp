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

    int path_col = attributes.insertOrResetColumn("Metric Shortest Path");
    int linked_col = attributes.insertOrResetColumn("Metric Shortest Path Linked");
    int order_col = attributes.insertOrResetColumn("Metric Shortest Path Order");
    int zone_col = attributes.insertOrResetColumn("Metric Shortest Path Zone");
    int zone_3m_col = attributes.insertOrResetColumn("Metric Shortest Path Zone 3m");

    // custom linking costs from the attribute table
    int link_metric_cost_col = attributes.insertOrResetColumn("Link Metric Cost");

    for (auto& row: attributes) {
        PixelRef pix = PixelRef(row.getKey().value);
        Point &p = map.getPoint(pix);
        p.m_misc = 0;
        p.m_dist = -1.0f;
        p.m_cumangle = 0.0f;
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
    bool pixelFound = false;
    while (search_list.size()) {
        std::set<MetricTriple>::iterator it = search_list.begin();
        MetricTriple here = *it;
        search_list.erase(it);
        Point &p = map.getPoint(here.pixel);
        std::set<MetricTriple> newPixels;
        std::set<MetricTriple> mergePixels;
        // nb, the filled check is necessary as diagonals seem to be stored with 'gaps' left in
        if (p.filled() && p.m_misc != ~0) {
            extractMetric(p.getNode(), newPixels, &map, here, 0, map.getSpacing());
            p.m_misc = ~0;
            if (!p.getMergePixel().empty()) {
                Point &p2 = map.getPoint(p.getMergePixel());
                if (p2.m_misc != ~0) {
                    auto newTripleIter = newPixels.insert(MetricTriple(here.dist, p.getMergePixel(), NoPixel));
                    p2.m_cumangle = p.m_cumangle;
                    float extraMetricCost = 0;
                    if(link_metric_cost_col != -1) {
                        AttributeRow& mergeRow = attributes.getRow(AttributeKey(p.getMergePixel()));
                        extraMetricCost = mergeRow.getValue(link_metric_cost_col);
                    }
                    extractMetric(p2.getNode(), mergePixels, &map, *newTripleIter.first, extraMetricCost, map.getSpacing());
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
        for (auto &pixel : newPixels) {
            if (pixel.pixel == pixelTo) {
                pixelFound = true;
            }
        }
        if (!pixelFound)
            search_list.insert(newPixels.begin(), newPixels.end());
    }

    int linePixelCounter = 0;
    auto pixelToParent = parents.find(pixelTo);
    if (pixelToParent != parents.end()) {

        for (auto& row: attributes) {
            PixelRef pix = PixelRef(row.getKey().value);
            Point &p = map.getPoint(pix);
            p.m_misc = 0;
            p.m_dist = -1.0f;
            p.m_cumangle = 0.0f;
        }


        int counter = 0;
        AttributeRow& lastPixelRow = attributes.getRow(AttributeKey(pixelTo));
        lastPixelRow.setValue(order_col, counter);
        counter++;
        auto currParent = pixelToParent;
        counter++;
        while (currParent != parents.end()) {
            Point &p = map.getPoint(currParent->second);
            AttributeRow& row = attributes.getRow(AttributeKey(currParent->second));
            row.setValue(order_col, counter);

            if (!p.getMergePixel().empty() && p.getMergePixel() == currParent->first) {
                row.setValue(linked_col, 1);
                lastPixelRow.setValue(linked_col, 1);
            } else {
                // apparently we can't just have 1 number in the whole column
                row.setValue(linked_col, 0);
                auto pixelated = map.quickPixelateLine(currParent->first, currParent->second);
                for (auto &linePixel : pixelated) {
                    auto* linePixelRow = attributes.getRowPtr(AttributeKey(linePixel));
                    if (linePixelRow != 0) {
                        linePixelRow->setValue(path_col, linePixelCounter++);
                        linePixelRow->setValue(zone_col, 1);

                        std::set<MetricTriple> newPixels;
                        Point &p = map.getPoint(linePixel);
                        extractMetric(p.getNode(), newPixels, &map, MetricTriple(0.0f, linePixel, NoPixel), 0, map.getSpacing());
                        for (auto &zonePixel : newPixels) {
                            auto* zonePixelRow = attributes.getRowPtr(AttributeKey(zonePixel.pixel));
                            if (zonePixelRow != 0) {
                                double zoneLineDist = dist(linePixel, zonePixel.pixel);
                                float currZonePixelVal = zonePixelRow->getValue(zone_col);
                                if (currZonePixelVal == -1 || 1.0f / (zoneLineDist + 1) > currZonePixelVal) {
                                    zonePixelRow->setValue(zone_col, 1.0f / (zoneLineDist + 1));
                                }
                                if (zoneLineDist * map.getSpacing() < 3000) {
                                    zonePixelRow->setValue(zone_3m_col, linePixelCounter);
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

    return false;
}

void VGAMetricShortestPath::extractMetric(Node n, std::set<MetricTriple> &pixels, PointMap *pointdata,
                                          const MetricTriple &curs, float extraMetricCost, float spacing) {
    if (curs.dist == 0.0f || pointdata->getPoint(curs.pixel).blocked() || pointdata->blockedAdjacent(curs.pixel)) {
        for (int i = 0; i < 32; i++) {
            Bin &bin = n.bin(i);
            for (auto pixVec : bin.m_pixel_vecs) {
                for (PixelRef pix = pixVec.start(); pix.col(bin.m_dir) <= pixVec.end().col(bin.m_dir);) {
                    Point &pt = pointdata->getPoint(pix);
                    if (pt.m_misc == 0 &&
                            (pt.m_dist == -1.0 ||(extraMetricCost + curs.dist + spacing*dist(pix, curs.pixel) < pt.m_dist))) {
                        pt.m_dist = extraMetricCost + curs.dist + spacing*(float)dist(pix, curs.pixel);
                        // n.b. dmap v4.06r now sets angle in range 0 to 4 (1 = 90 degrees)
                        pt.m_cumangle = pointdata->getPoint(curs.pixel).m_cumangle +
                                        (curs.lastpixel == NoPixel
                                             ? 0.0f
                                             : (float)(angle(pix, curs.pixel, curs.lastpixel) / (M_PI * 0.5)));
                        pixels.insert(MetricTriple(pt.m_dist, pix, curs.pixel));
                    }
                    pix.move(bin.m_dir);
                }
            }
        }
    }
}
