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

#include "gridproperties.h"
#include <math.h>

GridProperties::GridProperties(double maxDimension) {
    int maxexponent = (int)floor(log10(maxDimension)) - 1;
    int minexponent = maxexponent - 2;
    int mantissa = (int)floor(maxDimension / pow(10.0, double(maxexponent + 1)));

    m_default = (double)mantissa * pow(10.0, double(maxexponent - 1));
    m_max = (double)2 * mantissa * pow(10.0, double(maxexponent));
    m_min = (double)mantissa * pow(10.0, double(minexponent));
}
