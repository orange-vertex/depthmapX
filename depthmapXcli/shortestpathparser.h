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

#include "imodeparser.h"
#include "genlib/p2dpoly.h"
#include <vector>

class ShortestPathParser : public IModeParser
{
public:
    ShortestPathParser() : m_shortestPathType(ShortestPathType::NONE)
    {}

    virtual std::string getModeName() const
    {
        return "SHORTESTPATH";
    }

    virtual std::string getHelp() const
    {
        return "Mode options for pointmap SHORTESTPATH are:\n" \
               "  -spo <point> Origin point\n" \
               "  -spd <point> Destination point\n" \
               "  -spt <type> Shortest Path type. One of metric, angular or visual\n";
    }

    enum class ShortestPathType {
        NONE,
        ANGULAR,
        METRIC,
        VISUAL
    };

    virtual void parse(int argc, char** argv);

    virtual void run(const CommandLineParser &clp, IPerformanceSink &perfWriter) const;

    Point2f getOrigin() const { return m_origin; }
    Point2f getDestination() const { return m_destination; }
    ShortestPathType getShortestPathType() const { return m_shortestPathType; }

private:
    Point2f m_origin;
    Point2f m_destination;
    ShortestPathType m_shortestPathType;
};

