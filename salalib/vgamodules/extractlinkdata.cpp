// sala - a component of the depthmapX - spatial network analysis platform
// Copyright (C) 2000-2010, University College London, Alasdair Turner
// Copyright (C) 2011-2012, Tasos Varoudis
// Copyright (C) 2017-2018, Petros Koutsolampros

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

#include "salalib/vgamodules/extractlinkdata.h"

#include "genlib/stringutils.h"

bool ExtractLinkData::run(Communicator *, const Options &, PointMap &map, bool) {

    auto &attributes = map.getAttributeTable();

    int angular_cost_col = attributes.insertColumn("Link Angular Cost");
    int metric_cost_col = attributes.insertColumn("Link Metric Cost");
    int link_to_col = attributes.insertColumn("Link To");
    int visual_cost_col = attributes.insertColumn("Link Visual Cost");

    for (int i = 0; i < attributes.getRowCount(); i++) {
        PixelRef pix = attributes.getRowKey(i);
        Point &p = map.getPoint(pix);
        PixelRef mergePixel = p.getMergePixel();
        if (!mergePixel.empty()) {
            attributes.setValue(i, link_to_col, static_cast<int>(mergePixel));
            attributes.setValue(i, visual_cost_col, 1);
            attributes.setValue(i, metric_cost_col, dist(pix, mergePixel) * map.getSpacing());
            attributes.setValue(i, angular_cost_col, 1);
        }
    }

    return true;
}
