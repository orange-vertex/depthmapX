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

#include "salalib/vgamodules/vgametricopenmp.h"

#include "genlib/stringutils.h"

bool VGAMetricOpenMP::run(Communicator *comm, const Options &options, PointMap &map, bool simple_version) {
    time_t atime = 0;

    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, map.getFilledPointCount());
    }

    AttributeTable &attributes = map.getAttributeTable();

    const std::vector<NewNode> nodes = getNodes(map);
    std::vector<int> rows;

    for (auto &n : nodes) {
        rows.push_back(attributes.getRowid(n.ref));
    }

    int lastFilledIdx = nodes.back().ref;

    int count = 0;

    struct MetricColData {
        float mspa, mspl, dist, count;
    };

    std::vector<MetricColData> colData(nodes.size());

    const std::vector<MetricIntermediateData> sourceData = getSourceData(nodes);

    int i, N = nodes.size();
#pragma omp parallel for default(shared) private(i) schedule(dynamic)
    for (i = 0; i < N; i++) {
        if (options.gates_only) {
            count++;
            continue;
        }

        std::vector<MetricIntermediateData> iData = sourceData;

        MetricIntermediateData &currData = iData[i];

        float euclid_depth = 0.0f;
        float total_depth = 0.0f;
        float total_angle = 0.0f;
        int total_nodes = 0;

        // note that m_misc is used in a different manner to analyseGraph / PointDepth
        // here it marks the node as used in calculation only

        std::set<NewMetricTriple> search_list;
        search_list.insert(NewMetricTriple(0.0f, &currData, NoPixel));
        while (search_list.size()) {
            std::set<NewMetricTriple>::iterator it = search_list.begin();
            NewMetricTriple here = *it;
            search_list.erase(it);
            if (int(options.radius) != -1 && double(here.dist) * map.getSpacing() > options.radius) {
                break;
            }
            auto &filledData = iData[here.mid->n->idx];
            bool &p1seen = filledData.seen;
            float &p1cumangle = filledData.cumangle;
            // nb, the filled check is necessary as diagonals seem to be stored with 'gaps' left in
            if (!p1seen) {
                extractMetric(search_list, here, iData);
                p1seen = true;
                if (here.mid->n->merge != -1) {
                    auto &linkData = iData[here.mid->n->merge];
                    bool &p2seen = linkData.seen;
                    float &p2cumangle = linkData.cumangle;
                    if (!p2seen) {
                        p2cumangle = p1cumangle;
                        extractMetric(search_list, NewMetricTriple(here.dist, &linkData, NoPixel), iData);
                        p2seen = true;
                    }
                }
                total_depth += here.dist * float(map.getSpacing());
                total_angle += p1cumangle;
                euclid_depth += float(map.getSpacing() * dist(here.mid->n->ref, currData.n->ref));
                total_nodes += 1;
            }
        }

        if (lastFilledIdx == i) {
            // kept to achieve parity in binary comparison with old versions
            // TODO: Remove at next version of .graph file
            NewNode &n = nodes[size_t(i)];
            auto &filledData = iData[n.idx];
            map.getPoint(n.ref).m_misc = static_cast<int>(filledData.seen);
            map.getPoint(n.ref).m_dist = filledData.dist;
            map.getPoint(n.ref).m_cumangle = filledData.cumangle;
        }

        auto &rowData = colData[static_cast<size_t>(i)];
        rowData.mspa = float(double(total_angle) / double(total_nodes));
        rowData.mspl = float(double(total_depth) / double(total_nodes));
        rowData.dist = float(double(euclid_depth) / double(total_nodes));
        rowData.count = float(total_nodes);

#pragma omp critical(count)
        count++; // <- increment count

        if (comm) {
            if (qtimer(atime, 500)) {
                if (comm->IsCancelled()) {
                    throw Communicator::CancelledException();
                }
                comm->CommPostMessage(Communicator::CURRENT_RECORD, count);
            }
        }
    }

    std::string radius_text;
    if (int(options.radius) != -1) {
        if (options.radius > 100.0) {
            radius_text = std::string(" R") + dXstring::formatString(options.radius, "%.f");
        } else if (map.getRegion().width() < 1.0) {
            radius_text = std::string(" R") + dXstring::formatString(options.radius, "%.4f");
        } else {
            radius_text = std::string(" R") + dXstring::formatString(options.radius, "%.2f");
        }
    }
    // n.b. these must be entered in alphabetical order to preserve col indexing:
    std::string mspa_col_text = std::string("Metric Mean Shortest-Path Angle") + radius_text;
    int mspa_col = attributes.insertColumn(mspa_col_text.c_str());
    std::string mspl_col_text = std::string("Metric Mean Shortest-Path Distance") + radius_text;
    int mspl_col = attributes.insertColumn(mspl_col_text.c_str());
    std::string dist_col_text = std::string("Metric Mean Straight-Line Distance") + radius_text;
    int dist_col = attributes.insertColumn(dist_col_text.c_str());
    std::string count_col_text = std::string("Metric Node Count") + radius_text;
    int count_col = attributes.insertColumn(count_col_text.c_str());

    for (size_t i = 0; i < rows.size(); i++) {
        auto &rowData = colData[static_cast<size_t>(i)];
        attributes.setValue(rows[i], mspa_col, rowData.mspa);
        attributes.setValue(rows[i], mspl_col, rowData.mspl);
        attributes.setValue(rows[i], dist_col, rowData.dist);
        attributes.setValue(rows[i], count_col, rowData.count);
    }

    map.overrideDisplayedAttribute(-2);
    map.setDisplayedAttribute(mspl_col);

    return true;
}

void VGAMetricOpenMP::extractMetric(std::set<NewMetricTriple> &pixels,
                                    const NewMetricTriple &curs, std::vector<MetricIntermediateData> &iData) {
    float iH_PI = 1.0f / (M_PI * .5);
    if (curs.dist == 0.0f || curs.mid->n->edge) {
        for (auto &pix : curs.mid->n->hood) {
            auto &pixData = iData[pix];
            float &pixdist = pixData.dist;
            float newDist = dist(pixData.n->ref, curs.mid->n->ref);
            if (!pixData.seen && (pixdist == -1.0 || (curs.dist + newDist < pixdist))) {
                pixdist = curs.dist + newDist;
                // n.b. dmap v4.06r now sets angle in range 0 to 4 (1 = 90 degrees)
                pixData.cumangle = pixData.cumangle +
                                   (curs.lastpixel == NoPixel
                                        ? 0.0f
                                        : (float)(angle(pixData.n->ref, curs.mid->n->ref, curs.lastpixel) * iH_PI));
                pixels.insert(NewMetricTriple(pixdist, &pixData, curs.mid->n->ref));
            }
        }
    }
}

std::vector<VGAMetricOpenMP::NewNode> VGAMetricOpenMP::getNodes(PointMap &map) {
    std::map<PixelRef, int> filledRefs;
    std::vector<NewNode> nodes(map.getFilledPointCount());
    //    depthmapX::RowMatrix<int> filledMap(map.getRows(), map.getCols());

    int filledCounter = 0;
    for (short ii = 0; ii < map.getCols(); ii++) {
        for (short jj = 0; jj < map.getRows(); jj++) {
            PixelRef curs = PixelRef(ii, jj);
            Point &p = map.getPoint(curs);
            if (p.filled()) {
                filledRefs[curs] = filledCounter;
                NewNode n;
                n.ref = curs;
                n.idx = filledCounter;
                n.edge = map.getPoint(curs).blocked() || map.blockedAdjacent(curs);
                nodes[filledCounter] = n;
                filledCounter++;
            }
        }
    }

    int i, N = int(nodes.size());
#pragma omp parallel for default(shared) private(i) schedule(dynamic)
    for (i = 0; i < N; i++) {
        auto &n = nodes[i];
        Point &p = map.getPoint(n.ref);
        if (!p.getMergePixel().empty())
            n.merge = filledRefs[p.getMergePixel()];
        Node &node = p.getNode();
        for (int i = 0; i < 32; i++) {
            Bin &bin = node.bin(i);
            for (auto pixVec : bin.m_pixel_vecs) {
                for (PixelRef pix = pixVec.start(); pix.col(bin.m_dir) <= pixVec.end().col(bin.m_dir);) {
                    n.hood.push_back(filledRefs[pix]);
                    pix.move(bin.m_dir);
                }
            }
        }
    }
    return nodes;
}

std::vector<VGAMetricOpenMP::MetricIntermediateData>
VGAMetricOpenMP::getSourceData(const std::vector<NewNode> &nodes) {

    std::vector<MetricIntermediateData> sourceData(nodes.size());

    int i, N = nodes.size();
#pragma omp parallel for default(shared) private(i) schedule(dynamic)
    for (i = 0; i < N; i++) {
        sourceData[i].n = &nodes[i];
    }
    return sourceData;
}
