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

#include "salalib/vgamodules/vgavisuallocalopenmp.h"

#include "genlib/stringutils.h"

bool VGAVisualLocalOpenMP::run(Communicator *comm, const Options &options, PointMap &map, bool simple_version) {
    time_t atime = 0;

    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, map.getFilledPointCount());
    }

    AttributeTable &attributes = map.getAttributeTable();

    std::vector<PixelRef> filled;
    std::vector<int> rows;

    for (int i = 0; i < map.getCols(); i++) {
        for (int j = 0; j < map.getRows(); j++) {
            PixelRef curs = PixelRef(static_cast<short>(i), static_cast<short>(j));
            if (map.getPoint(curs).filled()) {
                filled.push_back(curs);
                rows.push_back(attributes.getRowid(curs));
            }
        }
    }

    int count = 0;

    count = 0;

    std::vector<float> cluster_col_data(filled.size());
    std::vector<float> control_col_data(filled.size());
    std::vector<float> controllability_col_data(filled.size());

    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_STEPS, 1);
        comm->CommPostMessage(Communicator::CURRENT_STEP, 1);
        comm->CommPostMessage(Communicator::NUM_RECORDS, filled.size());
    }
    std::vector<std::set<int>> hoods(filled.size());

    int i, N = int(filled.size());
    std::map<PixelRef, int> refToFilled;
    for (i = 0; i < N; ++i) {
        refToFilled.insert(std::make_pair(filled[size_t(i)], i));
    }
#pragma omp parallel for default(shared) private(i) schedule(dynamic)
    for (i = 0; i < N; ++i) {
        Point &p = map.getPoint(filled[size_t(i)]);
        std::set<PixelRef> neighbourhood;
#pragma omp critical(dumpNeighbourhood)
        { dumpNeighbourhood(p.getNode(), neighbourhood); }
        for (auto &neighbour : neighbourhood) {
            if (map.getPoint(neighbour).hasNode()) {
                hoods[size_t(i)].insert(refToFilled[neighbour]);
            }
        }
    }

#pragma omp parallel for default(shared) private(i) schedule(dynamic)
    for (i = 0; i < N; ++i) {

        Point &p = map.getPoint(filled[size_t(i)]);
        if ((p.contextfilled() && !filled[size_t(i)].iseven()) || (options.gates_only)) {
            count++;
            continue;
        }

        // This is much easier to do with a straight forward list:
        std::set<int> &neighbourhood = hoods[size_t(i)];
        std::set<int> totalneighbourhood;
        int cluster = 0;
        float control = 0.0f;

        for (auto &neighbour : neighbourhood) {
            std::set<int> &retneighbourhood = hoods[size_t(neighbour)];
            std::set<int> intersect;
            std::set_intersection(neighbourhood.begin(), neighbourhood.end(), retneighbourhood.begin(),
                                  retneighbourhood.end(), std::inserter(intersect, intersect.begin()));
            totalneighbourhood.insert(retneighbourhood.begin(), retneighbourhood.end());
            control += 1.0f / float(retneighbourhood.size());
            cluster += intersect.size();
        }
#pragma omp critical(add_to_col)
        {
            if (neighbourhood.size() > 1) {
                cluster_col_data[size_t(i)] =
                    float(cluster / double(neighbourhood.size() * (neighbourhood.size() - 1.0)));
                control_col_data[size_t(i)] = float(control);
                controllability_col_data[size_t(i)] =
                    float(double(neighbourhood.size()) / double(totalneighbourhood.size()));
            } else {
                cluster_col_data[size_t(i)] = -1.0f;
                control_col_data[size_t(i)] = -1.0f;
                controllability_col_data[size_t(i)] = neighbourhood.size();
            }
        }

#pragma omp critical(count)
        {
            count++; // <- increment count
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

    int cluster_col = attributes.insertColumn("Visual Clustering Coefficient");
    int control_col = attributes.insertColumn("Visual Control");
    int controllability_col = attributes.insertColumn("Visual Controllability");

    for (size_t i = 0; i < rows.size(); i++) {
        attributes.setValue(rows[i], cluster_col, cluster_col_data[i]);
        attributes.setValue(rows[i], control_col, control_col_data[i]);
        attributes.setValue(rows[i], controllability_col, controllability_col_data[i]);
    }
    map.setDisplayedAttribute(cluster_col);

    return true;
}