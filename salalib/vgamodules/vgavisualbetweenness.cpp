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

#include "salalib/vgamodules/vgavisualbetweenness.h"

#include "genlib/stringutils.h"

bool VGAVisualBetweenness::run(Communicator *comm, const Options &options, PointMap &map, bool simple_version) {
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

    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_STEPS, 1);
        comm->CommPostMessage(Communicator::CURRENT_STEP, 1);
        comm->CommPostMessage(Communicator::NUM_RECORDS, filled.size());
    }
    std::vector<float> betwn_col_data(filled.size());

    depthmapX::RowMatrix<int> counts(map.getRows(), map.getCols());
    counts.initialiseValues(0);
    depthmapX::RowMatrix<int> vals(map.getRows(), map.getCols());
    vals.initialiseValues(0);
//    for (int i = 0; i < filled.size(); i++) {
    {
        int i = 0;

//        if ((map.getPoint(filled[i]).contextfilled() && !filled[i].iseven()) || (options.gates_only)) {
//            count++;
//            //continue;
//        }

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

        int& sourceVal = vals(filled[i].y, filled[i].x) ;
        int& sourceCount = counts(filled[i].y, filled[i].x);
        sourceVal = 1;
        sourceCount = 1;

        int level = 0;
        while (search_tree[level].size()) {
            search_tree.push_back(PixelRefVector());
            distribution.push_back(0);
            for (size_t n = search_tree[level].size() - 1; n != paftl::npos; n--) {
                PixelRef curr = search_tree[level][n];
                Point &p = map.getPoint(curr);
                int &p1misc = miscs(curr.y, curr.x);
                int& p1Val = vals(curr.y, curr.x);
                int& p1Count = counts(curr.y, curr.x);
                if (p.filled() && p1misc != ~0) {
                    total_depth += level;
                    total_nodes += 1;
                    distribution.back() += 1;
                    if ((int)options.radius == -1 ||
                        level < (int)options.radius && (!p.contextfilled() || search_tree[level][n].iseven())) {
                        extractUnseen(p.getNode(), search_tree[level + 1], miscs, extents, vals, counts, p1Val, p1Count);
                        p1misc = ~0;
                        if (!p.getMergePixel().empty()) {
                            Point &p2 = map.getPoint(p.getMergePixel());
                            int &p2misc = miscs(p.getMergePixel().y, p.getMergePixel().x);
                            int& p2Val = vals(p.getMergePixel().y, p.getMergePixel().x);
                            int& p2Count = counts(p.getMergePixel().y, p.getMergePixel().x);
                            if (p2misc != ~0) {
                                extractUnseen(p2.getNode(), search_tree[level + 1], miscs, extents, vals, counts, p2Val, p2Count);
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

        betwn_col_data[i] = float(total_nodes); // note: total nodes includes this one;

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

    std::string betwn_col_text = std::string("Visual Integration [HH]");
    int betwn_col = attributes.insertColumn(betwn_col_text.c_str());

    for (size_t i = 0; i < rows.size(); i++) {
        attributes.setValue(rows[i], betwn_col, counts(filled[i].y, filled[i].x));
//        attributes.setValue(rows[i], betwn_col, betwn_col_data[i]);
    }

    map.overrideDisplayedAttribute(-2);
    map.setDisplayedAttribute(betwn_col);

    return true;
}

void VGAVisualBetweenness::extractUnseen(Node &node, PixelRefVector &pixels, depthmapX::RowMatrix<int> &miscs,
                                         depthmapX::RowMatrix<PixelRef> &extents, depthmapX::RowMatrix<int>& vals,
                                         depthmapX::RowMatrix<int>& counts, int& sourceVal, int& sourceCount) {
    for (int i = 0; i < 32; i++) {
        Bin &bin = node.bin(i);
        for (auto pixVec : bin.m_pixel_vecs) {
            for (PixelRef pix = pixVec.start(); pix.col(bin.m_dir) <= pixVec.end().col(bin.m_dir);) {
                int &misc = miscs(pix.y, pix.x);
                int &currCount = counts(pix.y, pix.x);
                int &currVal = vals(pix.y, pix.x);
                PixelRef &extent = extents(pix.y, pix.x);
                if (misc == 0) {
                    pixels.push_back(pix);
                    misc |= (1 << i);
                    currVal = sourceVal+1;
                    currCount = sourceCount;
                } else if(currVal == sourceVal+1) {
                    currCount = currCount + sourceCount;
                } else if(currVal > sourceVal+1) {
                    currVal = sourceVal+1;
                    currCount = sourceCount;
                }
                // 10.2.02 revised --- diagonal was breaking this as it was extent in diagonal or horizontal
                if (!(bin.m_dir & PixelRef::DIAGONAL)) {
                    if (extent.col(bin.m_dir) >= pixVec.end().col(bin.m_dir))
                        break;
                    extent.col(bin.m_dir) = pixVec.end().col(bin.m_dir);
                }
                pix.move(bin.m_dir);
            }
        }
    }
}
