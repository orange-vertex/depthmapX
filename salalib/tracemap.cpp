// sala - a component of the depthmapX - spatial network analysis platform
// Copyright (C) 2011-2012, Petros Koutsolampros

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

#include "tracemap.h"

int TraceMap::makeTraceWithRef(const std::vector<Event2f> &trace, int trace_ref,
                               const std::map<int, float> &extraAttributes) {
    std::vector<Point2f> traceGeometry(trace.size());
    std::vector<double> traceTimes(trace.size());
    int counter = 0;
    for (const Event2f &event : trace) {
        traceGeometry[counter] = event;
        traceTimes[counter] = event.t;
        counter++;
    }
    makePolyShapeWithRef(traceGeometry, true, trace_ref, false, extraAttributes);
    m_traceTimes.insert(std::make_pair(trace_ref, traceTimes));
    return trace_ref;
}

int TraceMap::makeTrace(const std::vector<Event2f> &trace, const std::map<int, float> &extraAttributes) {
    return makeTraceWithRef(trace, getNextShapeKey(), extraAttributes);
}

bool TraceMap::read( std::istream& stream, int version )
{
    ShapeMap::read(stream, version, false);

    int count = 0;
    stream.read((char *) &count, sizeof(count));
    for (int j = 0; j < count; j++) {
       int key;
       stream.read((char *) &key, sizeof(key));
       m_traceTimes.insert(std::make_pair(key, dXreadwrite::readVector<double>(stream)));
    }
   return true;
}

bool TraceMap::write(std::ofstream &stream, int version) {
    ShapeMap::write(stream, version);

    // write trace times
    int count = m_traceTimes.size();
    stream.write((char *)&count, sizeof(count));
    for (auto &traceTimes : m_traceTimes) {
        int key = traceTimes.first;
        stream.write((char *)&key, sizeof(key));
        dXreadwrite::writeVector<double>(stream, traceTimes.second);
    }

    return true;
}
