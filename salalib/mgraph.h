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


#ifndef __MGRAPH_H__
#define __MGRAPH_H__

// Interface: the meta graph loads and holds all sorts of arbitrary data...

#include "mgraph_consts.h"

#include <genlib/paftl.h>
#include <genlib/p2dpoly.h>

#include <salalib/displayparams.h>
#include <salalib/fileproperties.h>
#include <salalib/spacepix.h>
#include <salalib/attributes.h>

#include <salalib/pointdata.h>
#include <salalib/connector.h>
#include <salalib/shapemap.h>
#include <salalib/axialmap.h>

#include <mutex>

// for agent engine interface
#include <salalib/nagent.h>
#include <salalib/importtypedefs.h>

#include <vector>
#include <memory>

///////////////////////////////////////////////////////////////////////////////////

class Communicator;

// A meta graph is precisely what it says it is

class MetaGraph : public SuperSpacePixel, public FileProperties
{
public:
   enum { ADD = 0x0001, REPLACE = 0x0002, CAT = 0x0010, DXF = 0x0020, NTF = 0x0040, RT1 = 0x0080, GML = 0x0100 };
   enum { NONE = 0x0000, POINTMAPS = 0x0002, LINEDATA = 0x0004,
          ANGULARGRAPH = 0x0010, DATAMAPS = 0x0020, AXIALLINES = 0x0040, SHAPEGRAPHS = 0x0100,
          BUGGY = 0x8000 };
   enum { NOT_EDITABLE = 0, EDITABLE_OFF = 1, EDITABLE_ON = 2 };
protected:
   int m_file_version;
   int m_state;
public:
   // some display options now set at file level
   bool m_showgrid;
   bool m_showtext;
   //
public:
   std::vector<ShapeMap> m_dataMaps;
   ShapeGraphs m_shape_graphs;
public:
   MetaGraph();
   ~MetaGraph();
   //
   int getVersion()
   {
      // note, if unsaved, m_file_version is -1
      return m_file_version;
   }


   std::vector<PointMap>& getPointMaps()
   { return m_pointMaps; }
   PointMap& getDisplayedPointMap()
   { return m_pointMaps[m_displayed_pointmap]; }
   const PointMap& getDisplayedPointMap() const
   { return m_pointMaps[m_displayed_pointmap]; }
   void setDisplayedPointMapRef(int i)
   { m_displayed_pointmap = i; }
   int getDisplayedPointMapRef() const
   { return m_displayed_pointmap; }
   void redoPointMapBlockLines()   // (flags blockedlines, but also flags that you need to rebuild a bsp tree if you have one)
   { for (auto& pointMap: m_pointMaps) { pointMap.m_blockedlines = false; } }
   int addNewPointMap(const std::string& name = std::string("VGA Map"));

private:
   std::vector<PointMap> m_pointMaps;
   int m_displayed_pointmap;
   SuperSpacePixel *m_spacepix;

   void setSpacePixel(SuperSpacePixel *spacepix)
   { m_spacepix = spacepix; for (auto& pointMap: m_pointMaps) pointMap.setSpacePixel(spacepix); }
   void removePointMap(int i)
   {
       if (m_displayed_pointmap >= i) m_displayed_pointmap--;
       if(m_displayed_pointmap < 0) m_displayed_pointmap = 0;
       m_pointMaps.erase(m_pointMaps.begin() + i);
   }

   bool readPointMaps(istream &stream, int version );
   bool writePointMaps( ofstream& stream, int version, bool displayedmaponly = false );

   std::recursive_mutex mLock;
public:
    std::unique_lock<std::recursive_mutex> getLock(){
       return std::unique_lock<recursive_mutex>(mLock);
   }

    std::unique_lock<std::recursive_mutex> getLockDeferred(){
        return std::unique_lock<std::recursive_mutex>(mLock, std::defer_lock_t());
    }

   int getState() const
      { return m_state; }
   // use with caution: only very rarely needed outside MetaGraph itself
   void setState(int state)
      { m_state = state; }

   int loadLineData( Communicator *communicator, int load_type );
   int loadCat( istream& stream, Communicator *communicator );
   int loadRT1(const std::vector<string>& fileset, Communicator *communicator);
   ShapeMap &createNewShapeMap(depthmapX::ImportType mapType, std::string name);
   void deleteShapeMap(depthmapX::ImportType mapType, ShapeMap &shapeMap);
   void updateParentRegions(ShapeMap &shapeMap);
   bool clearPoints();
   bool setGrid( double spacing, const Point2f& offset = Point2f() );                 // override of PointMap
   bool makePoints( const Point2f& p, int semifilled, Communicator *communicator = NULL);  // override of PointMap
   bool makeGraph( Communicator *communicator, int algorithm, double maxdist );
   bool analyseGraph(Communicator *communicator, Options options , bool simple_version); // <- options copied to keep thread safe
   //
   // helpers for editing maps
   bool isEditableMap();
   ShapeMap& getEditableMap();
   // currently only making / moving lines, but should be able to extend this to polys fairly easily:
   bool makeShape(const Line& line);
   bool moveSelShape(const Line& line);
   // onto polys as well:
   int polyBegin(const Line& line);
   bool polyAppend(int shape_ref, const Point2f& point);
   bool polyClose(int shape_ref);
   bool polyCancel(int shape_ref);
   //
   int addShapeGraph(const std::string& name, int type);
   int addShapeMap(const std::string& name);
   void removeDisplayedMap();
   //
   // various map conversions
   bool convertDrawingToAxial(Communicator *comm, std::string layer_name);  // n.b., name copied for thread safety
   bool convertDataToAxial(Communicator *comm, std::string layer_name, bool keeporiginal, bool pushvalues);
   bool convertDrawingToSegment(Communicator *comm, std::string layer_name);
   bool convertDataToSegment(Communicator *comm, std::string layer_name, bool keeporiginal, bool pushvalues);
   bool convertToData(Communicator *comm, std::string layer_name, bool keeporiginal, int typeflag);  // -1 signifies convert from drawing layer, else convert from data map
   bool convertToDrawing(Communicator *comm, std::string layer_name, int typeflag); // 0 signifies convert from data map, else convert from graph
   bool convertToConvex(Communicator *comm, std::string layer_name, bool keeporiginal, int typeflag); // -1 signifies convert from drawing layer, else convert from data map
   bool convertAxialToSegment(Communicator *comm, std::string layer_name, bool keeporiginal, bool pushvalues, double stubremoval);
   // note: not same categories
   bool convertPointsToShape();
   //bool convertBoundaryGraph( Communicator *communicator );

   int loadMifMap(Communicator *comm, istream& miffile, istream& midfile);
   bool makeAllLineMap( Communicator *communicator, const Point2f& seed );
   bool makeFewestLineMap( Communicator *communicator, int replace );
   bool analyseAxial( Communicator *communicator, Options options, bool simple_version ); // <- options copied to keep thread safe
   bool analyseSegments( Communicator *communicator, Options options ); // <- options copied to keep thread safe
   bool analyseTopoMet( Communicator *communicator, Options options ); // <- options copied to keep thread safe
   //
   bool hasAllLineMap()
   { return m_shape_graphs.hasAllLineMap(); }
   //
   enum { PUSH_FUNC_MAX = 0, PUSH_FUNC_MIN = 1, PUSH_FUNC_AVG = 2, PUSH_FUNC_TOT = 3};
   bool pushValuesToLayer(int desttype, int destlayer, int push_func, bool count_col = false);
   bool pushValuesToLayer(int sourcetype, int sourcelayer, int desttype, int destlayer, int col_in, int col_out, int push_func, bool count_col = false);
   //
   int getDisplayedMapRef() const;
   //
   // NB -- returns 0 (not editable), 1 (editable off) or 2 (editable on)
   int isEditable() const;
   bool canUndo() const;
   void undo();
   //
   ShapeGraph& getDisplayedShapeGraph()
   { return m_shape_graphs.getDisplayedMap(); }
   size_t m_displayed_datamap = -1;
   ShapeMap& getDisplayedDataMap()
   { return m_dataMaps[m_displayed_datamap]; }
   const ShapeMap& getDisplayedDataMap() const
   { return m_dataMaps[m_displayed_datamap]; }
   size_t getDisplayedDataMapRef() const
   { return m_displayed_datamap; }

   void removeDataMap(int i)
   { if (m_displayed_datamap >= i) m_displayed_datamap--; m_dataMaps.erase(m_dataMaps.begin() + i); }

   void setDisplayedDataMapRef(size_t map)
   {
      if (m_displayed_datamap != -1 && m_displayed_datamap != map)
         m_dataMaps[m_displayed_datamap].clearSel();
      m_displayed_datamap = map;
   }

   template <class T>
   size_t getMapRef(std::vector<T>& maps, const std::string& name) const
   {
      // note, only finds first map with this name
      for (size_t i = 0; i < maps.size(); i++) {
         if (maps[i].getName() == name)
            return i;
      }
      return -1;
   }

   ShapeGraphs& getShapeGraphs()
   { return m_shape_graphs; }
   std::vector<ShapeMap>& getDataMaps()
   { return m_dataMaps; }

   bool readDataMaps(istream &stream, int version );
   bool writeDataMaps( ofstream& stream, int version, bool displayedmaponly = false );

   void writeDrawingLayersAsDXF(std::ostream& stream);
   //
   int getDisplayedMapType();
   //
   QtRegion getBoundingBox() const;
   //
   int getDisplayedAttribute() const;
   void setDisplayedAttribute(int col);
   int addAttribute(const std::string& name);
   void removeAttribute(int col);
   bool isAttributeLocked(int col);
   AttributeTable& getAttributeTable(int type = -1, int layer = -1);
   const AttributeTable& getAttributeTable(int type = -1, int layer = -1) const;

   int getLineFileCount() const
      { return (int) m_spacePixels.size(); }

   // Quick mod - TV
   const std::string& getLineFileName(int file) const
      { return m_spacePixels[file].getName(); }
   int getLineLayerCount(int file) const
      { return (int) m_spacePixels[file].m_spacePixels.size(); }

   ShapeMap& getLineLayer(int file, int layer)
      { return m_spacePixels[file].m_spacePixels[layer]; }
   const ShapeMap& getLineLayer(int file, int layer) const
      { return m_spacePixels[file].m_spacePixels[layer]; }
   //
   // Some error handling -- the idea is that you catch the error in MetaGraph,
   // return a generic error code and then get your front end to interrogate the 
   // last error (pretty much as per standard Unix etc).
   // May have problems with multithreading.
public:
   class Error 
   {
   protected:
      std::string error;
   public:
      Error(const std::string& err = std::string()) { error = err; }
   };
protected:
   Error m_last_error;
public:
   Error& getLastError()
   { return m_last_error; }
   // for drawing either axial analysis or VGA
protected:
   int m_view_class;
public:
   enum { SHOWHIDEVGA = 0x0100, SHOWVGATOP = 0x0200, SHOWHIDEAXIAL = 0x0400, SHOWAXIALTOP = 0x0800, SHOWHIDESHAPE = 0x1000, SHOWSHAPETOP = 0x2000 };
   enum { VIEWNONE = 0x00, VIEWVGA = 0x01, VIEWBACKVGA = 0x02, VIEWAXIAL = 0x04, VIEWBACKAXIAL = 0x08,
          VIEWDATA = 0x20, VIEWBACKDATA = 0x40, VIEWFRONT = 0x25 };
   //
   int getViewClass()
   { return m_view_class; }
   // These functions make specifying conditions to do things much easier:
   bool viewingNone()
   { return (m_view_class == VIEWNONE); }
   bool viewingProcessed()
   { return ((m_view_class & (VIEWAXIAL | VIEWDATA)) || (m_view_class & VIEWVGA && getDisplayedPointMap().isProcessed())); }
   bool viewingShapes()
   { return (m_view_class & (VIEWAXIAL | VIEWDATA)) != 0; }
   bool viewingProcessedLines()
   { return ((m_view_class & VIEWAXIAL) == VIEWAXIAL); } 
   bool viewingProcessedShapes()
   { return ((m_view_class & VIEWDATA) == VIEWDATA); } 
   bool viewingProcessedPoints()
   { return ((m_view_class & VIEWVGA) && getDisplayedPointMap().isProcessed()); }
   bool viewingUnprocessedPoints()
   { return ((m_view_class & VIEWVGA) && !getDisplayedPointMap().isProcessed()); }
   //
   bool setViewClass(int command);
   //
   double getLocationValue(const Point2f& point);
   //
public:
   // these are dependent on what the view class is:
   bool isSelected()                              // does a selection exist
   {  if (m_view_class & VIEWVGA) 
         return getDisplayedPointMap().isSelected();
      else if (m_view_class & VIEWAXIAL) 
         return m_shape_graphs.getDisplayedMap().isSelected();
      else if (m_view_class & VIEWDATA) 
         return getDisplayedDataMap().isSelected();
      else 
         return false;
   }
   bool setCurSel( QtRegion& r, bool add = false )  // set current selection
   {  if (m_view_class & VIEWAXIAL) 
         return m_shape_graphs.getDisplayedMap().setCurSel(r, add);
      else if (m_view_class & VIEWDATA)
         return getDisplayedDataMap().setCurSel( r, add );
      else if (m_view_class & VIEWVGA)
         return getDisplayedPointMap().setCurSel( r, add );
      else if (m_state & POINTMAPS && !getDisplayedPointMap().isProcessed()) // this is a default select application
         return getDisplayedPointMap().setCurSel( r, add );
      else if (m_state & DATAMAPS) // I'm not sure why this is a possibility, but it appears you might have state & DATAMAPS without VIEWDATA...
         return getDisplayedDataMap().setCurSel( r, add );
      else 
         return false;
   }
   bool clearSel()
   {
      // really needs a separate clearSel for the datalayers... at the moment this is handled in PointMap
      if (m_view_class & VIEWVGA)
         return getDisplayedPointMap().clearSel();
      else if (m_view_class & VIEWAXIAL) 
         return m_shape_graphs.getDisplayedMap().clearSel();
      else if (m_view_class & VIEWDATA) 
         return getDisplayedDataMap().clearSel();
      else
         return false;
   }
   int getSelCount()
   {
      if (m_view_class & VIEWVGA)
         return getDisplayedPointMap().getSelCount();
      else if (m_view_class & VIEWAXIAL) 
         return (int) m_shape_graphs.getDisplayedMap().getSelCount();
      else if (m_view_class & VIEWDATA) 
         return (int) getDisplayedDataMap().getSelCount();
      else
         return 0;
   }
   float getSelAvg()
   {
      if (m_view_class & VIEWVGA)
         return (float)getDisplayedPointMap().getAttributeTable().getSelAvg();
      else if (m_view_class & VIEWAXIAL) 
         return (float)m_shape_graphs.getDisplayedMap().getAttributeTable().getSelAvg();
      else if (m_view_class & VIEWDATA) 
         return (float)getDisplayedDataMap().getAttributeTable().getSelAvg();
      else
         return -1.0f;
   }
   QtRegion getSelBounds()
   {
      if (m_view_class & VIEWVGA)
         return getDisplayedPointMap().getSelBounds();
      else if (m_view_class & VIEWAXIAL) 
         return m_shape_graphs.getDisplayedMap().getSelBounds();
      else if (m_view_class & VIEWDATA) 
         return getDisplayedDataMap().getSelBounds();
      else
         return QtRegion();
   }
   // setSelSet expects a set of ref ids:
   void setSelSet(const std::vector<int>& selset, bool add = false)
   { if (m_view_class & VIEWVGA && m_state & POINTMAPS)
         getDisplayedPointMap().setCurSel(selset,add);
      else if (m_view_class & VIEWAXIAL) 
         m_shape_graphs.getDisplayedMap().setCurSel(selset,add);
      else // if (m_view_class & VIEWDATA) 
         getDisplayedDataMap().setCurSel(selset,add); }
   std::set<int>& getSelSet()
   {  if (m_view_class & VIEWVGA && m_state & POINTMAPS)
         return getDisplayedPointMap().getSelSet();
      else if (m_view_class & VIEWAXIAL) 
         return m_shape_graphs.getDisplayedMap().getSelSet();
      else // if (m_view_class & VIEWDATA) 
         return getDisplayedDataMap().getSelSet(); }
   const std::set<int>& getSelSet() const
   {  if (m_view_class & VIEWVGA && m_state & POINTMAPS)
         return getDisplayedPointMap().getSelSet();
      else if (m_view_class & VIEWAXIAL) 
         return m_shape_graphs.getDisplayedMap().getSelSet();
      else // if (m_view_class & VIEWDATA) 
         return getDisplayedDataMap().getSelSet(); }
//
public:
   // no longer supported
   //int addLineDynamic(const Line& l);
   //bool removeLineDynamic(LineKey linekey);
   //
   // Agent engine interface:
protected:
   AgentEngine m_agent_engine;
public:
   AgentEngine& getAgentEngine()
   { return m_agent_engine; }
   void runAgentEngine(Communicator *comm);
   //
public:
   // thru vision
   bool analyseThruVision(Communicator *comm = NULL, int gatelayer = -1);
   // BSP tree for making isovists
protected:
   BSPNode *m_bsp_root;
   bool m_bsp_tree;
public:
   bool makeBSPtree(Communicator *communicator = NULL);
   void resetBSPtree() { m_bsp_tree = false; }
   // returns 0: fail, 1: made isovist, 2: made isovist and added new shapemap layer
   int makeIsovist(Communicator *communicator, const Point2f& p, double startangle = 0, double endangle = 0, bool simple_version = true);
   // returns 0: fail, 1: made isovist, 2: made isovist and added new shapemap layer
   int makeIsovistPath(Communicator *communicator, double fov_angle = 2.0 * M_PI, bool simple_version = true);
   bool makeIsovist(const Point2f& p, Isovist& iso);
protected:
   // properties
public:
   // a few read-write returns:
   enum { OK, WARN_BUGGY_VERSION, WARN_CONVERTED, NOT_A_GRAPH, DAMAGED_FILE, DISK_ERROR, NEWER_VERSION, DEPRECATED_VERSION };
   // likely to use communicator if too slow...
   int readFromFile( const std::string& filename );
   int readFromStream( istream &stream, const std::string& filename );
   int write( const std::string& filename, int version, bool currentlayer = false);
   //
   std::vector<SimpleLine> getVisibleDrawingLines();
protected:
   streampos skipVirtualMem(istream &stream, int version);
};

///////////////////////////////////////////////////////////////////////////////

#endif
