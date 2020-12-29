// Copyright (C) 2017 Petros Koutsolampros

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

#include "depthmapXcli/imodeparser.h"
#include "genlib/p2dpoly.h"
#include <vector>

class VGAParallelParser : public IModeParser {
  public:
    VGAParallelParser() : m_stepType(StepType::NONE) {}

    virtual std::string getModeName() const { return "SEGMENTSHORTESTPATH"; }

    virtual std::string getHelp() const {
        return "Mode options for pointmap SEGMENTSHORTESTPATH are:\n"
               "  -sspo <shortest path origin point> point where to calculate shortest path between.\n"
               "  -sspd <shortest path destination point> point where to calculate shortest path between.\n"
               "  -sspt <type> step type. One of metric, tulip or topological.\n";
    }

    enum class StepType { NONE, TULIP, METRIC, TOPOLOGICAL };

    virtual void parse(int argc, char **argv);

    virtual void run(const CommandLineParser &clp, IPerformanceSink &perfWriter) const;

    Point2f getShortestPathOrigin() const { return m_originPoint; }
    Point2f getShortestPathDestination() const { return m_destinationPoint; }

    StepType getStepType() const { return m_stepType; }

  private:
    Point2f m_originPoint;
    Point2f m_destinationPoint;

    StepType m_stepType;
};
