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

#pragma once

#include "salalib/ivga.h"
#include "salalib/pixelref.h"
#include "salalib/pointdata.h"

class VGAIsovistZone : IVGA {
  private:
    std::set<PixelRef> m_originPoints;
    float m_restrictDistance;

    struct MetricPoint {
        Point *m_point = nullptr;
    };
    MetricPoint &getMetricPoint(depthmapX::ColumnMatrix<MetricPoint> &metricPoints, PixelRef ref) {
        return (metricPoints(static_cast<size_t>(ref.y), static_cast<size_t>(ref.x)));
    }
    void extractMetric(Node n, std::set<MetricTriple> &pixels, PointMap &map, const MetricTriple &curs);
    void setColumnFormulaAndUpdate(PointMap &pointmap, int columnIndex, std::string formula, bool selectionOnly);

  public:
    std::string getAnalysisName() const override { return "Path Zone"; }
    bool run(Communicator *comm, PointMap &map, bool) override;
    VGAIsovistZone(std::set<PixelRef> originPoints, float restrictDistance = -1)
        : m_originPoints(originPoints), m_restrictDistance(restrictDistance) {}
};
