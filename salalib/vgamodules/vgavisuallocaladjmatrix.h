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

#pragma once

#include "salalib/ivga.h"
#include "salalib/options.h"
#include "salalib/pixelref.h"
#include "salalib/pointdata.h"

class VGAVisualLocalAdjMatrix : IVGA {
  public:
    std::string getAnalysisName() const override { return "Local Visibility Analysis (OpenMP)"; }
    bool run(Communicator *comm, const Options &options, PointMap &map, bool simple_version) override;
    void dumpNeighbourhood(Node &node, std::set<PixelRef> &hood) const {
        for (int i = 0; i < 32; i++) {
            Bin &bin = node.bin(i);
            for (auto pixVec : bin.m_pixel_vecs) {
                for (PixelRef pix = pixVec.start(); pix.col(bin.m_dir) <= pixVec.end().col(bin.m_dir);) {
                    hood.insert(pix);
                    pix.move(bin.m_dir);
                }
            }
        }
    }
};
