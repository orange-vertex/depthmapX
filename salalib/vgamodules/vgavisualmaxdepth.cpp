// sala - a component of the depthmapX - spatial network analysis platform
// Copyright (C) 2019, Petros Koutsolampros

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

#include "salalib/vgamodules/vgavisualmaxdepth.h"

#include "genlib/stringutils.h"

bool VGAVisualMaxDepth::run(Communicator *comm, PointMap &map, bool) {
    time_t atime = 0;
    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, map.getFilledPointCount());
    }
    AttributeTable &attributes = map.getAttributeTable();

    std::string radius_text;
    if (m_radius != -1) {
        radius_text = std::string(" R") + dXstring::formatString(int(m_radius), "%d");
    }

    std::string maxdepth_col_text = std::string("Visual Max Depth") + radius_text;
    int maxdepth_col = attributes.insertOrResetColumn(maxdepth_col_text.c_str());

    int count = 0;

    depthmapX::RowMatrix<int> miscs(map.getRows(), map.getCols());
    depthmapX::RowMatrix<PixelRef> extents(map.getRows(), map.getCols());

    for (int i = 0; i < map.getCols(); i++) {
        for (int j = 0; j < map.getRows(); j++) {
            PixelRef curs = PixelRef(i, j);
            if (map.getPoint(curs).filled()) {

                if ((map.getPoint(curs).contextfilled() && !curs.iseven())) {
                    count++;
                    continue;
                }

                for (int ii = 0; ii < map.getCols(); ii++) {
                    for (int jj = 0; jj < map.getRows(); jj++) {
                        miscs(jj, ii) = 0;
                        extents(jj, ii) = PixelRef(ii, jj);
                    }
                }

                std::vector<PixelRefVector> search_tree;
                search_tree.push_back(PixelRefVector());
                search_tree.back().push_back(curs);

                int depth = 0;
                int maxdepth = 0;
                while (search_tree[depth].size()) {
                    search_tree.push_back(PixelRefVector());
                    const PixelRefVector& searchTreeAtLevel = search_tree[depth];
                    auto currLvlIter = searchTreeAtLevel.rbegin();
                    for (; currLvlIter != searchTreeAtLevel.rend(); currLvlIter++) {
                        int &pmisc = miscs(currLvlIter->y, currLvlIter->x);
                        Point &p = map.getPoint(*currLvlIter);
                        if (p.filled() && pmisc != ~0) {
                            if(depth > maxdepth) {
                                maxdepth = depth;
                            }
                            if ((int) m_radius == -1 ||
                                (depth < (int) m_radius &&
                                    (!p.contextfilled() || currLvlIter->iseven()))) {
                                extractUnseen(p.getNode(), search_tree[depth + 1], miscs, extents);
                                pmisc = ~0;
                                if (!p.getMergePixel().empty()) {
                                    PixelRef mergePixel = p.getMergePixel();
                                    int &p2misc = miscs(mergePixel.y, mergePixel.x);
                                    Point &p2 = map.getPoint(mergePixel);
                                    if (p2misc != ~0) {
                                        extractUnseen(p2.getNode(), search_tree[depth + 1], miscs,
                                                      extents); // did say p.misc
                                        p2misc = ~0;
                                    }
                                }
                            } else {
                                pmisc = ~0;
                            }
                        }
                        search_tree[depth].pop_back();
                    }
                    depth++;
                }
                AttributeRow &row = attributes.getRow(AttributeKey(curs));
                row.setValue(maxdepth_col, float(maxdepth));

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
        }
    }
    map.setDisplayedAttribute(-2);
    map.setDisplayedAttribute(maxdepth_col);

    return true;
}

void VGAVisualMaxDepth::extractUnseen(Node &node, PixelRefVector &pixels, depthmapX::RowMatrix<int> &miscs,
                   depthmapX::RowMatrix<PixelRef> &extents) {
    for (int i = 0; i < 32; i++) {
        Bin &bin = node.bin(i);
        for (auto pixVec : bin.m_pixel_vecs) {
            for (PixelRef pix = pixVec.start(); pix.col(bin.m_dir) <= pixVec.end().col(bin.m_dir);) {
                int &misc = miscs(pix.y, pix.x);
                PixelRef &extent = extents(pix.y, pix.x);
                if (misc == 0) {
                    pixels.push_back(pix);
                    misc |= (1 << i);
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
