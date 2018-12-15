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

#include "shortestpathparser.h"
#include "exceptions.h"
#include "parsingutils.h"
#include "salalib/entityparsing.h"
#include "runmethods.h"
#include <sstream>
#include <cstring>

using namespace depthmapX;

void ShortestPathParser::parse(int argc, char ** argv)
{

    std::string origin;
    std::string destination;
    for ( int i = 1; i < argc; ++i )
    {
        if ( std::strcmp ("-spo", argv[i]) == 0)
        {
            ENFORCE_ARGUMENT("-spo", i)
            if (!has_only_digits_dots_commas(argv[i]))
            {
                std::stringstream message;
                message << "Invalid origin point provided ("
                        << argv[i]
                        << "). Should only contain digits dots and commas"
                        << std::flush;
                throw CommandLineException(message.str().c_str());
            }
            origin = argv[i];
        }
        else if ( std::strcmp ("-spd", argv[i]) == 0)
        {
            ENFORCE_ARGUMENT("-spd", i)
            if (!has_only_digits_dots_commas(argv[i]))
            {
                std::stringstream message;
                message << "Invalid destination point provided ("
                        << argv[i]
                        << "). Should only contain digits dots and commas"
                        << std::flush;
                throw CommandLineException(message.str().c_str());
            }
            destination = argv[i];
        }
        else if ( std::strcmp ("-spt", argv[i]) == 0)
        {
            ENFORCE_ARGUMENT("-spt", i)
            if ( std::strcmp(argv[i], "angular") == 0 )
            {
                m_shortestPathType = ShortestPathType::ANGULAR;
            }
            else if ( std::strcmp(argv[i], "metric") == 0 )
            {
                m_shortestPathType = ShortestPathType::METRIC;
            }
            else if ( std::strcmp(argv[i], "visual") == 0 )
            {
                m_shortestPathType = ShortestPathType::VISUAL;
            }
            else
            {
                throw CommandLineException(std::string("Invalid Shortest Path type: ") + argv[i]);
            }
        }
    }

    if (origin.empty())
    {
        throw CommandLineException("Origin point (-spo) must be provided");
    }
    else
    {
        m_origin = EntityParsing::parsePoint(origin);
    }
    if (destination.empty())
    {
        throw CommandLineException("Destination point (-spd) must be provided");
    }
    else
    {
        m_destination = EntityParsing::parsePoint(destination);
    }
    if (m_shortestPathType == ShortestPathType::NONE)
    {
        throw CommandLineException("Shortest path type (-spt) must be provided");
    }
}

void ShortestPathParser::run(const CommandLineParser &clp, IPerformanceSink &perfWriter) const
{
    dm_runmethods::runShortestPath(clp, m_shortestPathType, m_origin, m_destination, perfWriter);
}
