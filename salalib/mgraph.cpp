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



// The meta graph 

#include <math.h>
#include <time.h>
#include <genlib/paftl.h>
#include <genlib/pafmath.h>
#include <genlib/p2dpoly.h>
#include <genlib/dxfp.h>
#include <genlib/comm.h>

#include "isovist.h"
#include "ntfp.h"
#include "tigerp.h"
#include <salalib/mgraph.h>

// shouldn't really include this -- required for node in PushValuesToLayer
#include <salalib/ngraph.h>
#include <salalib/importutils.h>

#include "mgraph440/mgraph.h"
#include <sstream>

// Quick mod - TV
#pragma warning (disable: 4800)

///////////////////////////////////////////////////////////////////////////////////

MetaGraph::MetaGraph()
{ 
   m_state = 0; 
   m_view_class = VIEWNONE;
   m_file_version = -1; // <- if unsaved, file version is -1

   // whether or not showing text / grid saved with file:
   m_showtext = false;
   m_showgrid = false;

   // bsp tree for making isovists:
   m_bsp_tree = false;
   m_bsp_root = NULL;
}

MetaGraph::~MetaGraph()
{
   if (m_bsp_root) {
      delete m_bsp_root;
      m_bsp_root = NULL;
   }
   m_bsp_tree = false;
}

QtRegion MetaGraph::getBoundingBox() const
{
   QtRegion bounds = SuperSpacePixel::getRegion();
   if (bounds.atZero() && ((getState() & MetaGraph::SHAPEGRAPHS) == MetaGraph::SHAPEGRAPHS)) {
      bounds = m_shape_graphs.getBoundingBox();
   }
   if (bounds.atZero() && ((getState() & MetaGraph::DATAMAPS) == MetaGraph::DATAMAPS)) {
      bounds = getDisplayedDataMap().getRegion();
   }
   return bounds;
}

bool MetaGraph::setViewClass(int command)
{
   if (command < 0x10) {
      throw ("Use with a show command, not a view class type");
   }
   if ((command & (SHOWHIDEVGA | SHOWVGATOP)) && (~m_state & POINTMAPS)) 
      return false; 
   if ((command & (SHOWHIDEAXIAL | SHOWAXIALTOP)) && (~m_state & SHAPEGRAPHS)) 
      return false; 
   if ((command & (SHOWHIDESHAPE | SHOWSHAPETOP)) && (~m_state & DATAMAPS)) 
      return false; 
   switch (command) {
   case SHOWHIDEVGA:
      if (m_view_class & (VIEWVGA | VIEWBACKVGA)) {
         m_view_class &= ~(VIEWVGA | VIEWBACKVGA);
         if (m_view_class & VIEWBACKAXIAL) {
            m_view_class ^= (VIEWAXIAL | VIEWBACKAXIAL);
         }
         else if (m_view_class & VIEWBACKDATA) {
            m_view_class ^= (VIEWDATA | VIEWBACKDATA);
         }
      }
      else if (m_view_class & (VIEWAXIAL | VIEWDATA)) {
         m_view_class &= ~(VIEWBACKAXIAL | VIEWBACKDATA);
         m_view_class |= VIEWBACKVGA;
      }
      else {
         m_view_class |= VIEWVGA;
      }
      break;
   case SHOWHIDEAXIAL:
      if (m_view_class & (VIEWAXIAL | VIEWBACKAXIAL)) {
         m_view_class &= ~(VIEWAXIAL | VIEWBACKAXIAL);
         if (m_view_class & VIEWBACKVGA) {
            m_view_class ^= (VIEWVGA | VIEWBACKVGA);
         }
         else if (m_view_class & VIEWBACKDATA) {
            m_view_class ^= (VIEWDATA | VIEWBACKDATA);
         }
      }
      else if (m_view_class & (VIEWVGA | VIEWDATA)) {
         m_view_class &= ~(VIEWBACKVGA | VIEWBACKDATA);
         m_view_class |= VIEWBACKAXIAL;
      }
      else {
         m_view_class |= VIEWAXIAL;
      }
      break;
   case SHOWHIDESHAPE:
      if (m_view_class & (VIEWDATA | VIEWBACKDATA)) {
         m_view_class &= ~(VIEWDATA | VIEWBACKDATA);
         if (m_view_class & VIEWBACKVGA) {
            m_view_class ^= (VIEWVGA | VIEWBACKVGA);
         }
         else if (m_view_class & VIEWBACKAXIAL) {
            m_view_class ^= (VIEWAXIAL | VIEWBACKAXIAL);
         }
      }
      else if (m_view_class & (VIEWVGA | VIEWAXIAL)) {
         m_view_class &= ~(VIEWBACKVGA | VIEWBACKAXIAL);
         m_view_class |= VIEWBACKDATA;
      }
      else {
         m_view_class |= VIEWDATA;
      }
      break;
   case SHOWVGATOP:
      if (m_view_class & VIEWAXIAL) {
         m_view_class = VIEWBACKAXIAL | VIEWVGA;
      }
      else if (m_view_class & VIEWDATA) {
         m_view_class = VIEWBACKDATA | VIEWVGA;
      }
      else {
         m_view_class = VIEWVGA | (m_view_class & (VIEWBACKAXIAL | VIEWBACKDATA));
      }
      break;
   case SHOWAXIALTOP:
      if (m_view_class & VIEWVGA) {
         m_view_class = VIEWBACKVGA | VIEWAXIAL;
      }
      else if (m_view_class & VIEWDATA) {
         m_view_class = VIEWBACKDATA | VIEWAXIAL;
      }
      else {
         m_view_class = VIEWAXIAL | (m_view_class & (VIEWBACKVGA | VIEWBACKDATA));
      }
      break;
   case SHOWSHAPETOP:
      if (m_view_class & VIEWVGA) {
         m_view_class = VIEWBACKVGA | VIEWDATA;
      }
      else if (m_view_class & VIEWAXIAL) {
         m_view_class = VIEWBACKAXIAL | VIEWDATA;
      }
      else {
         m_view_class = VIEWDATA | (m_view_class & (VIEWBACKVGA | VIEWBACKAXIAL));
      }
      break;
   }
   return true;
}

double MetaGraph::getLocationValue(const Point2f& point)
{
   // this varies according to whether axial or vga information is displayed on top
   double val = -2;

   if (viewingProcessedPoints()) {
      val = getDisplayedPointMap().getLocationValue(point);
   }
   else if (viewingProcessedLines()) {
      val = m_shape_graphs.getDisplayedMap().getLocationValue(point);
   }
   else if (viewingProcessedShapes()) {
      val = getDisplayedDataMap().getLocationValue(point);
   }

   return val;
}

void MetaGraph::copyLineData(const SuperSpacePixel& meta)
{
   m_state &= ~LINEDATA;

   *(SuperSpacePixel *)this = meta;

   setSpacePixel( (SuperSpacePixel *) this );   // <- also helpfully gives PointMap the space pixel

   m_state |= LINEDATA;
}

void MetaGraph::copyPointMap(const PointMap& meta)
{
   m_state &= ~POINTMAPS;

   *(PointMap *)this = meta;

   m_state |= POINTMAPS;
}

bool MetaGraph::setGrid( double spacing, const Point2f& offset )
{
   m_state &= ~POINTMAPS;

   getDisplayedPointMap().setSpacePixel( (SuperSpacePixel *) this );
   getDisplayedPointMap().setGrid( spacing, offset );

   m_state |= POINTMAPS;

   // just reassert that we should be viewing this (since set grid is essentially a "new point map")
   setViewClass(SHOWVGATOP);

   return true;
}

// AV TV // semifilled
bool MetaGraph::makePoints( const Point2f& p, int fill_type , Communicator *communicator )
{
//   m_state &= ~POINTS;

   try {
      getDisplayedPointMap().makePoints( p, fill_type, communicator );
   }
   catch (Communicator::CancelledException) {

      // By this stage points almost certainly exist,
      // To avoid problems, just say points exist:
      m_state |= POINTMAPS;
      
      return false;
   }

//   m_state |= POINTS;

   return true;
}

bool MetaGraph::undoPoints()
{
   bool b_return = getDisplayedPointMap().undoPoints();
   return b_return;
}

bool MetaGraph::clearPoints()
{
   bool b_return = getDisplayedPointMap().clearPoints();
   return b_return;
}

bool MetaGraph::makeGraph( Communicator *communicator, int algorithm, double maxdist )
{
   // this is essentially a version tag, and remains for historical reasons:
   m_state |= ANGULARGRAPH;

   bool retvar = false;
   
   try {
      // algorithm is now used for boundary graph option (as a simple boolean)
      retvar = getDisplayedPointMap().sparkGraph2(communicator, (algorithm != 0), maxdist);
   } 
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   if (retvar) {
      setViewClass(SHOWVGATOP);
   }

   return retvar;
}

bool MetaGraph::analyseGraph( Communicator *communicator, Options options , bool simple_version )   // <- options copied to keep thread safe
{
   bool retvar = false;

   if (options.point_depth_selection) {
      if (m_view_class & VIEWVGA && !getDisplayedPointMap().isSelected()) {
         return false;
      }
      else if (m_view_class & VIEWAXIAL && !m_shape_graphs.getDisplayedMap().isSelected()) {
         return false;
      }
   }

   try {
      retvar = true;
      if (options.point_depth_selection == 1) {
         if (m_view_class & VIEWVGA) {
            getDisplayedPointMap().analyseVisualPointDepth( communicator );
         }
         else if (m_view_class & VIEWAXIAL) {
            if (!m_shape_graphs.getDisplayedMap().isSegmentMap()) {
               m_shape_graphs.getDisplayedMap().stepdepth( communicator );
            }
            else {
               m_shape_graphs.getDisplayedMap().angularstepdepth( communicator );
            }
         }
         // REPLACES:
         // Graph::calculate_point_depth_matrix( communicator );
      }
      else if (options.point_depth_selection == 2) {
         if (m_view_class & VIEWVGA) {
            getDisplayedPointMap().analyseMetricPointDepth( communicator );
         }
         else if (m_view_class & VIEWAXIAL && m_shape_graphs.getDisplayedMap().isSegmentMap()) {
            m_shape_graphs.getDisplayedMap().analyseTopoMetPD( communicator, 1 ); // 1 is metric step depth
         }
      }
      else if (options.point_depth_selection == 3) {
         getDisplayedPointMap().analyseAngularPointDepth( communicator );
      }
      else if (options.point_depth_selection == 4) {
         if (m_view_class & VIEWVGA) {
            getDisplayedPointMap().binDisplay( communicator );
         }
         else if (m_view_class & VIEWAXIAL && m_shape_graphs.getDisplayedMap().isSegmentMap()) {
            m_shape_graphs.getDisplayedMap().analyseTopoMetPD( communicator, 0 ); // 0 is topological step depth
         }
      }
      else if (options.output_type == Options::OUTPUT_ISOVIST) {
         getDisplayedPointMap().analyseIsovist( communicator, *this, simple_version );
      }
      else if (options.output_type == Options::OUTPUT_VISUAL) {
         getDisplayedPointMap().analyseVisual( communicator, options, simple_version );
         // REPLACES:
         // Graph::calculate_depth_matrix( communicator, options, output_graph );
      }
      else if (options.output_type == Options::OUTPUT_METRIC) {
         getDisplayedPointMap().analyseMetric( communicator, options );
      }
      else if (options.output_type == Options::OUTPUT_ANGULAR) {
         getDisplayedPointMap().analyseAngular( communicator, options );
      }
      else if (options.output_type == Options::OUTPUT_THRU_VISION) {
         retvar = analyseThruVision( communicator, options.gatelayer );
      }
   } 
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   return retvar;
}

//////////////////////////////////////////////////////////////////

bool MetaGraph::isEditableMap()
{
   if (m_view_class & VIEWAXIAL) {
      return getDisplayedShapeGraph().isEditable();
   }
   else if (m_view_class & VIEWDATA) {
      return getDisplayedDataMap().isEditable();
   }
   // still to do: allow editing of drawing layers
   return false;
}

ShapeMap& MetaGraph::getEditableMap()
{
   ShapeMap *map = NULL;
   if (m_view_class & VIEWAXIAL) {
      map = &(getDisplayedShapeGraph());
   }
   else if (m_view_class & VIEWDATA) {
      map = &(getDisplayedDataMap());
   }
   else {
      // still to do: allow editing of drawing layers
   }
   if (map == NULL || !map->isEditable()) {
      throw 0;
   }
   return *map;
}

bool MetaGraph::makeShape(const Line& line)
{
   if (!isEditableMap()) {
      return false;
   }
   ShapeMap& map = getEditableMap();
   return (map.makeLineShape(line,true) != -1);
}

int MetaGraph::polyBegin(const Line& line)
{
   if (!isEditableMap()) {
      return -1;
   }
   ShapeMap& map = getEditableMap();
   return map.polyBegin(line);
}

bool MetaGraph::polyAppend(int shape_ref, const Point2f& point)
{
   if (!isEditableMap()) {
      return false;
   }
   ShapeMap& map = getEditableMap();
   return map.polyAppend(shape_ref, point);
}

bool MetaGraph::polyClose(int shape_ref)
{
   if (!isEditableMap()) {
      return false;
   }
   ShapeMap& map = getEditableMap();
   return map.polyClose(shape_ref);
}

bool MetaGraph::polyCancel(int shape_ref)
{
   if (!isEditableMap()) {
      return false;
   }
   ShapeMap& map = getEditableMap();
   return map.polyCancel(shape_ref);
}

bool MetaGraph::moveSelShape(const Line& line)
{
   bool retvar = false;
   if (m_view_class & VIEWAXIAL) {
      ShapeGraph& map = getDisplayedShapeGraph();
      if (!map.isEditable()) {
         return false;
      }
      if (map.getSelCount() > 1) {
         return false;
      }
      // note, selection sets currently store rowids not uids, but moveShape sensibly works off uid:
      int rowid = *map.getSelSet().begin();
      retvar = map.moveShape(map.getIndex(rowid),line);
      if (retvar) {
         map.clearSel();
      }
   }
   else if (m_view_class & VIEWDATA) {
      ShapeMap& map = getDisplayedDataMap();
      if (!map.isEditable()) {
         return false;
      }
      if (map.getSelCount() > 1) {
         return false;
      }
      // note, selection sets currently store rowids not uids, but moveShape sensibly works off uid:
      int rowid = *map.getSelSet().begin();
      retvar = map.moveShape(map.getIndex(rowid),line);
      if (retvar) {
         map.clearSel();
      }
   }
   return retvar;
}

//////////////////////////////////////////////////////////////////

// returns 0: fail, 1: made isovist, 2: made isovist and added new shapemap layer
int MetaGraph::makeIsovist(Communicator *communicator, const Point2f& p, double startangle, double endangle, bool simple_version)
{
   int retvar = 0;
   // first make isovist
   Isovist iso;

   if (makeBSPtree(communicator)) {
      retvar = 1;
      iso.makeit(m_bsp_root,p,SuperSpacePixel::m_region, startangle, endangle);
      int shapelayer = getMapRef(m_dataMaps, "Isovists");
      if (shapelayer == -1) {
         m_dataMaps.emplace_back("Isovists",ShapeMap::DATAMAP);
         setDisplayedDataMapRef(m_dataMaps.size() - 1);
         shapelayer = m_dataMaps.size() - 1;
         m_state |= DATAMAPS;
         retvar = 2;
      }
      ShapeMap& map = m_dataMaps[shapelayer];
      // false: closed polygon, true: isovist
      int polyref = map.makePolyShape(iso.getPolygon(),false);  
      map.getAllShapes()[polyref].setCentroid(p);
      map.overrideDisplayedAttribute(-2);
      map.setDisplayedAttribute(-1);
      setViewClass(SHOWSHAPETOP);
      AttributeTable& table = map.getAttributeTable();
      int row = table.getRowid(polyref);
      iso.setData(table,row, simple_version);
   }
   return retvar;
}

static pair<double,double> startendangle( Point2f vec, double fov)
{
   pair<double,double> angles;
   // n.b. you must normalise this before getting the angle!
   vec.normalise();
   angles.first = vec.angle() - fov / 2.0;
   angles.second = vec.angle() + fov / 2.0;
   if (angles.first < 0.0) 
      angles.first += 2.0 * M_PI;
   if (angles.second > 2.0 * M_PI)
      angles.second -= 2.0 * M_PI;
   return angles;
}

// returns 0: fail, 1: made isovist, 2: made isovist and added new shapemap layer
int MetaGraph::makeIsovistPath(Communicator *communicator, double fov, bool simple_version)
{
   int retvar = 0;

   // must be showing a suitable map -- that is, one which may have polylines or lines
   ShapeMap *map = NULL, *isovists = NULL;
   int isovistmapref = -1;
   int viewclass = getViewClass() & VIEWFRONT;
   if (viewclass == VIEWAXIAL) {
      map = &getDisplayedShapeGraph();
   }
   else if (viewclass == VIEWDATA) {
      map = &getDisplayedDataMap();
   }
   else {
      return 0;
   }

   // must have a selection: the selected shapes will form the set from which to create the isovist paths
   if (!map->isSelected()) {
      return 0;
   }

   bool first = true;
   if (makeBSPtree(communicator)) {
      std::set<int> selset = map->getSelSet();
      std::map<int,SalaShape>& shapes = map->getAllShapes();
      for (auto& sel: selset) {
         const SalaShape& path = depthmapX::getMapAtIndex(shapes, sel)->second;
         if (path.isLine() || path.isPolyLine()) {
            if (first) {
               retvar = 1;
               isovistmapref = getMapRef(m_dataMaps, "Isovists");
               if (isovistmapref == -1) {
                  m_dataMaps.emplace_back("Isovists",ShapeMap::DATAMAP);
                  isovistmapref = m_dataMaps.size() - 1;
                  setDisplayedDataMapRef(isovistmapref);
                  retvar = 2;
               }
               isovists = &(m_dataMaps[isovistmapref]);
               first = false;
            }
            // now make an isovist:
            Isovist iso;
            // 
            pair<double,double> angles;
            angles.first = 0.0;
            angles.second = 0.0;
            //
            if (path.isLine()) {
               Point2f start = path.getLine().t_start();
               Point2f vec = path.getLine().vector();
               if (fov < 2.0 * M_PI) {
                  angles = startendangle(vec, fov);
               }
               iso.makeit(m_bsp_root,start,SuperSpacePixel::m_region, angles.first, angles.second);
               int polyref = isovists->makePolyShape(iso.getPolygon(),false);  
               isovists->getAllShapes()[polyref].setCentroid(start);
               AttributeTable& table = isovists->getAttributeTable();
               int row = table.getRowid(polyref);
               iso.setData(table,row, simple_version);
            }
            else {
               for (size_t i = 0; i < path.m_points.size() - 1; i++) {
                  Line li = Line(path.m_points[i],path.m_points[i+1]);
                  Point2f start = li.t_start();
                  Point2f vec = li.vector();
                  if (fov < 2.0 * M_PI) {
                     angles = startendangle(vec, fov);
                  }
                  iso.makeit(m_bsp_root,start,SuperSpacePixel::m_region, angles.first, angles.second);
                  int polyref = isovists->makePolyShape(iso.getPolygon(),false);  
                  isovists->getAllShapes().find(polyref)->second.setCentroid(start);
                  AttributeTable& table = isovists->getAttributeTable();
                  int row = table.getRowid(polyref);
                  iso.setData(table,row, simple_version);
               }
            }
         }
      }
      if (isovists) {
         isovists->overrideDisplayedAttribute(-2);
         isovists->setDisplayedAttribute(-1);
         setDisplayedDataMapRef(isovistmapref);
      }
   }
   return retvar;
}

// this version uses your own isovist (and assumes no communicator required for BSP tree
bool MetaGraph::makeIsovist(const Point2f& p, Isovist& iso)
{
   if (makeBSPtree()) {
      iso.makeit(m_bsp_root,p,SuperSpacePixel::m_region);
      return true;
   }
   return false;
}

bool MetaGraph::makeBSPtree(Communicator *communicator)
{
   if (m_bsp_tree) {
      return true;
   }

   std::vector<TaggedLine> partitionlines;
   for (size_t i = 0; i < SuperSpacePixel::size(); i++) {
      for (size_t j = 0; j < SuperSpacePixel::at(i).size(); j++) {
         // chooses the first editable layer it can find:
         if (SuperSpacePixel::at(i).at(j).isShown()) {
             auto refShapes = SuperSpacePixel::at(i).at(j).getAllShapes();
             int k = -1;
             for (auto refShape: refShapes) {
                 k++;
                 std::vector<Line> newLines = refShape.second.getAsLines();
                 // I'm not sure what the tagging was meant for any more,
                 // tagging at the moment tags the *polygon* it was original attached to
                 // must check it is not a zero length line:
                 for(Line& line: newLines) {
                     if(line.length() > 0.0) {
                         partitionlines.push_back(TaggedLine(line,k));
                     }
                 }
            }
         }
      }
   }

   if (partitionlines.size()) {
      //
      // Now we'll try the BSP tree:
      //
      if (m_bsp_root) {
         delete m_bsp_root;
         m_bsp_root = NULL;
      }
      m_bsp_root = new BSPNode();

      // Quick mod - TV
#if defined(_WIN32)      
      __time64_t atime = 0;
#else
      time_t atime = 0;
#endif      
      communicator->CommPostMessage( Communicator::NUM_RECORDS, partitionlines.size() );
      qtimer( atime, 0 );

      try {
         BSPTree::make(communicator,atime,partitionlines,m_bsp_root);
         m_bsp_tree = true;
      } 
      catch (Communicator::CancelledException) {
         m_bsp_tree = false;
         // probably best to delete the half made bastard of a tree:
         delete m_bsp_root;
         m_bsp_root = NULL;
      }
   }

   partitionlines.clear();

   return m_bsp_tree;
}

//////////////////////////////////////////////////////////////////

int MetaGraph::addShapeGraph(const std::string& name, int type)
{
   int mapref= m_shape_graphs.addMap(name,type);
   m_state |= SHAPEGRAPHS;
   setViewClass(SHOWAXIALTOP);
   // add a couple of default columns:
   AttributeTable& table = m_shape_graphs.getMap(mapref).getAttributeTable();
   table.insertLockedColumn("Connectivity");
   if ((type & ShapeMap::LINEMAP) != 0) {
      table.insertLockedColumn("Line Length");
   }
   return mapref;
}
int MetaGraph::addShapeMap(const std::string& name)
{
   m_dataMaps.emplace_back(name,ShapeMap::DATAMAP);
   m_state |= DATAMAPS;
   setViewClass(SHOWSHAPETOP);
   return m_dataMaps.size() - 1;
}
void MetaGraph::scaleAll() {
    int pointMapIdx = getDisplayedPointMapRef();
    PointMap& pointMap = getDisplayedPointMap();

    double scale = 266.667794066544743;

    bool graphRegionReset = false;
    for (int i = 0; i < getLineFileCount(); i++) {
        bool fileRegionReset = false;
        for (int j = 0; j < getLineLayerCount(i); j++) {
           ShapeMap& map = getLineLayer(i,j);

           std::map<int, SalaShape>& shapes = map.getAllShapes();
           for(auto& shape: shapes) {
               if(shape.second.isLine()) {
                   shape.second.scaleRegion(scale);
               } else {
                   for(auto& pnt: shape.second.m_points) {
                       pnt.x *= scale;
                       pnt.y *= scale;
                   }

               }
           }
           map.recreateBounds();
           if(!fileRegionReset) {
               SuperSpacePixel::at(i).m_region = map.getRegion();
               fileRegionReset = true;
           } else {
               SuperSpacePixel::at(i).m_region = runion(SuperSpacePixel::at(i).m_region, map.getRegion());
           }
           if(!graphRegionReset) {
               SuperSpacePixel::m_region = map.getRegion();
               graphRegionReset = true;
           } else {
               SuperSpacePixel::m_region = runion(SuperSpacePixel::m_region, map.getRegion());
           }
        }
    }

    int newPointMapIdx = addNewPointMap();
    PointMap& newPointMap = getPointMaps()[size_t(newPointMapIdx)];
    newPointMap.setSpacePixel( (SuperSpacePixel *) this );
    newPointMap.setGrid(pointMap.getSpacing()*scale);
    for (int i = 0; i < pointMap.m_cols; i++) {
       for (int j = 0; j < pointMap.m_rows; j++) {
          PixelRef curs = PixelRef( i, j );
          Point2f cursPnt = pointMap.depixelate(curs);
          cursPnt.x *= scale;
          cursPnt.y *= scale;
          PixelRef cursOnNewPointmap = newPointMap.pixelate(cursPnt, false);
          Point& pt = pointMap.getPoint( curs );
          if ( pt.filled()) {
              newPointMap.m_points[cursOnNewPointmap.x][cursOnNewPointmap.y].set(pt.getState());
          }
       }
    }

    newPointMap.sparkGraph2(nullptr, pointMap.isBoundaryGraph() ? 1 : 0, -1);

    for(int i = 0; i < pointMap.getAttributeTable().getColumnCount(); i++) {
        if(newPointMap.getAttributeTable().getColumnIndex(pointMap.getAttributeTable().getColumnName(i)) == -1)
            newPointMap.getAttributeTable().insertColumn(pointMap.getAttributeTable().getColumnName(i));
    }
    for (int i = 0; i < pointMap.m_cols; i++) {
       for (int j = 0; j < pointMap.m_rows; j++) {
          PixelRef curs = PixelRef( i, j );
          Point2f cursPnt = pointMap.depixelate(curs);
          cursPnt.x *= scale;
          cursPnt.y *= scale;
          PixelRef cursOnNewPointmap = newPointMap.pixelate(cursPnt, false);
          Point& pt = pointMap.getPoint( curs );
          if ( pt.filled()) {
              newPointMap.m_points[cursOnNewPointmap.x][cursOnNewPointmap.y].set(pt.getState());
              for(int i = 0; i < pointMap.getAttributeTable().getColumnCount(); i++) {
                  int cursId = pointMap.getAttributeTable().getRowid(curs);
                  int newCursId = newPointMap.getAttributeTable().getRowid(cursOnNewPointmap);
                  newPointMap.getAttributeTable().setValue(newCursId, i,
                                                        pointMap.getAttributeTable().getValue(cursId, i));
              }
          }
       }
    }
    for (int i = 0; i < pointMap.m_cols; i++) {
       for (int j = 0; j < pointMap.m_rows; j++) {
          PixelRef curs = PixelRef( i, j );
          Point2f cursPnt = pointMap.depixelate(curs);
          cursPnt.x *= scale;
          cursPnt.y *= scale;
          PixelRef cursOnNewPointmap = newPointMap.pixelate(cursPnt, false);
          Point& pt = pointMap.getPoint( curs );
          if ( pt.filled()) {
              if(!pt.m_merge.empty()) {
                  Point2f mergePnt = pointMap.depixelate(pt.m_merge);
                  mergePnt.x *= scale;
                  mergePnt.y *= scale;
                  PixelRef mergeOnNewPointMap = newPointMap.pixelate(mergePnt, false);
                  newPointMap.getPoint( mergeOnNewPointMap ).m_merge = cursOnNewPointmap;
                  newPointMap.getPoint( cursOnNewPointmap ).m_merge = mergeOnNewPointMap;
              }
          }
       }
    }
    setDisplayedPointMapRef(newPointMapIdx);
    removePointMap(pointMapIdx);

}
void MetaGraph::moveAll() {
    int pointMapIdx = getDisplayedPointMapRef();
    PointMap& pointMap = getDisplayedPointMap();

    double minX = 61089.8505;
    double maxX = 94485.8007;
    double minY = 61366.2264;
    double maxY = 82024.3749;

    double offsetX = 67660.3192 - 65003.6471;
    double offsetY = 130472.9945 - 78910.8258;

//    int offsetPixX = -120;
//    int offsetPixY = -400;
//    double offsetX = pointMap.getSpacing()*offsetPixX;
//    double offsetY = pointMap.getSpacing()*offsetPixY;

    bool graphRegionReset = false;
    for (int i = 0; i < getLineFileCount(); i++) {
        bool fileRegionReset = false;
        for (int j = 0; j < getLineLayerCount(i); j++) {
           ShapeMap& map = getLineLayer(i,j);

           std::map<int, SalaShape>& shapes = map.getAllShapes();
           int shapesChanged = 0;
           for(auto& shape: shapes) {
               if(shape.second.isLine()) {
                   const Line& line = shape.second.getLine();
                   if((minX < line.ax() && line.ax() < maxX && minY < line.ay() && line.ay() < maxY) ||
                      (minX < line.bx() && line.bx() < maxX && minY < line.by() && line.by() < maxY)) {
                       shape.second.moveRegion(Point2f(offsetX, offsetY));
                       shapesChanged++;
                   }
               } else {
                   bool isInRange = false;
                   for(auto& pnt: shape.second.m_points) {
                       if(minX < pnt.x && pnt.x < maxX && minY < pnt.y && pnt.y < maxY) {
                           isInRange = true;
                           break;
                       }
                   }
                   if(isInRange) {
                       for(auto& pnt: shape.second.m_points) {
                           pnt.x += offsetX;
                           pnt.y += offsetY;
                       }
                       shapesChanged++;
                   }

               }
           }
           if(shapesChanged > 0) {
               map.recreateBounds();
               if(!fileRegionReset) {
                   SuperSpacePixel::at(size_t(i)).m_region = map.getRegion();
                   fileRegionReset = true;
               } else {
                   SuperSpacePixel::at(size_t(i)).m_region = runion(SuperSpacePixel::at(size_t(i)).m_region, map.getRegion());
               }
               if(!graphRegionReset) {
                   SuperSpacePixel::m_region = map.getRegion();
                   graphRegionReset = true;
               } else {
                   SuperSpacePixel::m_region = runion(SuperSpacePixel::m_region, map.getRegion());
               }

           }
        }
    }

    int newPointMapIdx = addNewPointMap();
    PointMap& newPointMap = getPointMaps()[size_t(newPointMapIdx)];
    newPointMap.setSpacePixel( (SuperSpacePixel *) this );
    newPointMap.setGrid(pointMap.getSpacing());
    for (int i = 0; i < pointMap.m_cols; i++) {
       for (int j = 0; j < pointMap.m_rows; j++) {
          PixelRef curs = PixelRef( short(i), short(j) );
          Point2f cursPoint = pointMap.depixelate(curs);
          PixelRef cursOnNewPointmap = newPointMap.pixelate(cursPoint, false);
          PixelRef movedCurs = newPointMap.pixelate( Point2f(cursPoint.x + offsetX, cursPoint.y + offsetY), false);
          Point& pt = pointMap.getPoint( curs );
          if ( pt.filled()) {
              if(minX < pt.m_location.x && pt.m_location.x < maxX
                      && minY < pt.m_location.y && pt.m_location.y < maxY) {
                  newPointMap.m_points[movedCurs.x][movedCurs.y].set(pt.getState());
              } else {
                  newPointMap.m_points[cursOnNewPointmap.x][cursOnNewPointmap.y].set(pt.getState());
              }
          }
       }
    }

    newPointMap.sparkGraph2(nullptr, pointMap.isBoundaryGraph() ? 1 : 0, -1);

    for(int i = 0; i < pointMap.getAttributeTable().getColumnCount(); i++) {
        if(newPointMap.getAttributeTable().getColumnIndex(pointMap.getAttributeTable().getColumnName(i)) == -1)
            newPointMap.getAttributeTable().insertColumn(pointMap.getAttributeTable().getColumnName(i));
    }
    for (int i = 0; i < pointMap.m_cols; i++) {
       for (int j = 0; j < pointMap.m_rows; j++) {
          PixelRef curs = PixelRef( short(i), short(j) );
          Point2f cursPoint = pointMap.depixelate(curs);
          PixelRef cursOnNewPointmap = newPointMap.pixelate(cursPoint, false);
          PixelRef movedCurs = newPointMap.pixelate( Point2f(cursPoint.x + offsetX, cursPoint.y + offsetY), false);
          Point& pt = pointMap.getPoint( curs );
          if ( pt.filled()) {
              if(minX < pt.m_location.x && pt.m_location.x < maxX
                      && minY < pt.m_location.y && pt.m_location.y < maxY) {
                  newPointMap.m_points[movedCurs.x][movedCurs.y].set(pt.getState());
                  for(int i = 0; i < pointMap.getAttributeTable().getColumnCount(); i++) {
                      int cursId = pointMap.getAttributeTable().getRowid(curs);
                      int newCursId = newPointMap.getAttributeTable().getRowid(movedCurs);
                      newPointMap.getAttributeTable().setValue(newCursId, i,
                                                            pointMap.getAttributeTable().getValue(cursId, i));
                  }
              } else {
                  newPointMap.m_points[cursOnNewPointmap.x][cursOnNewPointmap.y].set(pt.getState());
                  for(int i = 0; i < pointMap.getAttributeTable().getColumnCount(); i++) {
                      int cursId = pointMap.getAttributeTable().getRowid(curs);
                      int newCursId = newPointMap.getAttributeTable().getRowid(cursOnNewPointmap);
                      newPointMap.getAttributeTable().setValue(newCursId, i,
                                                            pointMap.getAttributeTable().getValue(cursId, i));
                  }
              }
          }
       }
    }
    for (int i = 0; i < pointMap.m_cols; i++) {
       for (int j = 0; j < pointMap.m_rows; j++) {
          PixelRef curs = PixelRef( short(i), short(j) );
          Point2f cursPoint = pointMap.depixelate(curs);
          PixelRef cursOnNewPointmap = newPointMap.pixelate(cursPoint, false);
          Point& pt = pointMap.getPoint( curs );
          if ( pt.filled()) {
              if(minX < pt.m_location.x && pt.m_location.x < maxX
                      && minY < pt.m_location.y && pt.m_location.y < maxY) {
                  PixelRef movedCurs = newPointMap.pixelate( Point2f(cursPoint.x + offsetX, cursPoint.y + offsetY), false);
                  if(!pt.m_merge.empty()) {
                      Point2f mergePnt = pointMap.depixelate(pt.m_merge);
                      PixelRef mergeOnNewPointMap = newPointMap.pixelate(mergePnt, false);
                      if(minX < mergePnt.x && mergePnt.x < maxX
                              && minY < mergePnt.y && mergePnt.y < maxY) {
                          PixelRef movedMerge = newPointMap.pixelate( Point2f(mergePnt.x + offsetX, mergePnt.y + offsetY), false);
                          newPointMap.getPoint( movedMerge ).m_merge = movedCurs;
                          newPointMap.getPoint( movedCurs ).m_merge = movedMerge;
                      } else {
                          newPointMap.getPoint( mergeOnNewPointMap ).m_merge = movedCurs;
                          newPointMap.getPoint( movedCurs ).m_merge = mergeOnNewPointMap;
                      }
                  }
              } else {
                  if(!pt.m_merge.empty()) {
                      Point2f mergePnt = pointMap.depixelate(pt.m_merge);
                      PixelRef mergeOnNewPointMap = newPointMap.pixelate(mergePnt, false);
                      if(minX < mergePnt.x && mergePnt.x < maxX
                              && minY < mergePnt.y && mergePnt.y < maxY) {
                          PixelRef movedMerge = newPointMap.pixelate( Point2f(mergePnt.x + offsetX, mergePnt.y + offsetY), false);
                          newPointMap.getPoint( movedMerge ).m_merge = cursOnNewPointmap;
                          newPointMap.getPoint( cursOnNewPointmap ).m_merge = movedMerge;
                      } else {
                          newPointMap.getPoint( mergeOnNewPointMap ).m_merge = cursOnNewPointmap;
                          newPointMap.getPoint( cursOnNewPointmap ).m_merge = mergeOnNewPointMap;
                      }
                  }
              }
          }
       }
    }
    setDisplayedPointMapRef(newPointMapIdx);
    removePointMap(pointMapIdx);
}
void MetaGraph::removeDisplayedMap()
{
   int ref = getDisplayedMapRef();
   switch (m_view_class & VIEWFRONT) {
   case VIEWVGA:
      removePointMap(ref);
      if (m_pointMaps.empty()) {
         setViewClass(SHOWHIDEVGA);
         m_state &= ~POINTMAPS;
      }
      break;
   case VIEWAXIAL:
      m_shape_graphs.removeMap(ref);
      if (m_shape_graphs.getMapCount() == 0) {
         setViewClass(SHOWHIDEAXIAL);
         m_state &= ~SHAPEGRAPHS;
      }
      break;
   case VIEWDATA:
      removeDataMap(ref);
      if (m_dataMaps.empty()) {
         setViewClass(SHOWHIDESHAPE);
         m_state &= ~DATAMAPS;
      }
      break;
   }
}


//////////////////////////////////////////////////////////////////


bool MetaGraph::convertDrawingToAxial(Communicator *comm, std::string layer_name)
{
   int oldstate = m_state;

   m_state &= ~SHAPEGRAPHS;

   bool retvar = false;
   
   try {
      int mapref = m_shape_graphs.convertDrawingToAxial( comm, layer_name, (SuperSpacePixel&) *this );
      if (mapref != -1) {
         retvar = true;
      }
   } 
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   m_state |= oldstate;

   if (retvar) {
      m_state |= SHAPEGRAPHS;
      setViewClass(SHOWAXIALTOP);
   }

   return retvar;
}

bool MetaGraph::convertDataToAxial(Communicator *comm, std::string layer_name, bool keeporiginal, bool pushvalues)
{
   int oldstate = m_state;

   m_state &= ~SHAPEGRAPHS;

   bool retvar = false;
   
   try {
      int mapref = m_shape_graphs.convertDataToAxial( comm, layer_name, getDisplayedDataMap(), pushvalues );
      if (mapref != -1) {
         retvar = true;
      }
   } 
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   m_state |= oldstate;

   if (retvar) {
      if (!keeporiginal) {
         removeDataMap( getDisplayedDataMapRef() );
         if (m_dataMaps.empty()) {
            setViewClass(SHOWHIDESHAPE);
            m_state &= ~DATAMAPS;
         }
      }
      m_state |= SHAPEGRAPHS;
      setViewClass(SHOWAXIALTOP);
   }

   return retvar;
}

// typeflag: -1 convert drawing to convex, 0 or 1, convert data to convex (1 is pushvalues)
bool MetaGraph::convertToConvex(Communicator *comm, std::string layer_name, bool keeporiginal, int typeflag)
{
   int oldstate = m_state;

   m_state &= ~SHAPEGRAPHS; // and convex maps...

   bool retvar = false;
   
   try {
      int mapref;
      if (typeflag == -1) {
         mapref = m_shape_graphs.convertDrawingToConvex( comm, layer_name, (SuperSpacePixel&) *this );
      }
      else {
         mapref = m_shape_graphs.convertDataToConvex( comm, layer_name, getDisplayedDataMap(), (typeflag != 0) );
      }
      if (mapref != -1) {
         retvar = true;
      }
   } 
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   m_state |= oldstate;

   if (retvar) {
      if (typeflag != -1 && !keeporiginal) {
         removeDataMap( getDisplayedDataMapRef() );
         if (m_dataMaps.empty()) {
            setViewClass(SHOWHIDESHAPE);
            m_state &= ~DATAMAPS;
         }
      }
      m_state |= SHAPEGRAPHS;
      setViewClass(SHOWAXIALTOP);
   }

   return retvar;
}

bool MetaGraph::convertDrawingToSegment(Communicator *comm, std::string layer_name)
{
   int oldstate = m_state;

   m_state &= ~SHAPEGRAPHS;

   bool retvar = false;
   
   try {
      int mapref = m_shape_graphs.convertDrawingToSegment( comm, layer_name, (SuperSpacePixel&) *this );
      if (mapref != -1) {
         retvar = true;
      }
   } 
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   m_state |= oldstate;

   if (retvar) {
      m_state |= SHAPEGRAPHS;
      setViewClass(SHOWAXIALTOP);
   }

   return retvar;
}

bool MetaGraph::convertDataToSegment(Communicator *comm, std::string layer_name, bool keeporiginal, bool pushvalues)
{
   int oldstate = m_state;

   m_state &= ~SHAPEGRAPHS;

   bool retvar = false;
   
   try {
      int mapref = m_shape_graphs.convertDataToSegment( comm, layer_name, getDisplayedDataMap(), pushvalues );
      if (mapref != -1) {
         retvar = true;
      }
   } 
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   m_state |= oldstate;

   if (retvar) {
      if (!keeporiginal) {
         removeDataMap( getDisplayedDataMapRef() );
         if (m_dataMaps.empty()) {
            setViewClass(SHOWHIDESHAPE);
            m_state &= ~DATAMAPS;
         }
      }
      m_state |= SHAPEGRAPHS;
      setViewClass(SHOWAXIALTOP);
   }

   return retvar;
}

// note: type flag says whether this is graph to data map or drawing to data map

bool MetaGraph::convertToData(Communicator *comm, std::string layer_name, bool keeporiginal, int typeflag)
{
   int oldstate = m_state;

   m_state &= ~DATAMAPS;

   bool retvar = false;
   
   try {
      // This should be much easier than before,
      // simply move the shapes from the drawing layer
      // note however that more than one layer might be combined:
      // create map layer...
      m_dataMaps.emplace_back(layer_name,ShapeMap::DATAMAP);
      int destmapref = m_dataMaps.size() - 1;
      ShapeMap& destmap = m_dataMaps.back();
      AttributeTable& table = destmap.getAttributeTable();
      int count = 0;
      //
      // drawing to data
      if (typeflag == -1) {
         int layercol = destmap.addAttribute("Drawing Layer");
         // add all visible layers to the set of map:
         for (size_t i = 0; i < SuperSpacePixel::size(); i++) {
            for (size_t j = 0; j < SuperSpacePixel::at(i).size(); j++) {
               if (SuperSpacePixel::at(i).at(j).isShown()) {
                  auto refShapes = SuperSpacePixel::at(i).at(j).getAllShapes();
                  for (auto refShape: refShapes) {
                     int key = destmap.makeShape(refShape.second);
                     table.setValue(table.getRowid(key),layercol,float(j+1));
                     count++;
                  }
                  SuperSpacePixel::at(i).at(j).setShow(false);
               }
            }
         }
      }
      // convex, axial or segment graph to data (similar)
      else {
         ShapeGraph& sourcemap = getDisplayedShapeGraph();
         count = sourcemap.getShapeCount();
         // take viewed graph and push all geometry to it (since it is *all* geometry, pushing is easy)
         int copyflag = (typeflag == 0) ? (ShapeMap::COPY_GEOMETRY) : (ShapeMap::COPY_GEOMETRY | ShapeMap::COPY_ATTRIBUTES);
         destmap.copy(sourcemap, copyflag);
      }
      //
      if (count == 0) {
         // if no objects converted then a crash is caused, so remove it:
         removeDataMap(destmapref);
         retvar = false;
      }
      else {
         // we can stop here! -- remember to set up display:
         setDisplayedDataMapRef(destmapref);
         destmap.invalidateDisplayedAttribute();
         destmap.setDisplayedAttribute(-1);
         retvar = true;
      }
   } 
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   m_state |= oldstate;

   if (retvar) {
      if (typeflag != -1 && !keeporiginal) {
         m_shape_graphs.removeMap( m_shape_graphs.getDisplayedMapRef() );
         if (m_shape_graphs.getMapCount() == 0) {
            setViewClass(SHOWHIDEAXIAL);
            m_state &= ~SHAPEGRAPHS;
         }
      }
      m_state |= DATAMAPS;
      setViewClass(SHOWSHAPETOP);
   }

   return retvar;
}

bool MetaGraph::convertToDrawing(Communicator *comm, std::string layer_name, int typeflag)
{
   bool retvar = false;

   int oldstate = m_state;

   m_state &= ~LINEDATA;

   try {
      const ShapeMap *sourcemap;
      if (typeflag == 0) {
         sourcemap = &(getDisplayedDataMap());
      }
      else {
         sourcemap = &(getDisplayedShapeGraph());
      }
      //
      if (sourcemap->getShapeCount() != 0) {
         // this is very simple: create a new drawing layer, and add the data...
         int group = -1;
         for (size_t i = 0; i < SuperSpacePixel::size(); i++) {
            if (SuperSpacePixel::at(i).getName() == "Converted Maps") {
               group = i;
            }
         }
         if (group == -1) {
            SuperSpacePixel::push_back(std::string("Converted Maps"));
            group = SuperSpacePixel::size() - 1;
         }
         SuperSpacePixel::at(group).push_back(ShapeMap(layer_name));
         SuperSpacePixel::at(group).tail().copy(*sourcemap, ShapeMap::COPY_GEOMETRY);
         //
         // dummy set still required:
         SuperSpacePixel::at(group).tail().invalidateDisplayedAttribute();
         SuperSpacePixel::at(group).tail().setDisplayedAttribute(-1);
         //      
         // two levels of merge region:
         if (SuperSpacePixel::at(group).size() == 1) {
            SuperSpacePixel::at(group).m_region = sourcemap->getRegion();
         }
         else {
            SuperSpacePixel::at(group).m_region = runion(SuperSpacePixel::at(group).m_region, sourcemap->getRegion());
         }
         if (SuperSpacePixel::size() == 1) {
            SuperSpacePixel::m_region = SuperSpacePixel::at(group).m_region;
         }
         else {
            SuperSpacePixel::m_region = runion(SuperSpacePixel::m_region, SuperSpacePixel::at(group).m_region);
         }
         //
         retvar = true;
      }
      retvar = true;
   }
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   m_state |= oldstate;

   if (retvar) {
      m_state |= LINEDATA;
   }

   return retvar;
}

bool MetaGraph::convertPointsToShape()
{
   bool retvar = false;
   int oldstate = m_state;

   m_state &= ~DATAMAPS;

   if (m_dataMaps.empty()) {
      m_dataMaps.emplace_back("Gates",ShapeMap::DATAMAP);
   }

   if (getDisplayedDataMap().makeShapeFromPointSet(getDisplayedPointMap()) != -1) {
      getDisplayedPointMap().clearSel();
      // override the displayed attribute and redisplay:
      getDisplayedDataMap().overrideDisplayedAttribute(-2);
      getDisplayedDataMap().setDisplayedAttribute(-1);
      // set up a specifc view class to show both layers:
      m_view_class = VIEWVGA | VIEWBACKDATA;
      m_state |= DATAMAPS;
      retvar = true;
   }
   else if (!oldstate) {
      removeDataMap(0);
   }

   m_state |= oldstate;

   return retvar;
}

/*
bool MetaGraph::convertBoundaryGraph( Communicator *communicator )
{
   m_state &= ~SHAPEGRAPHS;      // Clear axial map data flag (stops accidental redraw during reload) 

   bool retvar = false;

   try {
      retvar = m_shape_graphs.convertBoundaryGraph( communicator, (PointMap&) *this );
   } 
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   if (retvar) {
      m_state |= SHAPEGRAPHS;
      setViewClass(SHOWAXIALTOP);
   }

   return retvar;
}
*/

bool MetaGraph::convertAxialToSegment(Communicator *comm, std::string layer_name, bool keeporiginal, bool pushvalues, double stubremoval)
{
   int oldstate = m_state;

   m_state &= ~SHAPEGRAPHS;

   bool retvar = false;

   int orig_ref = m_shape_graphs.getDisplayedMapRef();

   try {
      int mapref = m_shape_graphs.convertAxialToSegment( comm, layer_name, keeporiginal, pushvalues, stubremoval);
      if (mapref != -1) {
         retvar = true;
      }
   }
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   m_state |= oldstate;

   if (retvar) {
      if (!keeporiginal) {
         m_shape_graphs.removeMap(orig_ref);
      }
      m_state |= SHAPEGRAPHS;
      setViewClass(SHOWAXIALTOP);
   }

   return retvar;
}

int MetaGraph::loadMifMap(Communicator *comm, istream& miffile, istream& midfile)
{
   int oldstate = m_state;
   m_state &= ~DATAMAPS;

   int retvar = -1;
   
   try {
      // create map layer...
      m_dataMaps.emplace_back(comm->GetMBInfileName(),ShapeMap::DATAMAP);
      int mifmapref = m_dataMaps.size() - 1;
      ShapeMap& mifmap = m_dataMaps.back();
      retvar = mifmap.loadMifMap(miffile, midfile);
      if (retvar == MINFO_OK || retvar == MINFO_MULTIPLE) { // multiple is just a warning
          // display an attribute:
         mifmap.overrideDisplayedAttribute(-2);
         mifmap.setDisplayedAttribute(-1);
         setDisplayedDataMapRef(mifmapref);
      }
      else { // error: undo!
         removeDataMap(mifmapref);
      }
   } 
   catch (Communicator::CancelledException) {
      retvar = -1;
   }

   m_state = oldstate;

   if (retvar == MINFO_OK || retvar == MINFO_MULTIPLE) { // MINFO_MULTIPLE is simply a warning
      m_state |= DATAMAPS;
      setViewClass(SHOWSHAPETOP);
   }

   return retvar;
}  

bool MetaGraph::makeAllLineMap( Communicator *communicator, const Point2f& seed )
{
   int oldstate = m_state;
   m_state &= ~SHAPEGRAPHS;      // Clear axial map data flag (stops accidental redraw during reload) 

   bool retvar = false;

   try {
      retvar = m_shape_graphs.makeAllLineMap( communicator, (SuperSpacePixel&) *this, seed );
   } 
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   m_state = oldstate;

   if (retvar) {
      m_state |= SHAPEGRAPHS;
      setViewClass(SHOWAXIALTOP);
   }

   return retvar;
}


bool MetaGraph::makeFewestLineMap( Communicator *communicator, int replace )
{
   int oldstate= m_state;
   m_state &= ~SHAPEGRAPHS;      // Clear axial map data flag (stops accidental redraw during reload) 

   bool retvar = false;

   try {
      retvar = m_shape_graphs.makeFewestLineMap(communicator, (replace != 0));
   } 
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   m_state = oldstate;

   if (retvar) {
      m_state |= SHAPEGRAPHS;   // note: should originally have at least one axial map
      setViewClass(SHOWAXIALTOP);
   }

   return retvar;
}

bool MetaGraph::analyseAxial( Communicator *communicator, Options options, bool simple_version ) // options copied to keep thread safe
{
   m_state &= ~SHAPEGRAPHS;      // Clear axial map data flag (stops accidental redraw during reload) 

   bool retvar = false;

   try {
      pvecint radius;
      for (size_t i = 0; i < options.radius_list.size(); i++) {
         radius.push_back( (int) options.radius_list[i] );
      }
      retvar = m_shape_graphs.getDisplayedMap().integrate( communicator, radius, options.choice, options.local, options.fulloutput, options.weighted_measure_col, simple_version );
   } 
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   m_state |= SHAPEGRAPHS;

   return retvar;
}

bool MetaGraph::analyseSegments( Communicator *communicator, Options options ) // <- options copied to keep thread safe
{
   m_state &= ~SHAPEGRAPHS;      // Clear axial map data flag (stops accidental redraw during reload) 

   bool retvar = false;

   try {
      if (options.tulip_bins == 0) {
         retvar = m_shape_graphs.getDisplayedMap().analyseAngular(communicator, options.radius_list);
      }
      else {
         retvar = m_shape_graphs.getDisplayedMap().analyseTulip(communicator, options.tulip_bins, options.choice, 
                                                                 options.radius_type, options.radius_list, options.weighted_measure_col);
      }
   } 
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   m_state |= SHAPEGRAPHS;

   return retvar;
}

bool MetaGraph::analyseTopoMet( Communicator *communicator, Options options ) // <- options copied to keep thread safe
{
   m_state &= ~SHAPEGRAPHS;      // Clear axial map data flag (stops accidental redraw during reload) 

   bool retvar = false;

   try {
      // note: "output_type" reused for analysis type (either 0 = topological or 1 = metric)
      retvar = m_shape_graphs.getDisplayedMap().analyseTopoMet(communicator, options.output_type, options.radius, options.sel_only);
   } 
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   m_state |= SHAPEGRAPHS;

   return retvar;
}

int MetaGraph::loadLineData( Communicator *communicator, int load_type )
{
    if (load_type & DXF) {
       // separate the stream and the communicator, allowing non-file streams read
       return depthmapX::importFile(*this, *communicator, communicator, communicator->GetMBInfileName(), depthmapX::ImportType::DRAWINGMAP, depthmapX::ImportFileType::DXF);
    }

   m_state &= ~LINEDATA;      // Clear line data flag (stops accidental redraw during reload) 

   // if bsp tree exists 
   if (m_bsp_root) {
      delete m_bsp_root;
      m_bsp_root = NULL;
   }
   m_bsp_tree = false;

   if (load_type & REPLACE) {
      SuperSpacePixel::clear();
   }

   SuperSpacePixel::push_back(communicator->GetMBInfileName());

   if (load_type & CAT) {
      // separate the stream and the communicator, allowing non-file streams read
      int error = loadCat(*communicator, communicator);
      if (error != 1) {
         return error;
      }
   }
   else if (load_type & RT1) {
      // separate the stream and the communicator, allowing non-file streams read
      int error = loadRT1(communicator->GetFileSet(), communicator);
      if (error != 1) {
         return error;
      }
   }
   else if (load_type & NTF) {
   
      NtfMap map;

      try {
         map.open(communicator->GetFileSet(), communicator);
      }
      catch (Communicator::CancelledException) {
         SuperSpacePixel::pop_back();
         return 0;
      }
      catch (pexception) {
         SuperSpacePixel::pop_back();
         return -1;
      }

      if (communicator->IsCancelled()) {
         SuperSpacePixel::pop_back();
         return 0;
      }

      SuperSpacePixel::tail().m_region = map.getRegion();;

      for (size_t i = 0; i < map.size(); i++) {

         SuperSpacePixel::tail().push_back(ShapeMap(map[i].getName()));
         SuperSpacePixel::tail().at(i).init(map[i].getLineCount(), map.getRegion());

         for (size_t j = 0; j < map[i].size(); j++) {

            for (size_t k = 0; k < map[i][j].size(); k++) {

               SuperSpacePixel::tail().at(i).makeLineShape( map[i][j][k] );
            }
         }

         SuperSpacePixel::tail().at(i).setDisplayedAttribute(-2);
         SuperSpacePixel::tail().at(i).setDisplayedAttribute(-1);
      }
   }

   if (SuperSpacePixel::size() == 1) {
      SuperSpacePixel::m_region = SuperSpacePixel::tail().m_region;
   }
   else {
      SuperSpacePixel::m_region = runion(SuperSpacePixel::m_region, SuperSpacePixel::tail().m_region);
   }

   m_state |= LINEDATA;

   return 1;
}

int MetaGraph::loadCat( istream& stream, Communicator *communicator )
{
   if (communicator) {
      long size = communicator->GetInfileSize();
      communicator->CommPostMessage( Communicator::NUM_RECORDS, size );
   }

   // Quick mod - TV
#if defined(_WIN32)   
   __time64_t a_time = 0;
#else
   time_t a_time = 0;
#endif
   
   qtimer( a_time, 0 );

   long size = 0; 
   int numlines = 0;
   int parsing = 0;
   bool first = true;

   Point2f current_point, min_point, max_point;

   while (!stream.eof()) {

      std::string inputline;
      stream >> inputline;
      if (inputline.length() > 1 && inputline[0] != '#') {
         if (!parsing) {
            if (dXstring::toLower(inputline) == "begin polygon") {
               parsing = 1;
            }
            else if (dXstring::toLower(inputline) == "begin polyline") {
               parsing = 2;
            }
         }
         else if (dXstring::toLower(inputline).substr(0,3) == "end") {
            parsing = 0;
         }
         else {
            auto tokens = dXstring::split(inputline, ' ', true);
            current_point.x = stod(tokens[0]);
            current_point.y = stod(tokens[1]);
            numlines++;
            if (first) {
               min_point = current_point;
               max_point = current_point;
               first = false;
            }
            else {
               if (current_point.x < min_point.x) {
                  min_point.x = current_point.x;
               }
               if (current_point.y < min_point.y) {
                  min_point.y = current_point.y;
               }
               if (current_point.x > max_point.x) {
                  max_point.x = current_point.x;
               }
               if (current_point.y > max_point.y) {
                  max_point.y = current_point.y;
               }
            }
         }
      }
   }
   SuperSpacePixel::tail().m_region = QtRegion(min_point, max_point);
   SuperSpacePixel::tail().push_back(ShapeMap());
   SuperSpacePixel::tail().tail().init( numlines, QtRegion(min_point, max_point) );

   // in MSVC 6, ios::eof remains set and it needs to be cleared.
   // in MSVC 8 it's even worse: it won't even seekg until eof flag has been cleared
   stream.clear();
   stream.seekg(0, ios::beg);

   parsing = 0;
   first = true;
   std::vector<Point2f> points;

   while (!stream.eof()) {

      std::string inputline;
      stream >> inputline;

      if (inputline.length() > 1 && inputline[0] != '#') {
         if (!parsing) {
            if (dXstring::toLower(inputline) == "begin polygon") {
               parsing = 1;
               first = true;
            }
            else if (dXstring::toLower(inputline) == "begin polyline") {
               parsing = 2;
               first = true;
            }
         }
         else if (dXstring::toLower(inputline).substr(0,3) == "end") {
            if (points.size() > 2) {
               if (parsing == 1) { // polygon
                  SuperSpacePixel::tail().tail().makePolyShape(points, false);
               }
               else { // polyline
                  SuperSpacePixel::tail().tail().makePolyShape(points, true);
               }
            }
            else if (points.size() == 2) {
               SuperSpacePixel::tail().tail().makeLineShape(Line(points[0],points[1]));
            }
            points.clear();
            parsing = 0;
         }
         else {
             auto tokens = dXstring::split(inputline, ' ', true);
            current_point.x = stod(tokens[0]);
            current_point.y = stod(tokens[1]);
            points.push_back(current_point);
         }
      }

      size += inputline.length() + 1;

      if (communicator) {
         if (qtimer( a_time, 500 )) {
            if (communicator->IsCancelled()) {
               throw Communicator::CancelledException();
            }
            communicator->CommPostMessage( Communicator::CURRENT_RECORD, size );
         }
      }
   }

   SuperSpacePixel::tail().tail().setDisplayedAttribute(-2);
   SuperSpacePixel::tail().tail().setDisplayedAttribute(-1);

   return 1;
}

int MetaGraph::loadRT1(const std::vector<string>& fileset, Communicator *communicator)
{
   TigerMap map;

   try {
      map.parse( fileset, communicator );
   }
   catch (Communicator::CancelledException) {
      SuperSpacePixel::pop_back();
      return 0;
   }
   catch (pexception) {
      SuperSpacePixel::pop_back();
      return -1;
   }

   if (communicator->IsCancelled()) {
      SuperSpacePixel::pop_back();
      return 0;
   }

   SuperSpacePixel::tail().m_region = QtRegion(map.getBottomLeft(), map.getTopRight());

   // for each category
   for (auto val: map.m_categories) {
      ShapeMap shapeMap = ShapeMap(val.first);
      shapeMap.init(val.second.chains.size(), map.getRegion() );

      // for each chains in category:
      for (size_t j = 0; j < val.second.chains.size(); j++) {
         // for each node pair in each category
         for (size_t k = 0; k < val.second.chains[j].lines.size(); k++) {
            shapeMap.makeLineShape( val.second.chains[j].lines[k] );
         }
      }

      shapeMap.setDisplayedAttribute(-2);
      shapeMap.setDisplayedAttribute(-1);
      SuperSpacePixel::tail().push_back(shapeMap);
   
   }

   return 1;
}

bool MetaGraph::importCat(istream& filecontents)
{
   // any name for the file will do...
   SuperSpacePixel::push_back(SpacePixelFile("salad"));

   // load the data from the file
   if (!loadCat( filecontents, NULL )) {
      return false;
   }

   if (SuperSpacePixel::size() == 1) {
      SuperSpacePixel::m_region = SuperSpacePixel::tail().m_region;
   }
   else {
      SuperSpacePixel::m_region = runion(SuperSpacePixel::m_region, SuperSpacePixel::tail().m_region);
   }

   m_state |= LINEDATA;

   return true;
}

// a second does the lot, especially for evolutionary graphs
// essentially, hand the ecoevograph a new meta graph each time...
// it'll do everything for you

void MetaGraph::fastGraph( const Point2f& seed, double spacing )
{
   setGrid( spacing );

   getDisplayedPointMap().makePoints(seed, 0); // 0 = not semifilled

   m_state |= POINTMAPS;

   getDisplayedPointMap().sparkGraph2(NULL, 0, -1.0);

   // historical tag - essentially stamps version, so kept:
   m_state |= ANGULARGRAPH;

   setViewClass(SHOWVGATOP);

   // Testing: 
   // write("dummy.graph");
}

ShapeMap &MetaGraph::createNewShapeMap(depthmapX::ImportType mapType, std::string name) {

    switch(mapType) {
        case depthmapX::ImportType::DRAWINGMAP: {
            SuperSpacePixel::tail().push_back(ShapeMap(name));
            return SuperSpacePixel::tail().tail();
        }
        case depthmapX::ImportType::DATAMAP: {
            m_dataMaps.emplace_back(name,ShapeMap::DATAMAP);
            return m_dataMaps.back();
        }
    }
}

void MetaGraph::deleteShapeMap(depthmapX::ImportType mapType, ShapeMap &shapeMap) {

    switch(mapType) {
        case depthmapX::ImportType::DRAWINGMAP: {
            // go through the files to find if the layer is in one of them
            // if it is, remove it and if the remaining file is empty then
            // remove that too
            for(size_t j = 0; j < SuperSpacePixel::size(); j++) {
                int mapToRemove = -1;
                for(size_t i = 0; i < SuperSpacePixel::at(j).size(); i++) {
                    if(&SuperSpacePixel::at(j).at(i) == &shapeMap) {
                        mapToRemove = i;
                        break;
                    }
                }
                if(mapToRemove != -1) {
                    SuperSpacePixel::at(j).remove_at(mapToRemove);
                    if(SuperSpacePixel::at(j).size() == 0) {
                        SuperSpacePixel::remove_at(j);
                    }
                    break;
                }
            }
        }
        case depthmapX::ImportType::DATAMAP: {
            for(size_t i = 0; i < m_dataMaps.size(); i++) {
                if(&m_dataMaps[i] == &shapeMap) {
                    m_dataMaps.erase(m_dataMaps.begin() + i);
                    break;
                }
            }
        }
    }
}

void MetaGraph::updateParentRegions(ShapeMap &shapeMap) {
    if(SuperSpacePixel::tail().m_region.atZero()) {
        SuperSpacePixel::tail().m_region = shapeMap.getRegion();
    } else {
        SuperSpacePixel::tail().m_region = runion(SuperSpacePixel::tail().m_region, shapeMap.getRegion());
    }
    if(SuperSpacePixel::m_region.atZero()) {
        SuperSpacePixel::m_region = SuperSpacePixel::tail().m_region;
    } else {
        SuperSpacePixel::m_region = runion(SuperSpacePixel::m_region, SuperSpacePixel::tail().m_region);
    }
}

int MetaGraph::importLinesAsShapeMap(const std::vector<Line> &lines,
                                     QtRegion region,
                                     std::string name,
                                     depthmapX::Table &data )
{
   int oldstate = m_state;

   m_state &= ~DATAMAPS;

   m_dataMaps.emplace_back(name,ShapeMap::DATAMAP);
   int x = m_dataMaps.size() - 1;

   getDisplayedDataMap().init(lines.size(), region);
   if (!getDisplayedDataMap().importLines( lines, data )) {
      removeDataMap(x);
      m_state = oldstate;
      return -1;
   }

   m_state |= DATAMAPS;
   setViewClass(SHOWSHAPETOP);

   return x;
}

int MetaGraph::importPointsAsShapeMap(const std::vector<Point2f> &points,
                                      QtRegion region,
                                      std::string name,
                                      depthmapX::Table &data )
{
   int oldstate = m_state;

   m_state &= ~DATAMAPS;

   m_dataMaps.emplace_back(name,ShapeMap::DATAMAP);
   int x = m_dataMaps.size() - 1;

   getDisplayedDataMap().init(points.size(), region);
   if (!getDisplayedDataMap().importPoints( points, data )) {
      removeDataMap(x);
      m_state = oldstate;
      return -1;
   }

   m_state |= DATAMAPS;
   setViewClass(SHOWSHAPETOP);

   return x;
}

// the tidy(ish) version: still needs to be at top level and switch between layers

bool MetaGraph::pushValuesToLayer(int desttype, int destlayer, int push_func, bool count_col)
{
   int sourcetype = m_view_class;
   int sourcelayer = getDisplayedMapRef();
   int col_in = getDisplayedAttribute(); 
   // no col_out specified
   int col_out = -2;

   // temporarily turn off everything to prevent redraw during sensitive time:
   int oldstate = m_state;
   m_state &= ~(DATAMAPS | AXIALLINES | POINTMAPS);

   bool retvar = pushValuesToLayer(sourcetype,sourcelayer,desttype,destlayer,col_in,col_out,push_func,count_col);

   m_state = oldstate;

   return retvar;
}

// helper

void pushValue(double& val, int& count, double thisval, int push_func)
{
   if (thisval != -1) {
      switch (push_func) {
      case MetaGraph::PUSH_FUNC_MAX:
         if (val == -1 || thisval > val)
            val = thisval;
         break;
      case MetaGraph::PUSH_FUNC_MIN:
         if (val == -1 || thisval < val)
            val = thisval;
         break;
      case MetaGraph::PUSH_FUNC_AVG:
      case MetaGraph::PUSH_FUNC_TOT:
         if (val == -1.0) 
            val = thisval;
         else 
            val += thisval;
         break;
      }
      count++;
   }
}

// the full ubercontrol version:

bool MetaGraph::pushValuesToLayer(int sourcetype, int sourcelayer, int desttype, int destlayer, int col_in, int col_out, int push_func, bool count_col)
{
   AttributeTable& table_in = getAttributeTable(sourcetype, sourcelayer);
   AttributeTable& table_out = getAttributeTable(desttype, destlayer);

   if (col_out == -2) {
      std::string name = table_in.getColumnName(col_in);
      if ((table_out.isValidColumn(name) && table_out.isColumnLocked(table_out.getColumnIndex(name))) || name == "Object Count") {
         name = std::string("Copied ") + name;
      }
      col_out = table_out.insertColumn(name);
   }

   int col_count = -1;
   if (count_col) {
      col_count = table_out.insertColumn("Object Count");
      if (col_count <= col_out) {
         col_out++;
      }
   }

   if (sourcetype & VIEWDATA) {
      pvecint gatelist;
      for (int i = 0; i < table_out.getRowCount(); i++) {
         if (!table_out.isVisible(i)) {
            continue;
         }
         gatelist.clear();
         if (desttype == VIEWVGA) {
            m_dataMaps[sourcelayer].pointInPolyList(m_pointMaps[destlayer].getPoint(table_out.getRowKey(i)).m_location,gatelist);
         }
         else if (desttype == VIEWAXIAL) {
            auto shapeMap = m_shape_graphs.getMap(destlayer).getAllShapes();
            m_dataMaps[sourcelayer].shapeInPolyList(shapeMap[table_out.getRowKey(i)],gatelist);
         }
         else if (desttype == VIEWDATA) {
            if (sourcelayer == destlayer) {
               // error: pushing to same map
               return false;
            }
            auto dataMap = m_dataMaps[destlayer].getAllShapes();
            m_dataMaps[sourcelayer].shapeInPolyList(dataMap[table_out.getRowKey(i)],gatelist);
         }
         double val = -1.0;
         int count = 0;
         for (size_t j = 0; j < gatelist.size(); j++) {
            if (table_in.isVisible(gatelist[j])) {
               double thisval = table_in.getValue(gatelist[j],col_in);
               pushValue(val,count,thisval,push_func);
            }
         }
         if (push_func == PUSH_FUNC_AVG && val != -1.0) {
            val /= double(count);
         }
         table_out.setValue(i,col_out,float(val));
         if (count_col) {
            table_out.setValue(i,col_count,float(count));
         }
      }
   }
   else {
      // prepare a temporary value table to store counts and values
      double *vals = new double [table_out.getRowCount()];
      int *counts = new int [table_out.getRowCount()];

      int i;
      for (i = 0; i < table_out.getRowCount(); i++) {
         counts[i] = 0; // count set to zero for all
         vals[i] = -1;
      }

      pvecint gatelist;
      if (sourcetype & VIEWVGA) {
         for (int i = 0; i < table_in.getRowCount(); i++) {
            if (!table_in.isVisible(i)) {
               continue;
            }
            gatelist.clear();
            if (desttype == VIEWDATA) {
               m_dataMaps[destlayer].pointInPolyList(m_pointMaps[sourcelayer].getPoint(table_in.getRowKey(i)).m_location,gatelist);
            }
            else if (desttype == VIEWAXIAL) {
               // note, "axial" could be convex map, and hence this would be a valid operation
               m_shape_graphs.getMap(destlayer).pointInPolyList(m_pointMaps[sourcelayer].getPoint(table_in.getRowKey(i)).m_location,gatelist);
            }
            double thisval = table_in.getValue(i,col_in);
            for (size_t j = 0; j < gatelist.size(); j++) {
               if (table_out.isVisible(gatelist[j])) {
                  double& val = vals[gatelist[j]];
                  int& count = counts[gatelist[j]];
                  pushValue(val,count,thisval,push_func);
               }
            }
         }
      }
      else if (sourcetype & VIEWAXIAL) {
         // note, in the spirit of mapping fewer objects in the gate list, it is *usually* best to 
         // perform axial -> gate map in this direction
         // however, "Axial" to VGA, likely to have more points than "axial" shapes, should probably be performed using the first 
         // algorithm
         for (int i = 0; i < table_in.getRowCount(); i++) {
            if (!table_in.isVisible(i)) {
               continue;
            }
            gatelist.clear();
            if (desttype == VIEWDATA) {
               auto dataMap = m_shape_graphs.getMap(sourcelayer).getAllShapes();
               m_dataMaps[destlayer].shapeInPolyList(dataMap[table_in.getRowKey(i)],gatelist);
            }
            else if (desttype == VIEWAXIAL) {
                auto shapeMap = m_shape_graphs.getMap(sourcelayer).getAllShapes();
               m_shape_graphs.getMap(destlayer).shapeInPolyList(shapeMap[table_in.getRowKey(i)],gatelist);
            }
            double thisval = table_in.getValue(i,col_in);
            for (size_t j = 0; j < gatelist.size(); j++) {
               if (table_out.isVisible(gatelist[j])) {
                  double& val = vals[gatelist[j]];
                  int& count = counts[gatelist[j]];
                  pushValue(val,count,thisval,push_func);
               }
            }
         }
      }

      for (i = 0; i < table_out.getRowCount(); i++) {
         if (!table_out.isVisible(i)) {
            continue;
         }
         if (push_func == PUSH_FUNC_AVG && vals[i] != -1.0) {
            vals[i] /= double(counts[i]);
         }
         table_out.setValue(i,col_out,float(vals[i]));
         if (count_col) {
            table_out.setValue(i,col_count,float(counts[i]));
         }
      }

      delete [] vals;
      delete [] counts;
   }

   // display new data in the relevant layer
   if (desttype == VIEWVGA) {
      m_pointMaps[destlayer].overrideDisplayedAttribute(-2);
      m_pointMaps[destlayer].setDisplayedAttribute(col_out);
   }
   else if (desttype == VIEWAXIAL) {
      m_shape_graphs.getMap(destlayer).overrideDisplayedAttribute(-2);
      m_shape_graphs.getMap(destlayer).setDisplayedAttribute(col_out);
   }
   else if (desttype == VIEWDATA) {
      m_dataMaps[destlayer].overrideDisplayedAttribute(-2);
      m_dataMaps[destlayer].setDisplayedAttribute(col_out);
   }


   return true;
}

// Agent functionality: some of it still kept here with the metagraph
// (to allow push value to layer and back again)

void MetaGraph::runAgentEngine(Communicator *comm)
{
   AttributeTable& table = getDisplayedPointMap().getAttributeTable();

   if (m_agent_engine.m_gatelayer != -1) {
      // switch the reference numbers from the gates layer to the vga layer
      int colgates = table.insertColumn(g_col_gate);
      pushValuesToLayer(VIEWDATA,m_agent_engine.m_gatelayer,
                        VIEWVGA,getDisplayedPointMapRef(),
                        -1,colgates,PUSH_FUNC_TOT);
      table.insertColumn(g_col_gate_counts);
   }

   m_agent_engine.run(comm, &(getDisplayedPointMap()) );

   if (m_agent_engine.m_gatelayer != -1) {
      // switch column counts from vga layer to gates layer...
      int colcounts = table.getColumnIndex(g_col_gate_counts);
      AttributeTable& tableout = m_dataMaps[m_agent_engine.m_gatelayer].getAttributeTable();
      int targetcol = tableout.insertColumn("Agent Counts");
      pushValuesToLayer(VIEWVGA,getDisplayedPointMapRef(),
                        VIEWDATA,m_agent_engine.m_gatelayer,
                        colcounts,targetcol,PUSH_FUNC_TOT);
      // and delete the temporary columns:
      table.removeColumn(colcounts);
      int colgates = table.getColumnIndex(g_col_gate);
      table.removeColumn(colgates);
   }
}

// Thru vision

bool MetaGraph::analyseThruVision(Communicator *comm, int gatelayer)
{
   bool retvar = false;

   AttributeTable& table = getDisplayedPointMap().getAttributeTable();

   // always have temporary gate counting layers -- makes it easier to code
   int colgates = table.insertColumn(g_col_gate);
   int colcounts = table.insertColumn(g_col_gate_counts);

   if (gatelayer != -1) {
      // switch the reference numbers from the gates layer to the vga layer
      pushValuesToLayer(VIEWDATA,gatelayer,
                        VIEWVGA,getDisplayedPointMapRef(),
                        -1,colgates,PUSH_FUNC_TOT);
   }

   try {
      retvar = getDisplayedPointMap().analyseThruVision(comm);
   }
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   // note after the analysis, the column order might have changed... retrieve:
   colgates = table.getColumnIndex(g_col_gate);
   colcounts = table.getColumnIndex(g_col_gate_counts);

   if (retvar && gatelayer != -1) {
      AttributeTable& tableout = m_dataMaps[gatelayer].getAttributeTable();
      int targetcol = tableout.insertColumn("Thru Vision Counts");
      pushValuesToLayer(VIEWVGA,getDisplayedPointMapRef(),
                        VIEWDATA,gatelayer,
                        colcounts,targetcol,PUSH_FUNC_TOT);
   }

   // and always delete the temporary columns:
   table.removeColumn(colcounts);
   table.removeColumn(colgates);

   return retvar;
}

///////////////////////////////////////////////////////////////////////////////////

int MetaGraph::getDisplayedMapRef() const
{
   int ref = -1;
   switch (m_view_class & VIEWFRONT) {
   case VIEWVGA:
      ref = getDisplayedPointMapRef();
      break;
   case VIEWAXIAL:
      ref = m_shape_graphs.getDisplayedMapRef();
      break;
   case VIEWDATA:
      ref = getDisplayedDataMapRef();
      break;
   }
   return ref;
}

// I'd like to use this more often so that several classes other than data maps and shape graphs 
// can be used in the future

int MetaGraph::getDisplayedMapType()
{
   int type = ShapeMap::EMPTYMAP;
   switch (m_view_class & VIEWFRONT) {
   case VIEWAXIAL:
      type = m_shape_graphs.getDisplayedMap().getMapType();
      break;
   case VIEWDATA:
      type = getDisplayedDataMap().getMapType();
      break;
   }
   return type;
}

// note: 0 is not at all editable, 1 is editable off and 2 is editable on
int MetaGraph::isEditable() const
{
   int editable = 0;
   switch (m_view_class & VIEWFRONT) {
   case VIEWVGA:
      if (getDisplayedPointMap().isProcessed()) {
         editable = NOT_EDITABLE;
      }
      else {
         editable = EDITABLE_ON;
      }
      break;
   case VIEWAXIAL:
      {
         int type = m_shape_graphs.getDisplayedMap().getMapType();
         if (type != ShapeMap::SEGMENTMAP && type != ShapeMap::ALLLINEMAP) {
            editable = m_shape_graphs.getDisplayedMap().isEditable() ? EDITABLE_ON : EDITABLE_OFF;
         }
         else {
            editable = NOT_EDITABLE;
         }
      }
      break;
   case VIEWDATA:
      editable = getDisplayedDataMap().isEditable() ? EDITABLE_ON : EDITABLE_OFF;
      break;
   }
   return editable;
}

bool MetaGraph::canUndo() const
{
   bool canundo = false;
   switch (m_view_class & VIEWFRONT) {
   case VIEWVGA:
      canundo = getDisplayedPointMap().canUndo();
      break;
   case VIEWAXIAL:
      canundo = m_shape_graphs.getDisplayedMap().canUndo();
      break;
   case VIEWDATA:
      canundo = getDisplayedDataMap().canUndo();
      break;
   }
   return canundo;
}

void MetaGraph::undo()
{
   switch (m_view_class & VIEWFRONT) {
   case VIEWVGA:
      getDisplayedPointMap().undoPoints();
      break;
   case VIEWAXIAL:
      m_shape_graphs.getDisplayedMap().undo();
      break;
   case VIEWDATA:
      getDisplayedDataMap().undo();
      break;
   }
}

// Moving to global ways of doing things:

int MetaGraph::addAttribute(const std::string& name)
{
   int col;
   switch (m_view_class & VIEWFRONT) {
   case VIEWVGA:
      col = getDisplayedPointMap().addAttribute(name);
      break;
   case VIEWAXIAL:
      col = m_shape_graphs.getDisplayedMap().addAttribute(name);
      break;
   case VIEWDATA:
      col = getDisplayedDataMap().addAttribute(name);
      break;
   }
   return col;
}

void MetaGraph::removeAttribute(int col)
{
   switch (m_view_class & VIEWFRONT) {
   case VIEWVGA:
      getDisplayedPointMap().removeAttribute(col);
      break;
   case VIEWAXIAL:
      m_shape_graphs.getDisplayedMap().removeAttribute(col);
      break;
   case VIEWDATA:
      getDisplayedDataMap().removeAttribute(col);
      break;
   }
}

bool MetaGraph::isAttributeLocked(int col)
{
   return getAttributeTable(m_view_class).isColumnLocked(col);
}

int MetaGraph::getDisplayedAttribute() const
{
   int col = -1;
   switch (m_view_class & VIEWFRONT) {
   case VIEWVGA:
      col = getDisplayedPointMap().getDisplayedAttribute();
      break;
   case VIEWAXIAL:
      col = m_shape_graphs.getDisplayedMap().getDisplayedAttribute();
      break;
   case VIEWDATA:
      col = getDisplayedDataMap().getDisplayedAttribute();
      break;
   }
   return col;
}

// this is coming from the front end, so force override:
void MetaGraph::setDisplayedAttribute(int col)
{
   switch (m_view_class & VIEWFRONT) {
   case VIEWVGA:
      getDisplayedPointMap().overrideDisplayedAttribute(-2);
      getDisplayedPointMap().setDisplayedAttribute(col);
      break;
   case VIEWAXIAL:
      m_shape_graphs.getDisplayedMap().overrideDisplayedAttribute(-2);
      m_shape_graphs.getDisplayedMap().setDisplayedAttribute(col);
      break;
   case VIEWDATA:
      getDisplayedDataMap().overrideDisplayedAttribute(-2);
      getDisplayedDataMap().setDisplayedAttribute(col);
      break;
   }
}

// const and non-const versions:

AttributeTable& MetaGraph::getAttributeTable(int type, int layer)
{
   AttributeTable *tab = NULL;
   if (type == -1) {
      type = m_view_class;
   }
   switch (type & VIEWFRONT) {
   case VIEWVGA:
      tab = (layer == -1) ? &(getDisplayedPointMap().getAttributeTable()) : &(m_pointMaps[layer].getAttributeTable());
      break;
   case VIEWAXIAL:
      tab = (layer == -1) ? &(m_shape_graphs.getDisplayedMap().getAttributeTable()) : &(m_shape_graphs.getMap(layer).getAttributeTable());
      break;
   case VIEWDATA:
      tab = (layer == -1) ? &(getDisplayedDataMap().getAttributeTable()) : &(m_dataMaps[layer].getAttributeTable());
      break;
   }
   return *tab;
}

const AttributeTable& MetaGraph::getAttributeTable(int type, int layer) const
{
   const AttributeTable *tab = NULL;
   if (type == -1) {
      type = m_view_class & VIEWFRONT;
   }
   switch (type) {
   case VIEWVGA:
      tab = layer == -1 ? &(getDisplayedPointMap().getAttributeTable()) : &(m_pointMaps[layer].getAttributeTable());
      break;
   case VIEWAXIAL:
      tab = layer == -1 ? &(m_shape_graphs.getDisplayedMap().getAttributeTable()) : &(m_shape_graphs.getMap(layer).getAttributeTable());
      break;
   case VIEWDATA:
      tab = layer == -1 ? &(getDisplayedDataMap().getAttributeTable()) : &(m_dataMaps[layer].getAttributeTable());
      break;
   }
   return *tab;
}

int MetaGraph::readFromFile( const std::string& filename )
{

    if (filename.empty()) {
       return NOT_A_GRAPH;
    }

 #ifdef _WIN32
    ifstream stream( filename.c_str(), ios::binary | ios::in );
 #else
    ifstream stream( filename.c_str(), ios::in );
 #endif
    int result = readFromStream(stream, filename);
    stream.close();
    return result;
}

int MetaGraph::readFromStream( std::istream &stream, const std::string& filename )
{
   m_state = 0;   // <- clear the state out

   // clear BSP tree if it exists:
   if (m_bsp_root) {
      delete m_bsp_root;
      m_bsp_root = NULL;
   }
   m_bsp_tree = false;

   char header[3];
   stream.read( header, 3 );
   if (stream.fail() || header[0] != 'g' || header[1] != 'r' || header[2] != 'f') {
      return NOT_A_GRAPH;
   }
   int version;
   stream.read( (char *) &version, sizeof( version ) );
   m_file_version = version;  // <- recorded for easy debugging
   if (version > METAGRAPH_VERSION) {
      return NEWER_VERSION;
   }
   if (version < METAGRAPH_VERSION) {
       std::unique_ptr<mgraph440::MetaGraph> mgraph(new mgraph440::MetaGraph);
       auto result = mgraph->read(filename);
       if ( result != mgraph440::MetaGraph::OK)
       {
           return DAMAGED_FILE;
       }
       std::stringstream tempstream;
       mgraph->writeToStream(tempstream, METAGRAPH_VERSION, 0);

       return readFromStream(tempstream, filename);
   }

   // have to use temporary state here as redraw attempt may come too early:
   int temp_state = 0;
   stream.read( (char *) &temp_state, sizeof( temp_state ) );
   stream.read( (char *) &m_view_class, sizeof(m_view_class) );
   stream.read( (char *) &m_showgrid, sizeof(m_showgrid) );
   stream.read( (char *) &m_showtext, sizeof(m_showtext) );

   // type codes: x --- properties
   //             v --- virtual graph (from versions below 70)
   //             n --- ngraph format
   //             l --- layer data
   //             p --- point data
   //             d --- data summary layers

   bool conversion_required = false;

   char type;
   stream.read( &type, 1 );
   if (type == 'd') {
       // contains deprecated datalayers. Read through mgraph440 which will
       // convert them into shapemaps
       std::unique_ptr<mgraph440::MetaGraph> mgraph(new mgraph440::MetaGraph);
       auto result = mgraph->read(filename);
       if ( result != mgraph440::MetaGraph::OK)
       {
           return DAMAGED_FILE;
       }
       std::stringstream tempstream;
       mgraph->writeToStream(tempstream, METAGRAPH_VERSION, 0);

       return readFromStream(tempstream, filename);
   }
   if (type == 'x') {
      FileProperties::read(stream,version);
      if (stream.eof()) {
         // erk... this shouldn't happen
         return DAMAGED_FILE;
      }
      else if (!stream.eof()) {
         stream.read( &type, 1 );
      }
   }
   else {
      FileProperties::setProperties("<unknown>","<unknown>","<unknown>","<unknown>");
   }
   if (stream.eof()) {
       // file is still ok, just empty
       return OK;
   }
   if (type == 'v') {

      conversion_required = true;

      skipVirtualMem(stream,version);

      // and set our filename:
      // Graph::m_nodes.setFilename( filename );

      // and tell everyone it's been archived
      // temp_state |= GRAPHARCHIVED;

      if (stream.eof()) {
         // erk... this shouldn't happen
         return DAMAGED_FILE;
      }
      else if (!stream.eof()) {
         stream.read( &type, 1 );
      }
   }
   if (type == 'l') {
      try {
         SuperSpacePixel::read( stream, version );
         temp_state |= LINEDATA;
         if (!stream.eof()) {
            stream.read( &type, 1 );         
         }
      }
      catch (pexception) {
         // erk... this shouldn't happen
         return DAMAGED_FILE;
      }
   }
   if (type == 'p') {
      readPointMaps( stream, version );
      setSpacePixel( (SuperSpacePixel *) this );
      temp_state |= POINTMAPS;
      if (!stream.eof()) {
         stream.read( &type, 1 );         
      }
   }
   if (type == 'g') {
      // record on state of actual point map:
      m_pointMaps.back().m_processed = true;

      if (!stream.eof()) {
         stream.read( &type, 1 );         
      }
   }
   if (type == 'a') {
      temp_state |= ANGULARGRAPH;
      if (!stream.eof()) {
         stream.read( &type, 1 );
      }
   }
   if (type == 'x') {
      m_shape_graphs.read( stream, version );
      temp_state |= SHAPEGRAPHS;
      if (!stream.eof()) {
         stream.read( &type, 1 );         
      }
   }
   if (type == 's') {
      readDataMaps(stream, version );
      temp_state |= DATAMAPS;
      if (!stream.eof()) {
         stream.read( &type, 1 );         
      }
   }
   m_state = temp_state;

   return OK;
}

int MetaGraph::write( const std::string& filename, int version, bool currentlayer )
{
   ofstream stream;

   int oldstate = m_state;
   m_state = 0;   // <- temporarily clear out state, avoids any potential read / write errors

   char type;

   // As of MetaGraph version 70 the disk caching has been removed
   stream.open( filename.c_str(), ios::binary | ios::out | ios::trunc );
   if (stream.fail()) {
      if (stream.rdbuf()->is_open()) {
         stream.close();
      }
      m_state = oldstate;
      return DISK_ERROR;  
   }
   stream.write("grf", 3);
   m_file_version = version; // <- note, the file may now have an updated file version
   stream.write( (char *) &version, sizeof(version) );
   if (currentlayer) {
      int tempstate, tempclass;
      if (m_view_class & VIEWVGA) {
         tempstate = POINTMAPS;
         tempclass = VIEWVGA;
      }
      else if (m_view_class & MetaGraph::VIEWAXIAL) {
         tempstate = SHAPEGRAPHS;
         tempclass = VIEWAXIAL;
      }
      else if (m_view_class & MetaGraph::VIEWDATA) {
         tempstate = DATAMAPS;
         tempclass = VIEWDATA;
      }
      stream.write( (char *) &tempstate, sizeof(tempstate) );
      stream.write( (char *) &tempclass, sizeof(tempclass) );
   }
   else {
      stream.write( (char *) &oldstate, sizeof(oldstate) );
      stream.write( (char *) &m_view_class, sizeof(m_view_class) );
   }
   stream.write( (char *) &m_showgrid, sizeof(m_showgrid) );
   stream.write( (char *) &m_showtext, sizeof(m_showtext) );

   type = 'x';
   stream.write(&type, 1);
   FileProperties::write(stream);

   if (currentlayer) {
      if (m_view_class & MetaGraph::VIEWVGA) {
         type = 'p';
         stream.write(&type, 1);
         writePointMaps( stream, version, true );
      }
      else if (m_view_class & MetaGraph::VIEWAXIAL) {
         type = 'x';
         stream.write(&type, 1);
         m_shape_graphs.write( stream, version, true );
      }
      else if (m_view_class & MetaGraph::VIEWDATA) {
         type = 's';
         stream.write(&type, 1);
         writeDataMaps( stream, version, true );
      }
   }
   else {
      if (oldstate & LINEDATA) {
         type = 'l';
         stream.write(&type, 1);
         SuperSpacePixel::write( stream, version );
      }
      if (oldstate & POINTMAPS) {
         type = 'p';
         stream.write(&type, 1);
         writePointMaps( stream, version );
      }
      if (oldstate & SHAPEGRAPHS) {
         type = 'x';
         stream.write(&type, 1);
         m_shape_graphs.write( stream, version );
      }
      if (oldstate & DATAMAPS) {
         type = 's';
         stream.write(&type, 1);
         writeDataMaps( stream, version );
      }
   }

   stream.close();

   m_state = oldstate;
   return OK;
}


streampos MetaGraph::skipVirtualMem(istream& stream, int version)
{
   // it's graph virtual memory: skip it
   int nodes = -1;
   stream.read( (char *) &nodes, sizeof(nodes) );

   nodes *= 2;

   for (int i = 0; i < nodes; i++) {
      int connections;
      stream.read( (char *) &connections, sizeof(connections) );
      // This relies on the pvecint storage... hope it don't change!
      stream.seekg( stream.tellg() + streamoff(connections * sizeof(connections)) );
   }
   return (stream.tellg());
}

std::vector<SimpleLine> MetaGraph::getVisibleDrawingLines() {

    std::vector<SimpleLine> lines;

    for (size_t i = 0; i < SuperSpacePixel::size(); i++) {
        for (size_t j = 0; j < SuperSpacePixel::at(i).size(); j++) {
            if (SuperSpacePixel::at(i).at(j).isShown()) {
                const std::vector<SimpleLine> &newLines = SuperSpacePixel::at(i).at(j).getAllShapesAsLines();
                lines.insert(std::end(lines), std::begin(newLines), std::end(newLines));
            }
        }
    }
    return lines;
}

int MetaGraph::addNewPointMap(const std::string& name)
{
   std::string myname = name;
   int counter = 1;
   bool duplicate = true;
   while (duplicate) {
      duplicate = false;
      for (auto& pointMap: m_pointMaps) {
         if (pointMap.getName() == myname) {
            duplicate = true;
            myname = dXstring::formatString(counter++,name + " %d");
            break;
         }
      }
   }
   m_pointMaps.push_back(PointMap(myname));
   m_pointMaps.back().setSpacePixel(m_spacepix);
   m_displayed_pointmap = m_pointMaps.size() - 1;
   return m_pointMaps.size() - 1;
}

bool MetaGraph::readPointMaps(istream& stream, int version)
{
   stream.read((char *) &m_displayed_pointmap, sizeof(m_displayed_pointmap));
   int count;
   stream.read((char *) &count, sizeof(count));
   for (int i = 0; i < count; i++) {
      m_pointMaps.push_back(PointMap());
      m_pointMaps.back().setSpacePixel( (SuperSpacePixel *) this );
      m_pointMaps.back().read( stream, version );
   }
   return true;
}

bool MetaGraph::writePointMaps(ofstream& stream, int version, bool displayedmaponly)
{
   if (!displayedmaponly) {
      stream.write((char *) &m_displayed_pointmap, sizeof(m_displayed_pointmap));
      int count = m_pointMaps.size();
      stream.write((char *) &count, sizeof(count));
      for (auto& pointmap: m_pointMaps) {
         pointmap.write( stream, version );
      }
   }
   else {
      int dummy;
      // displayed map is 0:
      dummy = 0;
      stream.write((char *) &dummy, sizeof(dummy));
      // count is 1
      dummy = 1;
      stream.write((char *) &dummy, sizeof(dummy));
      //
      m_pointMaps[m_displayed_pointmap].write(stream, version);
   }
   return true;
}

bool MetaGraph::readDataMaps(istream& stream, int version )
{
    m_dataMaps.clear(); // empty existing data
    // n.b. -- do not change to size_t as will cause 32-bit to 64-bit conversion problems
    unsigned int displayed_map;
    stream.read((char *)&displayed_map,sizeof(displayed_map));
    m_displayed_datamap = size_t(displayed_map);
    // read maps
    // n.b. -- do not change to size_t as will cause 32-bit to 64-bit conversion problems
    unsigned int count = 0;
    stream.read((char *) &count, sizeof(count));

    for (size_t j = 0; j < size_t(count); j++) {
        m_dataMaps.emplace_back();
        m_dataMaps.back().read(stream,version);
    }
    return true;
}

bool MetaGraph::writeDataMaps( ofstream& stream, int version, bool displayedmaponly )
{
   if (!displayedmaponly) {
      // n.b. -- do not change to size_t as will cause 32-bit to 64-bit conversion problems
      unsigned int displayed_map = (unsigned int)(m_displayed_datamap);
      stream.write((char *)&displayed_map,sizeof(displayed_map));
      // write maps
      // n.b. -- do not change to size_t as will cause 32-bit to 64-bit conversion problems
      unsigned int count = (unsigned int) m_dataMaps.size();
      stream.write((char *) &count, sizeof(count));
      for (size_t j = 0; j < count; j++) {
         m_dataMaps[j].write(stream,version);
      }
   }
   else {
      unsigned int dummy;
      // displayed map is 0
      dummy = 0;
      stream.write((char *)&dummy,sizeof(dummy));
      // count is 1
      dummy = 1;
      stream.write((char *)&dummy,sizeof(dummy));
      // write map:
      m_dataMaps[m_displayed_datamap].write(stream,version);
   }
   return true;
}
