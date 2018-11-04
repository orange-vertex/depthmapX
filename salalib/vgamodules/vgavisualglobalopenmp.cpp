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

#include "salalib/vgamodules/vgavisualglobalopenmp.h"

#include "genlib/stringutils.h"

bool VGAVisualGlobalOpenMP::run(Communicator *comm, const Options &options, PointMap &map, bool simple_version) {
    time_t atime = 0;

    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, map.getFilledPointCount());
    }

    AttributeTable &attributes = map.getAttributeTable();

    std::vector<PixelRef> filled;
    std::vector<int> rows;

    for (int i = 0; i < map.getCols(); i++) {
        for (int j = 0; j < map.getRows(); j++) {
            PixelRef curs = PixelRef(i, j);
            if (map.getPoint(curs).filled()) {
                filled.push_back(curs);
                rows.push_back(attributes.getRowid(curs));
            }
        }
    }

    int count = 0;

    if (options.global) {
        if (comm) {
            qtimer(atime, 0);
            comm->CommPostMessage(Communicator::NUM_STEPS, 1);
            comm->CommPostMessage(Communicator::CURRENT_STEP, 1);
            comm->CommPostMessage(Communicator::NUM_RECORDS, filled.size());
        }
        std::vector<float> count_col_data(filled.size());
        std::vector<float> depth_col_data(filled.size());
        std::vector<float> integ_dv_col_data(filled.size());
        std::vector<float> integ_pv_col_data(filled.size());
        std::vector<float> integ_tk_col_data(filled.size());
        std::vector<float> entropy_col_data(filled.size());
        std::vector<float> rel_entropy_col_data(filled.size());

#pragma omp parallel for
        for (int i = 0; i < filled.size(); i++) {

            if ((map.getPoint(filled[i]).contextfilled() && !filled[i].iseven()) || (options.gates_only)) {
                count++;
                continue;
            }

            depthmapX::RowMatrix<int> miscs(map.getRows(), map.getCols());
            depthmapX::RowMatrix<PixelRef> extents(map.getRows(), map.getCols());

            for (int ii = 0; ii < map.getCols(); ii++) {
                for (int jj = 0; jj < map.getRows(); jj++) {
                    miscs(jj, ii) = 0;
                    extents(jj, ii) = PixelRef(ii, jj);
                }
            }

            int total_depth = 0;
            int total_nodes = 0;

            std::vector<int> distribution;
            std::vector<PixelRefVector> search_tree;
            search_tree.push_back(PixelRefVector());
            search_tree.back().push_back(filled[i]);

            int level = 0;
            while (search_tree[level].size()) {
                search_tree.push_back(PixelRefVector());
                distribution.push_back(0);
                for (size_t n = search_tree[level].size() - 1; n != paftl::npos; n--) {
                    PixelRef curr = search_tree[level][n];
                    Point &p = map.getPoint(curr);
                    int &p1misc = miscs(curr.y, curr.x);
                    if (p.filled() && p1misc != ~0) {
                        total_depth += level;
                        total_nodes += 1;
                        distribution.back() += 1;
                        if ((int)options.radius == -1 ||
                            level < (int)options.radius && (!p.contextfilled() || search_tree[level][n].iseven())) {
                            extractUnseen(p.getNode(), search_tree[level + 1], miscs, extents);
                            p1misc = ~0;
                            if (!p.getMergePixel().empty()) {
                                Point &p2 = map.getPoint(p.getMergePixel());
                                int &p2misc = miscs(p.getMergePixel().y, p.getMergePixel().x);
                                if (p2misc != ~0) {
                                    extractUnseen(p.getNode(), search_tree[level + 1], miscs, extents);
                                    p2misc = ~0;
                                }
                            }
                        } else {
                            p1misc = ~0;
                        }
                    }
                    search_tree[level].pop_back();
                }
                level++;
            }

            // only set to single float precision after divide
            // note -- total_nodes includes this one -- mean depth as per p.108 Social Logic of Space

            count_col_data[i] = float(total_nodes); // note: total nodes includes this one;

            // ERROR !!!!!!
            if (total_nodes > 1) {
                double mean_depth = double(total_depth) / double(total_nodes - 1);
                depth_col_data[i] = float(mean_depth);
                // total nodes > 2 to avoid divide by 0 (was > 3)
                if (total_nodes > 2 && mean_depth > 1.0) {
                    double ra = 2.0 * (mean_depth - 1.0) / double(total_nodes - 2);
                    // d-value / p-values from Depthmap 4 manual, note: node_count includes this one
                    double rra_d = ra / dvalue(total_nodes);
                    double rra_p = ra / pvalue(total_nodes);
                    double integ_tk = teklinteg(total_nodes, total_depth);
                    integ_dv_col_data[i] = float(1.0 / rra_d);
                    integ_pv_col_data[i] = float(1.0 / rra_p);

                    if (total_depth - total_nodes + 1 > 1) {
                        integ_tk_col_data[i] = float(integ_tk);
                    } else {
                        integ_tk_col_data[i] = -1.0f;
                    }
                } else {
                    integ_dv_col_data[i] = -1.0f;
                    integ_pv_col_data[i] = -1.0f;
                    integ_tk_col_data[i] = -1.0f;
                }
                double entropy = 0.0, rel_entropy = 0.0, factorial = 1.0;
                // n.b., this distribution contains the root node itself in distribution[0]
                // -> chopped from entropy to avoid divide by zero if only one node
                for (size_t k = 1; k < distribution.size(); k++) {
                    if (distribution[k] > 0) {
                        double prob = double(distribution[k]) / double(total_nodes - 1);
                        entropy -= prob * log2(prob);
                        // Formula from Turner 2001, "Depthmap"
                        factorial *= double(k + 1);
                        double q = (pow(mean_depth, double(k)) / double(factorial)) * exp(-mean_depth);
                        rel_entropy += (float)prob * log2(prob / q);
                    }
                }
                entropy_col_data[i] = float(entropy);
                rel_entropy_col_data[i] = float(rel_entropy);
            } else {
                depth_col_data[i] = -1.0f;
                entropy_col_data[i] = -1.0f;
                rel_entropy_col_data[i] = -1.0f;
            }
            count++; // <- increment count

            if (comm) {
                if (qtimer(atime, 500)) {
                    if (comm->IsCancelled()) {
                        throw Communicator::CancelledException();
                    }
                    comm->CommPostMessage(Communicator::CURRENT_RECORD, count);
                }
            }

            // kept to achieve parity in binary comparison with old versions
            // TODO: Remove at next version of .graph file
            size_t filledIdx = size_t(filled[i].y * map.getCols() + filled[i].x);
            map.getPoint(filled[i]).m_misc = miscs(filled[i].y, filled[i].x);
            map.getPoint(filled[i]).m_extent = extents(filled[i].y, filled[i].x);
        }

        int entropy_col, rel_entropy_col, integ_dv_col, integ_pv_col, integ_tk_col, depth_col, count_col;

        std::string radius_text;
        if (options.radius != -1) {
            radius_text = std::string(" R") + dXstring::formatString(int(options.radius), "%d");
        }

        // n.b. these must be entered in alphabetical order to preserve col indexing:
        // dX simple version test // TV
        if (!simple_version) {
            std::string entropy_col_text = std::string("Visual Entropy") + radius_text;
            entropy_col = attributes.insertColumn(entropy_col_text.c_str());
        }

        std::string integ_dv_col_text = std::string("Visual Integration [HH]") + radius_text;
        integ_dv_col = attributes.insertColumn(integ_dv_col_text.c_str());

        if (!simple_version) {
            std::string integ_pv_col_text = std::string("Visual Integration [P-value]") + radius_text;
            integ_pv_col = attributes.insertColumn(integ_pv_col_text.c_str());
            std::string integ_tk_col_text = std::string("Visual Integration [Tekl]") + radius_text;
            integ_tk_col = attributes.insertColumn(integ_tk_col_text.c_str());
            std::string depth_col_text = std::string("Visual Mean Depth") + radius_text;
            depth_col = attributes.insertColumn(depth_col_text.c_str());
            std::string count_col_text = std::string("Visual Node Count") + radius_text;
            count_col = attributes.insertColumn(count_col_text.c_str());
            std::string rel_entropy_col_text = std::string("Visual Relativised Entropy") + radius_text;
            rel_entropy_col = attributes.insertColumn(rel_entropy_col_text.c_str());
        }

        for (size_t i = 0; i < rows.size(); i++) {
            attributes.setValue(rows[i], integ_dv_col, integ_dv_col_data[i]);
            attributes.setValue(rows[i], integ_pv_col, integ_pv_col_data[i]);
            attributes.setValue(rows[i], integ_tk_col, integ_tk_col_data[i]);
            if (!simple_version) {
                attributes.setValue(rows[i], count_col, count_col_data[i]);
                attributes.setValue(rows[i], depth_col, depth_col_data[i]);
                attributes.setValue(rows[i], entropy_col, entropy_col_data[i]);
                attributes.setValue(rows[i], rel_entropy_col, rel_entropy_col_data[i]);
            }
        }

        map.setDisplayedAttribute(integ_dv_col);
    }

    return true;
}
