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

#include "../depthmapXcli/maxdepthparser.h"
#include "argumentholder.h"
#include "catch.hpp"

TEST_CASE("Max Depth args invalid", "") {
    {
        ArgumentHolder ah{"prog", "-f", "infile", "-o", "outfile", "-m", "MAXDEPTH", "-mdt"};
        MaxDepthParser p;
        REQUIRE_THROWS_WITH(p.parse(ah.argc(), ah.argv()), Catch::Contains("-mdt requires an argument"));
    }

    {
        ArgumentHolder ah{"prog", "-f", "infile", "-o", "outfile", "-m", "MAXDEPTH", "-mdt", "foo"};
        MaxDepthParser p;
        REQUIRE_THROWS_WITH(p.parse(ah.argc(), ah.argv()), Catch::Contains("Invalid step type: foo"));
    }

    {
        ArgumentHolder ah{"prog",     "-f",   "infile", "-o",   "outfile", "-m",
                          "MAXDEPTH", "-mdt", "visual", "-mdt", "metric"};
        MaxDepthParser p;
        REQUIRE_THROWS_WITH(p.parse(ah.argc(), ah.argv()), Catch::Contains("-mdt can only be used once"));
    }

    {
        ArgumentHolder ah{"prog", "-f", "infile", "-o", "outfile", "-m", "MAXDEPTH", "-mdt", "visual", "-mdr"};
        MaxDepthParser p;
        REQUIRE_THROWS_WITH(p.parse(ah.argc(), ah.argv()), Catch::Contains("-mdr requires an argument"));
    }

    {
        ArgumentHolder ah{"prog", "-f", "infile", "-o", "outfile", "-m", "MAXDEPTH", "-mdt", "visual", "-mdr", "foo"};
        MaxDepthParser p;
        REQUIRE_THROWS_WITH(p.parse(ah.argc(), ah.argv()),
                            Catch::Contains("Radius must be a positive integer number or n, got foo"));
    }
}

TEST_CASE("Max Depth args valid", "valid") {

    {
        ArgumentHolder ah{"prog", "-f", "infile", "-o", "outfile", "-m", "MAXDEPTH", "-mdt", "metric"};
        MaxDepthParser cmdP;
        cmdP.parse(ah.argc(), ah.argv());
        REQUIRE(cmdP.getStepType() == MaxDepthParser::StepType::METRIC);
    }

    {
        ArgumentHolder ah{"prog", "-f", "infile", "-o", "outfile", "-m", "MAXDEPTH", "-mdt", "angular"};
        MaxDepthParser cmdP;
        cmdP.parse(ah.argc(), ah.argv());
        REQUIRE(cmdP.getStepType() == MaxDepthParser::StepType::ANGULAR);
    }

    {
        ArgumentHolder ah{"prog", "-f", "infile", "-o", "outfile", "-m", "MAXDEPTH", "-mdt", "visual"};
        MaxDepthParser cmdP;
        cmdP.parse(ah.argc(), ah.argv());
        REQUIRE(cmdP.getStepType() == MaxDepthParser::StepType::VISUAL);
    }

    {
        ArgumentHolder ah{"prog", "-f", "infile", "-o", "outfile", "-m", "MAXDEPTH", "-mdt", "visual", "-mdr", "4"};
        MaxDepthParser cmdP;
        cmdP.parse(ah.argc(), ah.argv());
        REQUIRE(cmdP.getStepType() == MaxDepthParser::StepType::VISUAL);
        REQUIRE(cmdP.getRadius() == "4");
    }
}
