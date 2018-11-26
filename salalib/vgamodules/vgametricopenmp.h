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

#include <unordered_set>

class VGAMetricOpenMP : IVGA {
  public:
    struct NewNode {
        int idx, merge = -1;
        PixelRef ref;
        std::vector<int> hood;
        bool edge;
        NewNode(PixelRef r = NoPixel) { ref = r; }
    };
    struct MetricIntermediateData {
        bool seen = false;
        float dist = -1.0f, cumangle = 0.0f;
        NewNode *n;
    };

    static MetricIntermediateData noPixelNode;
    struct NewMetricTriple {
        float dist;
        MetricIntermediateData *mid;
        PixelRef lastpixel;
        NewMetricTriple(float d = 0.0f, MetricIntermediateData *m = &noPixelNode, PixelRef lp = NoPixel) {
            dist = d;
            mid = m;
            lastpixel = lp;
        }
        bool operator==(const NewMetricTriple &mp2) const {
            return (dist == mp2.dist && mid->n->ref == mp2.mid->n->ref);
        }
        bool operator<(const NewMetricTriple &mp2) const {
            return (dist < mp2.dist) || (dist == mp2.dist && mid->n->ref < mp2.mid->n->ref);
        }
        bool operator>(const NewMetricTriple &mp2) const {
            return (dist > mp2.dist) || (dist == mp2.dist && mid->n->ref > mp2.mid->n->ref);
        }
        bool operator!=(const NewMetricTriple &mp2) const {
            return (dist != mp2.dist) || (mid->n->ref != mp2.mid->n->ref);
        }
    };

    struct NewMetricTripleHasher
    {
    };

    std::vector<NewNode> getNodes(PointMap &map);
    std::vector<MetricIntermediateData> getSourceData(const std::vector<NewNode> &nodes);

  public:
    std::string getAnalysisName() const override { return "Metric Analysis (OpenMP)"; }
    bool run(Communicator *comm, const Options &options, PointMap &map, bool simple_version) override;
    void extractMetric(std::set<NewMetricTriple> &pixels, const NewMetricTriple &curs,
                       std::vector<MetricIntermediateData> &iData);
};
