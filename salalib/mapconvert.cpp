
#include "salalib/mapconvert.h"

// convert line layers to an axial map

ShapeGraph& MapConvert::convertDrawingToAxial(Communicator *comm, const std::string& name, std::vector<SpacePixelFile> &drawingFiles)
{
   if (comm) {
      comm->CommPostMessage( Communicator::NUM_STEPS, 2 );
      comm->CommPostMessage( Communicator::CURRENT_STEP, 1 );
   }

   QtRegion region;
   std::map<int,Line> lines;  // map required for tidy lines, otherwise acts like vector
   std::map<int,int> layers;  // this is used to say which layer it originated from

   bool recordlayer = false;

   // add all visible layers to the set of polygon lines...
   int count = 0;
   for (const auto& pixelGroup: drawingFiles) {
      int j = 0;
      for (const auto& pixel: pixelGroup.m_spacePixels) {
         if (pixel.isShown()) {
            if (region.atZero()) {
               region = pixel.getRegion();
            }
            else {
               region = runion(region, pixel.getRegion());
            }
            std::vector<SimpleLine> newLines = pixel.getAllShapesAsLines();
            for (const auto& line: newLines) {
               lines.insert(std::make_pair(count, Line(line.start(), line.end())));
               layers.insert(std::make_pair(count,j));
               count ++;
            }
            pixel.setShow(false);
         }
         if (j > 0) {
            recordlayer = true;
         }
         j++;
      }
   }
   if (count == 0) {
      throw RuntimeException("No lines created");
   }

   // quick tidy removes very short and duplicate lines, but does not merge overlapping lines
   TidyLines tidier;
   tidier.quicktidy(lines, region);
   if (lines.size() == 0) {
      throw RuntimeException("No lines left after cleaning");
   }

   if (comm) {
      comm->CommPostMessage( Communicator::CURRENT_STEP, 2 );
   }

   // create map layer...
   // we can stop here for all line axial map!
   ShapeGraph usermap(name,ShapeMap::AXIALMAP);

   usermap.init(lines.size(),region);        // used to be double density
   std::map<int, float> layerAttributes;
   usermap.initialiseAttributesAxial();
   int layerCol = -1;
   if (recordlayer)   {
       layerCol = usermap.getAttributeTable().insertColumn("Drawing Layer");
   }
   for (auto & line: lines) {
      if (recordlayer)
      {
          layerAttributes[layerCol] = float(layers.find(line.first)->second);
      }
      usermap.makeLineShape(line.second, false, false, layerAttributes );
   }

   usermap.makeConnections();

   return usermap;
}

// create axial map directly from data maps
// note that actually should be able to merge this code with the line layers, now both use similar code

int MapConvert::convertDataToAxial(Communicator *comm, const std::string& name, ShapeMap& shapemap, bool copydata)
{
   if (comm) {
      comm->CommPostMessage( Communicator::NUM_STEPS, 2 );
      comm->CommPostMessage( Communicator::CURRENT_STEP, 1 );
   }

   // add all visible layers to the set of polygon lines...

   std::map<int,Line> lines;
   std::map<int,int> keys;

   //m_region = shapemap.getRegion();
   QtRegion region = shapemap.getRegion();

   // add all visible layers to the set of polygon lines...

   int count = 0;
   for (auto shape: shapemap.getAllShapes()) {
      int key = shape.first;

      std::vector<Line> shapeLines = shape.second.getAsLines();
      for(Line line: shapeLines) {
         lines.insert(std::make_pair(count,line));
         keys.insert(std::make_pair(count,key));
         count++;
      }
   }
   if (lines.size() == 0) {
      return -1;
   }

   // quick tidy removes very short and duplicate lines, but does not merge overlapping lines
   TidyLines tidier;
   tidier.quicktidy(lines, region);
   if (lines.size() == 0) {
      return -1;
   }

   if (comm) {
      comm->CommPostMessage( Communicator::CURRENT_STEP, 2 );
   }

   // create map layer...
   int mapref = addMap(name,ShapeMap::AXIALMAP);
   // we can stop here for all line axial map!
   ShapeGraph& usermap = tail();

   usermap.init(lines.size(),region);  // used to be double density
   usermap.initialiseAttributesAxial();

   int dataMapShapeRefCol = usermap.getAttributeTable().insertColumn("Data Map Ref");

   std::map<int, float> extraAttr;
   std::vector<int> attrCols;
   if (copydata)   {
       AttributeTable& input = shapemap.getAttributeTable();
       AttributeTable& output = usermap.getAttributeTable();
       for (int i = 0; i < input.getColumnCount(); i++) {
          std::string colname = input.getColumnName(i);
          for (size_t k = 1; output.getColumnIndex(colname) != -1; k++){
             colname = dXstring::formatString((int)k,input.getColumnName(i) + " %d");
          }
          attrCols.push_back( output.insertColumn(colname));
       }
   }

    AttributeTable& input = shapemap.getAttributeTable();
    auto keyIter = keys.begin();
    for (auto& line: lines) {
        if (copydata){
            int rowid = input.getRowid(keyIter->second);
            for (int i = 0; i < input.getColumnCount(); ++i){
                extraAttr[attrCols[i]] = input.getValue(rowid,i);
            }
        }
        extraAttr[dataMapShapeRefCol] = keyIter->second;
        usermap.makeLineShape(line.second, false, false, extraAttr);
        ++keyIter;
    }

   // n.b. make connections also initialises attributes

   usermap.makeConnections();

   // if we are inheriting from a mapinfo map, pass on the coordsys and bounds:
   if (shapemap.hasMapInfoData()) {
      usermap.m_mapinfodata = MapInfoData();
      usermap.m_mapinfodata.m_coordsys = shapemap.getMapInfoData().m_coordsys;
      usermap.m_mapinfodata.m_bounds = shapemap.getMapInfoData().m_bounds;
      usermap.m_hasMapInfoData = true;
   }

   usermap.m_displayed_attribute = -2; // <- override if it's already showing
   usermap.setDisplayedAttribute( usermap.m_attributes.getColumnIndex("Connectivity") );

   // we can stop here!
   setDisplayedMapRef(mapref);

   return mapref;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

// yet more conversions, this time polygons to shape elements

int MapConvert::convertDrawingToConvex(Communicator *comm, const std::string& name, std::vector<SpacePixelFile> &drawingFiles)
{
   QtRegion region;
   pvecint polygon_refs;

   int mapref = addMap(name,ShapeMap::CONVEXMAP);
   ShapeGraph& usermap = tail();
   int conn_col = usermap.m_attributes.insertLockedColumn("Connectivity");

   size_t count = 0;
   size_t i = 0;

   for (const auto& pixelGroup: drawingFiles) {
      for (const auto& pixel: pixelGroup.m_spacePixels) {
         if (pixel.isShown()) {
             auto refShapes = pixel.getAllShapes();
             for (const auto& refShape: refShapes) {
               const SalaShape& shape = refShape.second;
               if (shape.isPolygon()) {
                  usermap.makeShape(shape);
                  usermap.m_connectors.push_back( Connector() );
                  usermap.m_attributes.setValue(count,conn_col,0);
                  count++;
               }
            }
         }
      }
   }
   if (count == 0) {
      removeMap(mapref);
      return -1;
   }

   for (const auto& pixelGroup: drawingFiles) {
      for (const auto& pixel: pixelGroup.m_spacePixels) {
         pixel.setShow(false);
      }
   }

   usermap.m_displayed_attribute = -2; // <- override if it's already showing
   usermap.setDisplayedAttribute( -1 );
   // we can stop here!
   setDisplayedMapRef(mapref);

   return mapref;
}

int MapConvert::convertDataToConvex(Communicator *comm, const std::string& name, ShapeMap& shapemap, bool copydata)
{
   pvecint polygon_refs;

   int mapref = addMap(name,ShapeMap::CONVEXMAP);
   ShapeGraph& usermap = getMap(mapref);
   int conn_col = usermap.m_attributes.insertLockedColumn("Connectivity");

   pvecint lookup;
   auto refShapes = shapemap.getAllShapes();
   std::map<int,float> extraAttr;
   std::vector<int> attrCols;
   AttributeTable& input = shapemap.getAttributeTable();
   if (copydata) {
      AttributeTable& output = usermap.getAttributeTable();
      for (int i = 0; i < input.getColumnCount(); i++) {
         std::string colname = input.getColumnName(i);
         for (int k = 1; output.getColumnIndex(colname) != -1; k++){
            colname = dXstring::formatString(k,input.getColumnName(i) + " %d");
         }
         attrCols.push_back(output.insertColumn(colname));
      }
   }

   int k = -1;
   for (auto refShape: refShapes) {
      k++;
      if ( copydata ){
          for ( int i = 0; i < input.getColumnCount(); ++i ){
              extraAttr[attrCols[i]] =input.getValue(k, i);
          }
      }
      SalaShape& shape = refShape.second;
      if (shape.isPolygon()) {
         int n = usermap.makeShape(shape, -1, extraAttr);
         usermap.m_connectors.push_back( Connector() );
         usermap.m_attributes.setValue(n,conn_col,0);
         lookup.push_back(k);
      }
   }
   if (lookup.size() == 0) {
      removeMap(mapref);
      return -1;
   }

   usermap.m_displayed_attribute = -2; // <- override if it's already showing
   usermap.setDisplayedAttribute( -1 );
   // we can stop here!
   setDisplayedMapRef(mapref);

   return mapref;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

// create segment map directly from line layers

int MapConvert::convertDrawingToSegment(Communicator *comm, const std::string& name, std::vector<SpacePixelFile> &drawingFiles)
{
   if (comm) {
      comm->CommPostMessage( Communicator::NUM_STEPS, 2 );
      comm->CommPostMessage( Communicator::CURRENT_STEP, 1 );
   }

   std::map<int,Line> lines;
   std::map<int,int> layers;  // this is used to say which layer it originated from
   bool recordlayer = false;

   QtRegion region;

   // add all visible layers to the set of polygon lines...
   int count = 0;
   for (const auto& pixelGroup: drawingFiles) {
       int j = 0;
      for (const auto& pixel: pixelGroup.m_spacePixels) {
         if (pixel.isShown()) {
            if (region.atZero()) {
               region = pixel.getRegion();
            }
            else {
               region = runion(region, pixel.getRegion());
            }
            std::vector<SimpleLine> newLines = pixel.getAllShapesAsLines();
            for (const auto& line: newLines) {
               lines.insert(std::make_pair(count, Line(line.start(), line.end())));
               layers.insert(std::make_pair(count,j));
               count++;
            }
            pixel.setShow(false);
         }
         if (j > 0) {
            recordlayer = true;
         }
         j++;
      }
   }
   if (count == 0) {
      return -1;
   }

   // quick tidy removes very short and duplicate lines, but does not merge overlapping lines
   TidyLines tidier;
   tidier.quicktidy(lines, region);
   if (lines.size() == 0) {
      return -1;
   }

   if (comm) {
      comm->CommPostMessage( Communicator::CURRENT_STEP, 2 );
   }

   // create map layer...
   int mapref = addMap(name,ShapeMap::SEGMENTMAP);
   // we can stop here for all line axial map!
   ShapeGraph& usermap = tail();

   usermap.init(lines.size(),region);
   std::map<int, float> layerAttributes;
   usermap.initialiseAttributesSegment();
   int layerCol = -1;
   if (recordlayer)   {
       layerCol = usermap.getAttributeTable().insertColumn("Drawing Layer");
   }
   for (auto & line: lines) {
      if (recordlayer)
      {
          layerAttributes[layerCol] = float(layers.find(line.first)->second);
      }
      usermap.makeLineShape(line.second, false, false, layerAttributes);
   }

   // make it!
   usermap.makeNewSegMap();

   // we can stop here!
   setDisplayedMapRef(mapref);

   return mapref;
}

// create segment map directly from data maps (ultimately, this will replace the line layers version)

int MapConvert::convertDataToSegment(Communicator *comm, const std::string& name, ShapeMap& shapemap, bool copydata)
{
   if (comm) {
      comm->CommPostMessage( Communicator::NUM_STEPS, 2 );
      comm->CommPostMessage( Communicator::CURRENT_STEP, 1 );
   }

   std::map<int,Line> lines;
   std::map<int,int> keys;

   // no longer requires m_region
   //m_region = shapemap.getRegion();
   QtRegion region = shapemap.getRegion();

   // add all visible layers to the set of polygon lines...

   int count = 0;
   for (auto shape: shapemap.getAllShapes()) {
      int key = shape.first;
      std::vector<Line> shapeLines = shape.second.getAsLines();
      for(Line line: shapeLines) {
         lines.insert(std::make_pair(count,line));
         keys.insert(std::make_pair(count,key));
         count++;
      }
   }
   if (lines.size() == 0) {
      return -1;
   }

   // quick tidy removes very short and duplicate lines, but does not merge overlapping lines
   TidyLines tidier;
   tidier.quicktidy(lines, region);

   if (lines.size() == 0) {
      return -1;
   }

   if (comm) {
      comm->CommPostMessage( Communicator::CURRENT_STEP, 2 );
   }

   // create map layer...
   int mapref = addMap(name,ShapeMap::SEGMENTMAP);

   // note, I may need to reuse this:
   ShapeGraph& usermap = tail();

   // if we are inheriting from a mapinfo map, pass on the coordsys and bounds:
   if (shapemap.hasMapInfoData()) {
      usermap.m_mapinfodata = MapInfoData();
      usermap.m_mapinfodata.m_coordsys = shapemap.getMapInfoData().m_coordsys;
      usermap.m_mapinfodata.m_bounds = shapemap.getMapInfoData().m_bounds;
      usermap.m_hasMapInfoData = true;
   }

   usermap.init(lines.size(),region);
   usermap.initialiseAttributesSegment();

   int dataMapShapeRefCol = usermap.getAttributeTable().insertColumn("Data Map Ref");

   std::map<int,float> extraAttr;
   std::vector<int> attrCols;
   AttributeTable& input = shapemap.getAttributeTable();
   if (copydata) {
      AttributeTable& output = usermap.getAttributeTable();
      for (int i = 0; i < input.getColumnCount(); i++) {
         std::string colname = input.getColumnName(i);
         for (int k = 1; output.getColumnIndex(colname) != -1; k++){
            colname = dXstring::formatString(k,input.getColumnName(i) + " %d");
         }
         attrCols.push_back(output.insertColumn(colname));
      }
   }

   auto keyIter = keys.begin();
   for (auto& line: lines) {
       if (copydata){
           int rowid = input.getRowid(keyIter->second);
           for (int i = 0; i < input.getColumnCount(); ++i){
               extraAttr[attrCols[i]] = input.getValue(rowid,i);
           }
       }
       extraAttr[dataMapShapeRefCol] = keyIter->second;
      usermap.makeLineShape(line.second, false, false, extraAttr);
      ++keyIter;
   }

   // start to be a little bit more efficient about memory now we are hitting the limits
   // from time to time:
   if (!copydata) {
      lines.clear();
   }

   // make it!
   usermap.makeNewSegMap();

   usermap.m_displayed_attribute = -2; // <- override if it's already showing
   usermap.setDisplayedAttribute( usermap.m_attributes.getColumnIndex("Connectivity") );

   // we can stop here!
   setDisplayedMapRef(mapref);

   return mapref;
}

// stubremoval is fraction of overhanging line length before axial "stub" is removed
int MapConvert::convertAxialToSegment(Communicator *comm, const std::string& name, bool keeporiginal, bool copydata, double stubremoval)
{
   if (m_displayed_map == -1) {
      return -1;
   }

   std::vector<Line> lines;
   prefvec<Connector> connectionset;

   ShapeGraph& dispmap = getDisplayedMap();
   dispmap.makeSegmentMap(lines, connectionset, stubremoval);

   // destroy unnecessary parts of axial map as quickly as possible in order not to overload memory
   if (!keeporiginal) {
      dispmap.m_shapes.clear();
      dispmap.m_connectors.clear();
   }

   // create map layer...
   int mapref = addMap(name,ShapeMap::SEGMENTMAP);
   ShapeGraph& segmap = getMap(mapref);

   segmap.init(lines.size(),dispmap.m_region);
   segmap.initialiseAttributesSegment();

   for (size_t k = 0; k < lines.size(); k++) {
      segmap.makeLineShape(lines[k]);
   }

   // clear data as soon as we do not need it:
   lines.clear();

   // if we are inheriting from a mapinfo map, pass on the coordsys and bounds:
   if (dispmap.m_hasMapInfoData) {
      segmap.m_mapinfodata = MapInfoData();
      segmap.m_mapinfodata.m_coordsys = dispmap.m_mapinfodata.m_coordsys;
      segmap.m_mapinfodata.m_bounds = dispmap.m_mapinfodata.m_bounds;
      segmap.m_hasMapInfoData = true;
   }


   segmap.makeSegmentConnections(connectionset);

   if (copydata) {
      segmap.pushAxialValues(dispmap);
   }
   // destroy unnecessary parts of axial map as quickly as possible in order not to overload memory
   if (!keeporiginal) {
      dispmap.m_attributes.clear();
   }


   segmap.m_displayed_attribute = -2; // <- override if it's already showing
   segmap.setDisplayedAttribute( segmap.m_attributes.getColumnIndex("Connectivity") );

   // we can stop here!
   setDisplayedMapRef(mapref);

   return mapref;
}
