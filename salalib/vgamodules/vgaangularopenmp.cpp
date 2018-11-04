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

#include "salalib/vgamodules/vgaangularopenmp.h"

#include "genlib/stringutils.h"

bool VGAAngularOpenMP::run(Communicator *comm, const Options &options, PointMap &map, bool simple_version) {
    time_t atime = 0;

    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, map.getFilledPointCount());
    }

    AttributeTable &attributes = map.getAttributeTable();

    std::vector<PixelRef> filled;
    std::vector<int> rows;

    for (short i = 0; i < map.getCols(); i++) {
        for (short j = 0; j < map.getRows(); j++) {
            PixelRef curs = PixelRef(i, j);
            if (map.getPoint(curs).filled()) {
                filled.push_back(curs);
                rows.push_back(attributes.getRowid(curs));
            }
        }
    }

    int count = 0;

    std::vector<float> total_depth_col_data(filled.size());
    std::vector<float> mean_depth_col_data(filled.size());
    std::vector<float> count_col_data(filled.size());

    int i, N = int(filled.size());
#pragma omp parallel for default(shared) private(i) schedule(dynamic)
    for (i = 0; i < N; i++) {
        if (options.gates_only) {
            count++;
            continue;
        }

        depthmapX::RowMatrix<int> miscs(map.getRows(), map.getCols());
        depthmapX::RowMatrix<float> cumangles(map.getRows(), map.getCols());

        miscs.initialiseValues(0);
        cumangles.initialiseValues(-1.0f);

        float total_angle = 0.0f;
        int total_nodes = 0;

        // note that m_misc is used in a different manner to analyseGraph / PointDepth
        // here it marks the node as used in calculation only

        std::set<AngularTriple> search_list;
        search_list.insert(AngularTriple(0.0f, filled[size_t(i)], NoPixel));
        cumangles(filled[size_t(i)].y, filled[size_t(i)].x) = 0.0f;

        while (search_list.size()) {
            std::set<AngularTriple>::iterator it = search_list.begin();
            AngularTriple here = *it;
            search_list.erase(it);
            if (int(options.radius) != -1 && double(here.angle) > options.radius) {
                break;
            }
            Point &p = map.getPoint(here.pixel);
            int& p1misc = miscs(here.pixel.y, here.pixel.x);
            float& p1cumangle = cumangles(here.pixel.y, here.pixel.x);
            // nb, the filled check is necessary as diagonals seem to be stored with 'gaps' left in
            if (p.filled() && p1misc != ~0) {
                extractAngular(p.getNode(), search_list, &map, here, miscs, cumangles);
                p1misc = ~0;
                if (!p.getMergePixel().empty()) {
                    Point &p2 = map.getPoint(p.getMergePixel());
                    int& p2misc = miscs(p.getMergePixel().y, p.getMergePixel().x);
                    float& p2cumangle = cumangles(p.getMergePixel().y, p.getMergePixel().x);
                    if (p2misc != ~0) {
                        p2cumangle = p1cumangle;
                        extractAngular(p2.getNode(), search_list, &map, AngularTriple(here.angle, p.getMergePixel(), NoPixel),
                                                  miscs, cumangles);
                        p2misc = ~0;
                    }
                }
                total_angle += p1cumangle;
                total_nodes += 1;
            }
        }

        if (total_nodes > 0) {
            mean_depth_col_data[size_t(i)] = float(double(total_angle) / double(total_nodes));
        }
        total_depth_col_data[size_t(i)] = total_angle;
        count_col_data[size_t(i)] = float(total_nodes);

        count++; // <- increment count

        if (comm) {
            if (qtimer(atime, 500)) {
                if (comm->IsCancelled()) {
                    throw Communicator::CancelledException();
                }
                comm->CommPostMessage(Communicator::CURRENT_RECORD, count);
            }
        }

        // kept to achieve parity in binary comparison with old versions
        // TODO: Remove at next version of .graph file
        map.getPoint(filled[size_t(i)]).m_misc = miscs(filled[size_t(i)].y, filled[size_t(i)].x);
        map.getPoint(filled[size_t(i)]).m_cumangle = cumangles(filled[size_t(i)].y, filled[size_t(i)].x);
    }

    std::string radius_text;
    if (int(options.radius) != -1) {
        if (map.getRegion().width() > 100.0) {
            radius_text = std::string(" R") + dXstring::formatString(options.radius, "%.f");
        } else if (map.getRegion().width() < 1.0) {
            radius_text = std::string(" R") + dXstring::formatString(options.radius, "%.4f");
        } else {
            radius_text = std::string(" R") + dXstring::formatString(options.radius, "%.2f");
        }
    }
    // n.b. these must be entered in alphabetical order to preserve col indexing:
    std::string mean_depth_col_text = std::string("Angular Mean Depth") + radius_text;
    int mean_depth_col = attributes.insertColumn(mean_depth_col_text.c_str());
    std::string total_detph_col_text = std::string("Angular Total Depth") + radius_text;
    int total_depth_col = attributes.insertColumn(total_detph_col_text.c_str());
    std::string count_col_text = std::string("Angular Node Count") + radius_text;
    int count_col = attributes.insertColumn(count_col_text.c_str());

    for (size_t i = 0; i < rows.size(); i++) {
        attributes.setValue(rows[i], mean_depth_col, mean_depth_col_data[i]);
        attributes.setValue(rows[i], total_depth_col, total_depth_col_data[i]);
        attributes.setValue(rows[i], count_col, count_col_data[i]);
    }

    map.overrideDisplayedAttribute(-2);
    map.setDisplayedAttribute(mean_depth_col);

    return true;
}
