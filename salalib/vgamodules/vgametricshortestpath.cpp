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

    // custom linking costs from the attribute table
    int link_metric_cost_col = attributes.getOrInsertColumn("Link Metric Cost");

    int path_col = attributes.insertOrResetColumn("Metric Shortest Path");
    int linked_col = attributes.insertOrResetColumn("Metric Shortest Path Linked");
    int order_col = attributes.insertOrResetColumn("Metric Shortest Path Order");
    int zone_col = attributes.insertOrResetColumn("Metric Shortest Path Zone");
    int zone_3m_col = attributes.insertOrResetColumn("Metric Shortest Path Zone 3m");

    depthmapX::ColumnMatrix<MetricPoint> metricPoints(map.getRows(), map.getCols());

    for (auto& row: attributes) {
        PixelRef pix = PixelRef(row.getKey().value);
        getMetricPoint(metricPoints, pix).m_point = &(map.getPoint(pix));
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
        MetricPoint& mp = getMetricPoint(metricPoints, here.pixel);
        std::set<MetricTriple> newPixels;
        std::set<MetricTriple> mergePixels;
        // nb, the filled check is necessary as diagonals seem to be stored with 'gaps' left in
        if (mp.m_point->filled() && mp.m_misc != ~0) {
            extractMetric(metricPoints, mp.m_point->getNode(), newPixels, &map, here, 0, map.getSpacing());
            mp.m_misc = ~0;
            if (!mp.m_point->getMergePixel().empty()) {
                MetricPoint& mp2 = getMetricPoint(metricPoints, mp.m_point->getMergePixel());
                if (mp2.m_misc != ~0) {
                    auto newTripleIter = newPixels.insert(MetricTriple(here.dist, mp.m_point->getMergePixel(), NoPixel));
                    mp2.m_cumangle = mp.m_cumangle;
                    float extraMetricCost = 0;
                    if(link_metric_cost_col != -1) {
                        AttributeRow& mergeRow = attributes.getRow(AttributeKey(mp.m_point->getMergePixel()));
                        extraMetricCost = mergeRow.getValue(link_metric_cost_col);
                    }
                    mp2.m_dist = here.dist + extraMetricCost;
                    extractMetric(metricPoints, mp2.m_point->getNode(), mergePixels, &map, *newTripleIter.first, extraMetricCost, map.getSpacing());
                    for (auto &pixel : mergePixels) {
                        parents[pixel.pixel] = mp.m_point->getMergePixel();
                    }
                    mp2.m_misc = ~0;
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
            MetricPoint &mp = getMetricPoint(metricPoints, pix);
            mp.m_misc = 0;
            mp.m_dist = -1.0f;
            mp.m_cumangle = 0.0f;
        }


        int counter = 0;
        AttributeRow& lastPixelRow = attributes.getRow(AttributeKey(pixelTo));
        lastPixelRow.setValue(order_col, counter);
        counter++;
        auto currParent = pixelToParent;
        counter++;
        while (currParent != parents.end()) {
            MetricPoint &mp = getMetricPoint(metricPoints, currParent->second);
            AttributeRow& row = attributes.getRow(AttributeKey(currParent->second));
            row.setValue(order_col, counter);

            if (!mp.m_point->getMergePixel().empty() && mp.m_point->getMergePixel() == currParent->first) {
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
                        MetricPoint &lp = getMetricPoint(metricPoints, linePixel);
                        extractMetric(metricPoints, lp.m_point->getNode(), newPixels, &map,
                                      MetricTriple(0.0f, linePixel, NoPixel), 0, map.getSpacing());
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

void VGAMetricShortestPath::extractMetric(depthmapX::ColumnMatrix<MetricPoint> &metricPoints,
                                          Node n, std::set<MetricTriple> &pixels, PointMap* pointdata,
                                          const MetricTriple &curs, float extraMetricCost, float spacing) {
    MetricPoint & cursMP = getMetricPoint(metricPoints, curs.pixel);
    if (curs.dist == 0.0f || cursMP.m_point->blocked() ||
            pointdata->blockedAdjacent(curs.pixel)) {
        for (int i = 0; i < 32; i++) {
            Bin &bin = n.bin(i);
            for (auto pixVec : bin.m_pixel_vecs) {
                for (PixelRef pix = pixVec.start(); pix.col(bin.m_dir) <= pixVec.end().col(bin.m_dir);) {
                    MetricPoint &mpt = getMetricPoint(metricPoints, pix);
                    if (mpt.m_misc == 0 &&
                            (mpt.m_dist == -1.0 ||(extraMetricCost + curs.dist + spacing*dist(pix, curs.pixel) < mpt.m_dist))) {
                        mpt.m_dist = extraMetricCost + curs.dist + spacing*(float)dist(pix, curs.pixel);
                        // n.b. dmap v4.06r now sets angle in range 0 to 4 (1 = 90 degrees)
                        mpt.m_cumangle = cursMP.m_cumangle +
                                        (curs.lastpixel == NoPixel
                                             ? 0.0f
                                             : (float)(angle(pix, curs.pixel, curs.lastpixel) / (M_PI * 0.5)));
                        pixels.insert(MetricTriple(mpt.m_dist, pix, curs.pixel));
                    }
                    pix.move(bin.m_dir);
                }
            }
        }
    }
}
