// Copyright (C) 2019 Petros Koutsolampros

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

#include "isovistzoneparser.h"
#include "exceptions.h"
#include "parsingutils.h"
#include "runmethods.h"
#include "salalib/entityparsing.h"
#include <cstring>
#include <sstream>

using namespace depthmapX;

void IsovistZoneParser::parse(int argc, char **argv) {

    std::vector<std::string> origins;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp("-izo", argv[i]) == 0) {
            ENFORCE_ARGUMENT("-izo", i)
            if (!has_only_digits_dots_commas(argv[i])) {
                std::stringstream message;
                message << "Invalid origin point provided (" << argv[i]
                        << "). Should only contain digits dots and commas" << std::flush;
                throw CommandLineException(message.str().c_str());
            }
            origins.push_back(argv[i]);
        } else if (std::strcmp("-izr", argv[i]) == 0) {
            ENFORCE_ARGUMENT("-izr", i)
            if (!has_only_digits_dots_commas(argv[i])) {
                std::stringstream message;
                message << "Invalid restriction distance provided (" << argv[i]
                        << "). Should only contain digits dots and commas" << std::flush;
                throw CommandLineException(message.str().c_str());
            }
            m_restrictDistance = std::stof(argv[i]);
        }
    }

    if (origins.empty()) {
        throw CommandLineException("At least one origin point (-izo) must be provided");
    } else {
        for (const auto &origin : origins) {
            m_origins.push_back(EntityParsing::parsePoint(origin));
        }
    }
}

void IsovistZoneParser::run(const CommandLineParser &clp, IPerformanceSink &perfWriter) const {
    dm_runmethods::runIsovistZone(clp, m_origins, m_restrictDistance, perfWriter);
}
