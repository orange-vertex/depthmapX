// Copyright (C) 2017 Christian Sailer

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

#include "maxdepthparser.h"
#include "exceptions.h"
#include "parsingutils.h"
#include "radiusconverter.h"
#include "runmethods.h"
#include <cstring>

using namespace depthmapX;

MaxDepthParser::MaxDepthParser() : m_stepType(StepType::NONE) {}

void MaxDepthParser::parse(int argc, char *argv[]) {
    for (int i = 1; i < argc;) {

        if (std::strcmp("-mdt", argv[i]) == 0) {
            if (m_stepType != StepType::NONE) {
                throw CommandLineException("-mdt can only be used once, modes are mutually exclusive");
            }
            ENFORCE_ARGUMENT("-mdt", i)
            else if (std::strcmp(argv[i], "visual") == 0) {
                m_stepType = StepType::VISUAL;
            }
            else if (std::strcmp(argv[i], "metric") == 0) {
                m_stepType = StepType::METRIC;
            }
            else if (std::strcmp(argv[i], "angular") == 0) {
                m_stepType = StepType::ANGULAR;
            }
            else {
                throw CommandLineException(std::string("Invalid step type: ") + argv[i]);
            }
        } else if (std::strcmp(argv[i], "-mdr") == 0) {
            ENFORCE_ARGUMENT("-mdr", i)
            m_radius = argv[i];
        }
        ++i;
    }

    if (m_stepType == StepType::NONE) {
        throw CommandLineException("Step depth type (-mdt) must be provided");
    }

    if (m_radius.empty()) {
        m_radius = "n";
    } else if (m_radius != "n" & !has_only_digits(m_radius)) {
        throw CommandLineException(std::string("Radius must be a positive integer number or n, got ") + m_radius);
    }
}

void MaxDepthParser::run(const CommandLineParser &clp, IPerformanceSink &perfWriter) const {
    RadiusConverter radiusConverter;
    dm_runmethods::maxDepth(clp, *this, radiusConverter, perfWriter);
}
