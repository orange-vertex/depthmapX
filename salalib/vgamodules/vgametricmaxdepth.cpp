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

#include "salalib/vgamodules/vgametricmaxdepth.h"

#include "genlib/stringutils.h"

// This is a slow algorithm, but should give the correct answer
// for demonstrative purposes

bool VGAMetricMaxDepth::run(Communicator *comm, PointMap &map, bool) {
    time_t atime = 0;
    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, map.getFilledPointCount());
    }

    std::string radius_text;
    if (m_radius != -1.0) {
        if (m_radius > 100.0) {
            radius_text = std::string(" R") + dXstring::formatString(m_radius, "%.f");
        } else if (map.getRegion().width() < 1.0) {
            radius_text = std::string(" R") + dXstring::formatString(m_radius, "%.4f");
        } else {
            radius_text = std::string(" R") + dXstring::formatString(m_radius, "%.2f");
        }
    }
    AttributeTable &attributes = map.getAttributeTable();

    std::string maxdepth_col_text = std::string("Metric Max Depth") + radius_text;
    int maxdepth_col = attributes.insertOrResetColumn(maxdepth_col_text.c_str());

    int count = 0;

    for (size_t i = 0; i < map.getCols(); i++) {
        for (size_t j = 0; j < map.getRows(); j++) {
            PixelRef curs = PixelRef(static_cast<short>(i), static_cast<short>(j));

            if (map.getPoint(curs).filled()) {

                // TODO: Break out miscs/dist/cumangle into local variables and remove from Point class
                for (auto &point : map.getPoints()) {
                    point.m_misc = 0;
                    point.m_dist = -1.0f;
                    point.m_cumangle = 0.0f;
                }

                // note that m_misc is used in a different manner to analyseGraph / PointDepth
                // here it marks the node as used in calculation only

                std::set<MetricTriple> search_list;
                search_list.insert(MetricTriple(0.0f, curs, NoPixel));
                float maxdepth = 0;
                while (search_list.size()) {
                    std::set<MetricTriple>::iterator it = search_list.begin();
                    MetricTriple here = *it;
                    search_list.erase(it);
                    if (m_radius != -1.0 && (here.dist * map.getSpacing()) > m_radius) {
                        break;
                    }
                    Point &p = map.getPoint(here.pixel);
                    // nb, the filled check is necessary as diagonals seem to be stored with 'gaps' left in
                    if (p.filled() && p.m_misc != ~0) {
                        p.getNode().extractMetric(search_list, &map, here);
                        p.m_misc = ~0;
                        if (!p.getMergePixel().empty()) {
                            Point &p2 = map.getPoint(p.getMergePixel());
                            if (p2.m_misc != ~0) {
                                p2.getNode().extractMetric(search_list, &map,
                                                           MetricTriple(here.dist, p.getMergePixel(), NoPixel));
                                p2.m_misc = ~0;
                            }
                        }
                        float depth = float(here.dist * map.getSpacing());
                        if(depth > maxdepth) {
                            maxdepth = depth;
                        }
                    }
                }

                AttributeRow &row = attributes.getRow(AttributeKey(curs));
                row.setValue(maxdepth_col, maxdepth);

                count++; // <- increment count
            }
            if (comm) {
                if (qtimer(atime, 500)) {
                    if (comm->IsCancelled()) {
                        throw Communicator::CancelledException();
                    }
                    comm->CommPostMessage(Communicator::CURRENT_RECORD, count);
                }
            }
        }
    }

    map.overrideDisplayedAttribute(-2);
    map.setDisplayedAttribute(maxdepth_col);

    return true;
}
