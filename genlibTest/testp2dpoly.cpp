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

#include "catch.hpp"
#include "genlib/comm.h"
#include "genlib/p2dpoly.h"

//TEST_CASE("intersect_region")
//{

//}
//TEST_CASE("overlap_x")
//{

//}
//TEST_CASE("overlap_y")
//{

//}

TEST_CASE("intersect_line intersect & touch")
{
    Line l1;
    Line l2;
    SECTION ("Crossing") {
        l1 = Line(Point2f(0,0), Point2f(1,1));
        l2 = Line(Point2f(0,1), Point2f(1,0));
    }
    SECTION ("Touching mid-start") {
        l1 = Line(Point2f(1,0), Point2f(1,1));
        l2 = Line(Point2f(0,0), Point2f(2,0));
    }
    SECTION ("Touching mid-end") {
        l1 = Line(Point2f(1,1), Point2f(1,0));
        l2 = Line(Point2f(0,0), Point2f(2,0));
    }
    SECTION ("Touching start-mid") {
        l1 = Line(Point2f(0,0), Point2f(2,0));
        l2 = Line(Point2f(1,0), Point2f(1,1));
    }
    SECTION ("Touching end-mid") {
        l1 = Line(Point2f(0,0), Point2f(2,0));
        l2 = Line(Point2f(1,1), Point2f(1,0));
    }
    SECTION ("Test") {
        l1 = Line(Point2f(-1800, -42300), Point2f(-1350, -42300));
        l2 = Line(Point2f(-1556.773594361551, -42299.999999999964), Point2f(-1556.773594361551, -42253.999999999964));
    }
    SECTION ("Test1") {
        l1 = Line(Point2f(-1556.773594361551, -42299.999999999964), Point2f(-1556.773594361551, -42253.999999999964));
        l2 = Line(Point2f(-1800, -42300), Point2f(-1350, -42300));
    }
    SECTION ("Test2") {
        l1 = Line(Point2f(-1350, -42300), Point2f(-1800, -42300));
        l2 = Line(Point2f(-1556.773594361551, -42253.999999999964), Point2f(-1556.773594361551, -42299.999999999964));
    }
    SECTION ("Test3") {
        l1 = Line(Point2f(-1556.773594361551, -42253.999999999964), Point2f(-1556.773594361551, -42299.999999999964));
        l2 = Line(Point2f(-1350, -42300), Point2f(-1800, -42300));
    }

    double spacing = 450;
    REQUIRE(intersect_line(l1, l2, spacing * 1e-10));
    REQUIRE(intersect_region(l1, l2, spacing * 1e-10));

}

//intersect_line_no_touch
//intersect_line_distinguish
//intersect_line_b
