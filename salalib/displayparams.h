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

struct DisplayParams {
    enum {
        AXMANESQUE = 0,
        GREYSCALE = 1,
        MONOCHROME = 2,
        DEPTHMAPCLASSIC = 3,
        PURPLEORANGE = 4,
        BLUERED = 5,
        HUEONLYAXMANESQUE = 6
    };
    float blue;
    float red;
    int colorscale;
    DisplayParams() {
        blue = 0.0f;
        red = 1.0f;
        colorscale = 0;
    }
};
