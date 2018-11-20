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

#include "salalib/vgamodules/vgavisualshortestpath.h"

#include "genlib/stringutils.h"

bool VGAVisualShortestPath::run(Communicator *comm, const Options &options, PointMap &map, bool simple_version) {

    auto& attributes = map.getAttributeTable();
    auto& selection_set = map.getSelSet();

    int col = attributes.insertColumn("Visual Shortest Path");

    for (int i = 0; i < attributes.getRowCount(); i++) {
        PixelRef pix = attributes.getRowKey(i);
        map.getPoint(pix).m_misc = 0;
        map.getPoint(pix).m_extent = pix;
    }

    std::vector<PixelRefVector> search_tree;
    search_tree.push_back(PixelRefVector());
    if (selection_set.size() != 2) {
        throw depthmapX::RuntimeException("Two nodes must be selected");
    }
    PixelRef pixelFrom = *selection_set.begin();
    PixelRef pixelTo = *std::next(selection_set.begin());

    search_tree.back().push_back(pixelFrom);

    size_t level = 0;
    std::map<PixelRef, PixelRef> parents;
    while (search_tree[level].size()) {
        search_tree.push_back(PixelRefVector());
        auto &currLevelPix = search_tree[level];
        auto &nextLevelPix = search_tree[level + 1];
        for (auto iter = currLevelPix.rbegin(); iter != currLevelPix.rend(); ++iter) {
            PixelRefVector newPixels;
            Point &p = map.getPoint(*iter);
            if (p.filled() && p.m_misc != ~0) {
                if (!p.contextfilled() || iter->iseven() || level == 0) {
                    p.getNode().extractUnseen(newPixels, &map, p.m_misc);
                    p.m_misc = ~0;
                    if (!p.getMergePixel().empty()) {
                        Point &p2 = map.getPoint(p.getMergePixel());
                        if (p2.m_misc != ~0) {
                            p2.getNode().extractUnseen(newPixels, &map, p2.m_misc); // did say p.misc
                            p2.m_misc = ~0;
                        }
                    }
                } else {
                    p.m_misc = ~0;
                }
            }

            for (auto &pixel : newPixels) {
                parents[pixel] = *iter;
            }
            nextLevelPix.insert(nextLevelPix.end(), newPixels.begin(), newPixels.end());
        }
        for (auto iter = nextLevelPix.rbegin(); iter != nextLevelPix.rend(); ++iter) {
            if (*iter == pixelTo) {
                int counter = 0;
                int row = attributes.getRowid(*iter);
                attributes.setValue(row, col, counter);
                counter++;
                auto currParent = parents.find(*iter);
                while (currParent != parents.end()) {
                    int row = attributes.getRowid(currParent->second);
                    attributes.setValue(row, col, counter);
                    currParent = parents.find(currParent->second);
                    counter++;
                }
            }
        }
        level++;
    }

    map.setDisplayedAttribute(-2);
    map.setDisplayedAttribute(col);

    return true;
}

void VGAVisualShortestPath::extractUnseen(Node &node, PixelRefVector &pixels, depthmapX::RowMatrix<int> &miscs,
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
