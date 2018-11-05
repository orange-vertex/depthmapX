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

#include "genlib/simplematrix.h"

class VGAMetricOpenMP : IVGA {
  public:
    std::string getAnalysisName() const override { return "Metric Analysis (OpenMP)"; }
    bool run(Communicator *comm, const Options &options, PointMap &map, bool simple_version) override;
    void extractMetric(Node &node, std::set<MetricTriple> &pixels, PointMap *pointdata, const MetricTriple &curs,
                       depthmapX::RowMatrix<int> &miscs, depthmapX::RowMatrix<float> &dists,
                       depthmapX::RowMatrix<float> &cumangles) {
        if (curs.dist == 0.0f || pointdata->getPoint(curs.pixel).blocked() || pointdata->blockedAdjacent(curs.pixel)) {
            for (int i = 0; i < 32; i++) {
                Bin &bin = node.bin(i);
                for (auto pixVec : bin.m_pixel_vecs) {
                    for (PixelRef pix = pixVec.start(); pix.col(bin.m_dir) <= pixVec.end().col(bin.m_dir);) {
                        float &pixdist = dists(pix.y, pix.x);
                        if (miscs(pix.y, pix.x) == 0 &&
                            (pixdist == -1.0 || (curs.dist + dist(pix, curs.pixel) < pixdist))) {
                            pixdist = curs.dist + (float)dist(pix, curs.pixel);
                            // n.b. dmap v4.06r now sets angle in range 0 to 4 (1 = 90 degrees)
                            cumangles(pix.y, pix.x) =
                                cumangles(curs.pixel.y, curs.pixel.x) +
                                (curs.lastpixel == NoPixel
                                     ? 0.0f
                                     : (float)(angle(pix, curs.pixel, curs.lastpixel) / (M_PI * 0.5)));
                            pixels.insert(MetricTriple(pixdist, pix, curs.pixel));
                        }
                        pix.move(bin.m_dir);
                    }
                }
            }
        }
    }
};