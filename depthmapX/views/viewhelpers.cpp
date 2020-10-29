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

#include "viewhelpers.h"
#include <time.h>

namespace ViewHelpers {
    Point2f calculateCenter(const QPoint &point, const QPoint &oldCentre, double factor) {
        int diffX = oldCentre.x() - point.x();
        int diffY = oldCentre.y() - point.y();
        return Point2f(point.x() + double(diffX) * factor, point.y() + double(diffY) * factor);
    }

    std::string getCurrentDate() {
        time_t now = time(NULL);
        char timeString[11];
        strftime(timeString, 11, "%Y/%m/%d", localtime(&now));
        return timeString;
    }

} // namespace ViewHelpers
