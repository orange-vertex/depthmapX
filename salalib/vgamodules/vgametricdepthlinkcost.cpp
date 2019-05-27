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

#include "salalib/vgamodules/vgametricdepthlinkcost.h"

#include "genlib/stringutils.h"

bool VGAMetricDepthLinkCost::run(Communicator *comm, PointMap &map, bool) {

    AttributeTable &attributes = map.getAttributeTable();

    // custom linking costs from the attribute table
    int link_metric_cost_col = attributes.getOrInsertColumn("Link Metric Cost");

    int path_length_col = attributes.insertOrResetColumn("Metric Step Depth");

    depthmapX::ColumnMatrix<MetricPoint> metricPoints(map.getRows(), map.getCols());

    for (auto &row : attributes) {
        PixelRef pix = PixelRef(row.getKey().value);
        MetricPoint &pnt = getMetricPoint(metricPoints, pix);
        pnt.m_point = &(map.getPoint(pix));
        if (link_metric_cost_col != -1) {
            float linkCost = row.getRow().getValue(link_metric_cost_col);
            if (linkCost > 0)
                pnt.m_linkCost += linkCost;
        }
    }

    // in order to calculate Penn angle, the MetricPair becomes a metric triple...
    std::set<MetricTriple> search_list; // contains root point

    for (auto &sel : m_pixelsFrom) {
        search_list.insert(MetricTriple(0.0f, sel, NoPixel));
    }

    while (search_list.size()) {
        std::set<MetricTriple>::iterator it = search_list.begin();
        MetricTriple here = *it;
        search_list.erase(it);
        MetricPoint &mp = getMetricPoint(metricPoints, here.pixel);
        // nb, the filled check is necessary as diagonals seem to be stored with 'gaps' left in
        if (mp.m_point->filled() && (mp.m_dist == -1.0 || (here.dist < mp.m_dist))) {
            extractMetric(mp.m_point->getNode(), metricPoints, search_list, &map, here);
            mp.m_dist = here.dist;
            if (!mp.m_point->getMergePixel().empty()) {
                MetricPoint &mp2 = getMetricPoint(metricPoints, mp.m_point->getMergePixel());
                if ((mp2.m_dist == -1.0 || (here.dist + mp2.m_linkCost < mp2.m_dist))) {

                    mp2.m_dist = here.dist + mp2.m_linkCost;

                    extractMetric(mp2.m_point->getNode(), metricPoints, search_list, &map,
                                  MetricTriple(mp2.m_dist, mp.m_point->getMergePixel(), NoPixel));
                }
            }
        }
    }

    for (auto &row : attributes) {
        PixelRef pix = PixelRef(row.getKey().value);
        MetricPoint &pnt = getMetricPoint(metricPoints, pix);
        row.getRow().setValue(path_length_col, float(pnt.m_dist));
    }
    map.setDisplayedAttribute(-2);
    map.setDisplayedAttribute(path_length_col);

    return true;
}

void VGAMetricDepthLinkCost::extractMetric(Node n, depthmapX::ColumnMatrix<MetricPoint> metricPoints,
                                           std::set<MetricTriple> &pixels, PointMap *pointdata,
                                           const MetricTriple &curs) {
    MetricPoint &cursMP = getMetricPoint(metricPoints, curs.pixel);
    if (curs.dist == 0.0f || cursMP.m_point->blocked() || pointdata->blockedAdjacent(curs.pixel)) {
        for (int i = 0; i < 32; i++) {
            Bin &bin = n.bin(i);
            for (auto pixVec : bin.m_pixel_vecs) {
                for (PixelRef pix = pixVec.start(); pix.col(bin.m_dir) <= pixVec.end().col(bin.m_dir);) {
                    MetricPoint &mpt = getMetricPoint(metricPoints, pix);
                    if ((mpt.m_dist == -1.0 ||
                         (curs.dist + pointdata->getSpacing() * dist(pix, curs.pixel) < mpt.m_dist))) {
                        mpt.m_dist = curs.dist + pointdata->getSpacing() * (float)dist(pix, curs.pixel);
                        pixels.insert(MetricTriple(mpt.m_dist, pix, curs.pixel));
                    }
                    pix.move(bin.m_dir);
                }
            }
        }
    }
}
