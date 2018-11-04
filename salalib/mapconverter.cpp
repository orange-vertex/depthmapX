#include "salalib/mapconverter.h"
#include "salalib/tidylines.h"
#include "genlib/exceptions.h"

// convert line layers to an axial map

std::unique_ptr<ShapeGraph> MapConverter::convertDrawingToAxial(Communicator *comm, const std::string& name,
                                                                const std::vector<SpacePixelFile> &drawingFiles)
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
        // TODO: write a better error message
        throw depthmapX::RuntimeException("Failed to convert lines");
    }

    // quick tidy removes very short and duplicate lines, but does not merge overlapping lines
    TidyLines tidier;
    tidier.quicktidy(lines, region);
    if (lines.size() == 0) {
        throw depthmapX::RuntimeException("No lines found after removing short and duplicates");
    }

    if (comm) {
        comm->CommPostMessage( Communicator::CURRENT_STEP, 2 );
    }

    // create map layer...
    // we can stop here for all line axial map!
    std::unique_ptr<ShapeGraph> usermap(new ShapeGraph(name,ShapeMap::AXIALMAP));

    usermap->init(int(lines.size()),region);        // used to be double density
    std::map<int, float> layerAttributes;
    usermap->initialiseAttributesAxial();
    int layerCol = -1;
    if (recordlayer)   {
        layerCol = usermap->getAttributeTable().insertColumn("Drawing Layer");
    }
    for (auto & line: lines) {
        if (recordlayer)
        {
            layerAttributes[layerCol] = float(layers.find(line.first)->second);
        }
        usermap->makeLineShape(line.second, false, false, layerAttributes );
    }

    usermap->makeConnections();

    return usermap;
}

// create axial map directly from data maps
// note that actually should be able to merge this code with the line layers, now both use similar code

std::unique_ptr<ShapeGraph> MapConverter::convertDataToAxial(Communicator *comm, const std::string& name,
                                                             ShapeMap& shapemap, bool copydata)
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
       throw depthmapX::RuntimeException("No lines found in data map");
   }

   // quick tidy removes very short and duplicate lines, but does not merge overlapping lines
   TidyLines tidier;
   tidier.quicktidy(lines, region);
   if (lines.size() == 0) {
       throw depthmapX::RuntimeException("No lines found after removing short and duplicates");
   }

   if (comm) {
      comm->CommPostMessage( Communicator::CURRENT_STEP, 2 );
   }

   // create map layer...
   // we can stop here for all line axial map!
   std::unique_ptr<ShapeGraph> usermap(new ShapeGraph(name,ShapeMap::AXIALMAP));

   usermap->init(int(lines.size()),region);  // used to be double density
   usermap->initialiseAttributesAxial();

   int dataMapShapeRefCol = usermap->getAttributeTable().insertColumn("Data Map Ref");

   std::map<int, float> extraAttr;
   std::vector<int> attrCols;
   if (copydata)   {
       AttributeTable& input = shapemap.getAttributeTable();
       AttributeTable& output = usermap->getAttributeTable();
       for (int i = 0; i < input.getColumnCount(); i++) {
          std::string colname = input.getColumnName(i);
          for (size_t k = 1; output.getColumnIndex(colname) != -1; k++){
             colname = dXstring::formatString(int(k),input.getColumnName(i) + " %d");
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
                extraAttr[attrCols[size_t(i)]] = input.getValue(rowid,i);
            }
        }
        extraAttr[dataMapShapeRefCol] = keyIter->second;
        usermap->makeLineShape(line.second, false, false, extraAttr);
        ++keyIter;
    }

   // n.b. make connections also initialises attributes

   usermap->makeConnections();

   // if we are inheriting from a mapinfo map, pass on the coordsys and bounds:
   if (shapemap.hasMapInfoData()) {
      usermap->copyMapInfoBaseData(shapemap);
   }

   return usermap;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

// yet more conversions, this time polygons to shape elements

std::unique_ptr<ShapeGraph> MapConverter::convertDrawingToConvex(Communicator *comm, const std::string& name,
                                                                 const std::vector<SpacePixelFile> &drawingFiles)
{
   QtRegion region;
   pvecint polygon_refs;

   std::unique_ptr<ShapeGraph> usermap(new ShapeGraph(name,ShapeMap::CONVEXMAP));
   int conn_col = usermap->getAttributeTable().insertLockedColumn("Connectivity");

   size_t count = 0;

   for (const auto& pixelGroup: drawingFiles) {
      for (const auto& pixel: pixelGroup.m_spacePixels) {
         if (pixel.isShown()) {
             auto refShapes = pixel.getAllShapes();
             for (const auto& refShape: refShapes) {
               const SalaShape& shape = refShape.second;
               if (shape.isPolygon()) {
                  usermap->makeShape(shape);
                  usermap->getConnections().push_back( Connector() );
                  usermap->getAttributeTable().setValue(int(count),conn_col,0);
                  count++;
               }
            }
         }
      }
   }
   if (count == 0) {
       throw depthmapX::RuntimeException("No polygons found in drawing");
   }

   for (const auto& pixelGroup: drawingFiles) {
      for (const auto& pixel: pixelGroup.m_spacePixels) {
         pixel.setShow(false);
      }
   }

   return usermap;
}

std::unique_ptr<ShapeGraph> MapConverter::convertDataToConvex(Communicator *comm, const std::string& name,
                                                              ShapeMap& shapemap, bool copydata)
{
   pvecint polygon_refs;

   std::unique_ptr<ShapeGraph> usermap(new ShapeGraph(name,ShapeMap::CONVEXMAP));
   int conn_col = usermap->getAttributeTable().insertLockedColumn("Connectivity");

   pvecint lookup;
   auto refShapes = shapemap.getAllShapes();
   std::map<int,float> extraAttr;
   std::vector<int> attrCols;
   AttributeTable& input = shapemap.getAttributeTable();
   if (copydata) {
      AttributeTable& output = usermap->getAttributeTable();
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
              extraAttr[attrCols[size_t(i)]] =input.getValue(k, i);
          }
      }
      SalaShape& shape = refShape.second;
      if (shape.isPolygon()) {
         int n = usermap->makeShape(shape, -1, extraAttr);
         usermap->getConnections().push_back( Connector() );
         usermap->getAttributeTable().setValue(n,conn_col,0);
         lookup.push_back(k);
      }
   }
   if (lookup.size() == 0) {
       throw depthmapX::RuntimeException("No polygons found in data map");
   }

   return usermap;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

// create segment map directly from line layers

std::unique_ptr<ShapeGraph> MapConverter::convertDrawingToSegment(Communicator *comm, const std::string& name,
                                                                  const std::vector<SpacePixelFile> &drawingFiles)
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
       throw depthmapX::RuntimeException("No lines found in drawing");
   }

   // quick tidy removes very short and duplicate lines, but does not merge overlapping lines
   TidyLines tidier;
   tidier.quicktidy(lines, region);
   if (lines.size() == 0) {
       throw depthmapX::RuntimeException("No lines found after removing short and duplicates");
   }

   if (comm) {
      comm->CommPostMessage( Communicator::CURRENT_STEP, 2 );
   }

   // create map layer...
   // we can stop here for all line axial map!
   std::unique_ptr<ShapeGraph> usermap(new ShapeGraph(name,ShapeMap::SEGMENTMAP));

   usermap->init(int(lines.size()),region);
   std::map<int, float> layerAttributes;
   usermap->initialiseAttributesSegment();
   int layerCol = -1;
   if (recordlayer)   {
       layerCol = usermap->getAttributeTable().insertColumn("Drawing Layer");
   }
   for (auto & line: lines) {
      if (recordlayer)
      {
          layerAttributes[layerCol] = float(layers.find(line.first)->second);
      }
      usermap->makeLineShape(line.second, false, false, layerAttributes);
   }

   // make it!
   usermap->makeNewSegMap();

   return usermap;
}

// create segment map directly from data maps (ultimately, this will replace the line layers version)

std::unique_ptr<ShapeGraph> MapConverter::convertDataToSegment(Communicator *comm, const std::string& name,
                                                               ShapeMap& shapemap, bool copydata)
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
       throw depthmapX::RuntimeException("No lines found in data map");
   }

   // quick tidy removes very short and duplicate lines, but does not merge overlapping lines
   TidyLines tidier;
   tidier.quicktidy(lines, region);

   if (lines.size() == 0) {
       throw depthmapX::RuntimeException("No lines found after removing short and duplicates");
   }

   if (comm) {
      comm->CommPostMessage( Communicator::CURRENT_STEP, 2 );
   }

   // create map layer...
   // note, I may need to reuse this:
   std::unique_ptr<ShapeGraph> usermap(new ShapeGraph(name,ShapeMap::SEGMENTMAP));

   // if we are inheriting from a mapinfo map, pass on the coordsys and bounds:
   if (shapemap.hasMapInfoData()) {
      usermap->copyMapInfoBaseData(shapemap);
   }

   usermap->init(int(lines.size()),region);
   usermap->initialiseAttributesSegment();

   int dataMapShapeRefCol = usermap->getAttributeTable().insertColumn("Data Map Ref");

   std::map<int,float> extraAttr;
   std::vector<int> attrCols;
   AttributeTable& input = shapemap.getAttributeTable();
   if (copydata) {
      AttributeTable& output = usermap->getAttributeTable();
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
               extraAttr[attrCols[size_t(i)]] = input.getValue(rowid,i);
           }
       }
       extraAttr[dataMapShapeRefCol] = keyIter->second;
      usermap->makeLineShape(line.second, false, false, extraAttr);
      ++keyIter;
   }

   // start to be a little bit more efficient about memory now we are hitting the limits
   // from time to time:
   if (!copydata) {
      lines.clear();
   }

   // make it!
   usermap->makeNewSegMap();

   return usermap;
}

// stubremoval is fraction of overhanging line length before axial "stub" is removed
std::unique_ptr<ShapeGraph> MapConverter::convertAxialToSegment(Communicator *comm, ShapeGraph& axialMap,
                                                                const std::string& name, bool keeporiginal,
                                                                bool copydata, double stubremoval)
{
    std::vector<Line> lines;
    std::vector<Connector> connectionset;

    axialMap.makeSegmentMap(lines, connectionset, stubremoval);

    // destroy unnecessary parts of axial map as quickly as possible in order not to overload memory
    if (!keeporiginal) {
       axialMap.getAllShapes().clear();
       axialMap.getConnections().clear();
    }

    // create map layer...
    std::unique_ptr<ShapeGraph> segmap(new ShapeGraph(name,ShapeMap::SEGMENTMAP));

    segmap->init(int(lines.size()),axialMap.getRegion());
    segmap->initialiseAttributesSegment();

    for (size_t k = 0; k < lines.size(); k++) {
      segmap->makeLineShape(lines[k]);
    }

    // clear data as soon as we do not need it:
    lines.clear();

    // if we are inheriting from a mapinfo map, pass on the coordsys and bounds:
    if (axialMap.hasMapInfoData()) {
        segmap->copyMapInfoBaseData(axialMap);
    }


    segmap->makeSegmentConnections(connectionset);

    if (copydata) {
      segmap->pushAxialValues(axialMap);
    }
    // destroy unnecessary parts of axial map as quickly as possible in order not to overload memory
    if (!keeporiginal) {
      axialMap.getAttributeTable().clear();
    }

    return segmap;
}
