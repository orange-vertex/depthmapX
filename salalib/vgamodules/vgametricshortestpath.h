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

class VGAMetricShortestPath : IVGA {
private:
    struct MetricPoint {
        Point* m_point;
        int m_misc = 0;
        float m_dist = -1.0f;
        float m_cumangle = 0.0f;
        float m_cumdist = 0.0f;
    };
    MetricPoint& getMetricPoint(depthmapX::ColumnMatrix<MetricPoint> &metricPoints, PixelRef ref) {
        return(metricPoints(static_cast<size_t>(ref.y), static_cast<size_t>(ref.x)));
    }
  public:
    std::string getAnalysisName() const override { return "Metric Shortest Path"; }
    bool run(Communicator *comm, const Options &options, PointMap &map, bool) override;
    void extractMetric(depthmapX::ColumnMatrix<MetricPoint> &metricPoints,
                       Node n, std::set<MetricTriple> &pixels, PointMap *pointdata, const MetricTriple &curs,
                       float extraMetricCost, float spacing);
};
