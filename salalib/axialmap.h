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


#ifndef __SHAPEGRAPH_H__
#define __SHAPEGRAPH_H__

#include "salalib/spacepixfile.h"
#include "spacepix.h"
#include "connector.h"

struct AxialVertex;
struct AxialVertexKey;
struct RadialLine;
struct PolyConnector;

struct ValuePair;

// used during angular analysis
struct AnalysisInfo
{
   // lists used for multiple radius analysis
   bool leaf;
   bool choicecovered;
   SegmentRef previous;
   int depth;
   double choice;            
   double weighted_choice;
   double weighted_choice2; //EFEF
   AnalysisInfo() {
      choicecovered = false; leaf = true; previous = SegmentRef(); depth = 0; choice = 0.0; weighted_choice = 0.0; weighted_choice2 = 0.0; 
   }
   void clearLine() {
      choicecovered = false; leaf = true; previous = SegmentRef(); depth = 0; // choice values are cummulative and not cleared
   }
};

class MapInfoData;


class ShapeGraph : public ShapeMap
{
   friend class AxialMinimiser;
   friend class MapInfoData;
protected:
protected:
public:
//   //void initAttributes();
//   // extra parameters for selection_only and interactive are for parallel process extensions
   //
   //
};

class ShapeGraphUndirected : public ShapeGraph
{
protected:
    // for graph functionality
    // Note: this list is stored PACKED for optimal performance on graph analysis
    // ALWAYS check it is in the same order as the shape list and attribute table
    std::vector<Connector> m_connectors;
    prefvec<pvecint> m_keyvertices;       // but still need to return keyvertices here
    int m_keyvertexcount;
public:
    ShapeGraphUndirected(const std::string& name = "<axial map>", int type = ShapeMap::AXIALMAP) {
        m_keyvertexcount = 0;
    }
    const std::vector<Connector>& getConnections() const
    { return m_connectors; }
    std::vector<Connector>& getConnections()
    { return m_connectors; }
    void makeConnections(const prefvec<pvecint>& keyvertices = prefvec<pvecint>());
    void unlinkFromShapeMap(const ShapeMap& shapemap);

    void makeSegmentMap(std::vector<Line> &lineset, prefvec<Connector>& connectionset, double stubremoval);

    void initialiseAttributesAxial();
    bool integrate(Communicator *comm = NULL, const pvecint& radius = pvecint(),
                   bool choice = false, bool local = false, bool fulloutput = false,
                   int weighting_col = -1, bool simple_version = true);
    bool angularstepdepth(Communicator *comm);

    void writeAxialConnectionsAsDotGraph(std::ostream &stream);
    void writeAxialConnectionsAsPairsCSV(std::ostream &stream);
    void clearAll() { ShapeMap::clearAll(); m_connectors.clear(); }
    void outputNet(std::ostream& netfile) const;
    bool outputMifPolygons(std::ostream& miffile, std::ostream& midfile) const;

    bool moveShape(int shaperef, const Line& line, bool undoing = false);

    virtual bool read( std::istream& stream, int version );
    virtual bool write( std::ofstream& stream, int version );
};

class ShapeGraphDirected : public ShapeGraph
{
protected:
    // for graph functionality
    // Note: this list is stored PACKED for optimal performance on graph analysis
    // ALWAYS check it is in the same order as the shape list and attribute table
    std::vector<Connector> m_connectors;
    prefvec<pvecint> m_keyvertices;       // but still need to return keyvertices here
    int m_keyvertexcount;
public:
    ShapeGraphDirected(const std::string& name = "<segment map>", int type = ShapeMap::SEGMENTMAP);
    const std::vector<Connector>& getConnections() const
    { return m_connectors; }
    std::vector<Connector>& getConnections()
    { return m_connectors; }
    void makeConnections(const prefvec<pvecint>& keyvertices = prefvec<pvecint>());
    // lineset and connectionset are filled in by segment map
    void makeNewSegMap();
    void makeSegmentConnections(prefvec<Connector>& connectionset);

    void initialiseAttributesSegment();

    bool stepdepth(Communicator *comm = NULL);
    int analyseTulip(Communicator *comm, int tulip_bins, bool choice, int radius_type,
                     const pvecdouble& radius, int weighting_col, int weighting_col2 = -1,
                     int routeweight_col = -1, bool selection_only = false, bool interactive = true);
    // the two topomet analyses can be found in topomet.cpp:
    bool analyseTopoMet(Communicator *comm, int analysis_type, double radius, bool sel_only);
    bool analyseTopoMetPD(Communicator *comm, int analysis_type);
    bool analyseAngular(Communicator *comm, const pvecdouble& radius);
    void pushAxialValues(ShapeGraph& axialmap);

    void writeSegmentConnectionsAsPairsCSV(std::ostream &stream);
    void clearAll() { ShapeMap::clearAll(); m_connectors.clear(); }
    void outputNet(std::ostream& netfile) const;

    virtual bool read( std::istream& stream, int version );
    virtual bool write( std::ofstream& stream, int version );
};


#endif
