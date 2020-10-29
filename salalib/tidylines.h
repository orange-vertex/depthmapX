// sala - a component of the depthmapX - spatial network analysis platform
// Copyright (C) 2011-2012, Tasos Varoudis

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

#include "salalib/spacepix.h"

#include "genlib/p2dpoly.h"

// helpers... a class to tidy up ugly maps people may give me...

class TidyLines : public SpacePixel {
  public:
    void tidy(std::vector<Line> &lines, const QtRegion &region);
    void quicktidy(std::map<int, std::pair<Line, int>> &lines, const QtRegion &region);
};
