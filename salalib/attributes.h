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


#ifndef __ATTRIBUTES_H__
#define __ATTRIBUTES_H__

#include "salalib/mgraph_consts.h"
#include "salalib/pafcolor.h"

#include "genlib/containerutils.h"

#include <string>
#include <unordered_map>

// yet another way to do attributes, but one that is easily expandable
// it's slow to look for a column, since you have to find the column
// by name, but other than that it's fairly easy

// helpers... local sorting routines

////////////////////////////////////////////////////////////////////////////////

struct ValuePair
{
   double value;  // needs to be double for sorting in index at higher resolution than the stored data
   int index;
   ValuePair(int i = -1, double v = -1.0)
   { index = i; value = v; }
   friend bool operator < (const ValuePair& vp1, const ValuePair& vp2);
   friend bool operator > (const ValuePair& vp1, const ValuePair& vp2);
   friend bool operator == (const ValuePair& vp1, const ValuePair& vp2);
};
inline bool operator < (const ValuePair& vp1, const ValuePair& vp2)
{
   return (vp1.value < vp2.value);
}
inline bool operator > (const ValuePair& vp1, const ValuePair& vp2)
{
   return (vp1.value > vp2.value);
}
inline bool operator == (const ValuePair& vp1, const ValuePair& vp2)
{
   return (vp1.value == vp2.value);
}
int compareValuePair(const void *p1, const void *p2);

////////////////////////////////////////////////////////////////////////////////

// These aren't really to do with attributes per se, but helpful to have
// them around ValuePair definition

// note! unsorted
struct IntPair
{
   int a;
   int b;
   IntPair(int x = -1, int y = -1) {
      a = x;
      b = y;
   }
   // inlined at end of file
   friend bool operator == (const IntPair& x, const IntPair& y);
   friend bool operator != (const IntPair& x, const IntPair& y);
   friend bool operator <  (const IntPair& x, const IntPair& y);
   friend bool operator >  (const IntPair& x, const IntPair& y);
};

// note! sorted
struct OrderedIntPair
{
   int a;
   int b;
   OrderedIntPair(int x = -1, int y = -1) {
      a = (int) x < y ? x : y;
      b = (int) x < y ? y : x;
   }
   // inlined at end of file
   friend bool operator == (const OrderedIntPair& x, const OrderedIntPair& y);
   friend bool operator != (const OrderedIntPair& x, const OrderedIntPair& y);
   friend bool operator <  (const OrderedIntPair& x, const OrderedIntPair& y);
   friend bool operator >  (const OrderedIntPair& x, const OrderedIntPair& y);
};

// note: these are unordered, but in 'a' takes priority over 'b'
inline bool operator == (const IntPair& x, const IntPair& y)
{
   return (x.a == y.a && x.b == y.b);
}
inline bool operator != (const IntPair& x, const IntPair& y)
{
   return (x.a != y.a || x.b != y.b);
}
inline bool operator < (const IntPair& x, const IntPair& y)
{
   return ( (x.a == y.a) ? x.b < y.b : x.a < y.a );
}
inline bool operator > (const IntPair& x, const IntPair& y)
{
   return ( (x.a == y.a) ? x.b > y.b : x.a > y.a );
}

// note: these are made with a is always less than b
inline bool operator == (const OrderedIntPair& x, const OrderedIntPair& y)
{
   return (x.a == y.a && x.b == y.b);
}
inline bool operator != (const OrderedIntPair& x, const OrderedIntPair& y)
{
   return (x.a != y.a || x.b != y.b);
}
inline bool operator < (const OrderedIntPair& x, const OrderedIntPair& y)
{
   return ( (x.a == y.a) ? x.b < y.b : x.a < y.a );
}
inline bool operator > (const OrderedIntPair& x, const OrderedIntPair& y)
{
   return ( (x.a == y.a) ? x.b > y.b : x.a > y.a );
}

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////

class AttributeRow
{
   friend class AttributeTable;
protected:
   mutable bool m_selected;
   mutable ValuePair m_display_info;
   // this is for recording layers (up to 64 are possible)
   long m_layers;
public:
   AttributeRow()
      { m_selected = false; m_layers = 1; }
   void init(size_t length);

   std::vector<float> cells;
};

class AttributeTable;

// note pvector: this is stored in order, reorder by qsort
class AttributeIndex
{
   friend class AttributeTable;
protected:
   int m_col;
public:
   AttributeIndex();
   void clear();
   int makeIndex(const AttributeTable& table, int col, bool setdisplayinfo);
   std::vector<ValuePair> valuePairs;
};

class AttributeColumn
{
public:
   std::string m_name;
   bool m_updated; // <- this flag is not saved, and indicates new data in the column (set on insert column etc)
   int m_physical_col;
   mutable double  m_min;
   mutable double  m_max;
   mutable double m_tot;
   mutable double m_visible_min;
   mutable double m_visible_max;
   mutable double m_visible_tot;
   bool m_hidden;
   bool m_locked;
   // display parameters
   DisplayParams m_display_params;
   // retain formula used to create column
   std::string m_formula;
public:
   AttributeColumn(const std::string& name = std::string(), int physical_col = -1)
   {   m_name = name;
       m_min = -1.0f;
       m_max = 0.0f;
       m_tot = 0.0;
       m_visible_min = -1.0f;
       m_visible_max = 0.0f;
       m_visible_tot = 0.0;
       m_physical_col = physical_col;
       m_hidden = false;
       m_locked = false;
       m_updated = false;
   }
   float makeNormValue(float value) const
   { return (m_min == m_max) ? 0.5f : (value == -1.0f) ? -1.0f : (float)((value - m_min) / (m_max - m_min)); }
   double getMinValue() const
   { return m_min; }
   double getMaxValue() const
   { return m_max; }
   double getTotValue() const
   { return m_tot; }
   double getVisibleMinValue() const
   { return m_visible_min; }
   double getVisibleMaxValue() const
   { return m_visible_max; }
   double getVisibleTotValue() const
   { return m_visible_tot; }
   //
   void setValue(float value)
   { m_updated = true; m_tot += value; if (m_min == -1.0f || value < m_min) m_min = value; if (value > m_max) m_max = value; }
   void changeValue(float oldval, float newval)
   { m_updated = true; if (oldval != -1.0f) m_tot -= oldval; if (newval != -1.0f) setValue(newval); }
   //
   // override (irritatingly needs to be done occassionally)
   void setInfo(double min, double max, double tot, double vismin, double vismax, double vistot) const
   { m_min = min; m_max = max; m_tot = tot; m_visible_min = vismin; m_visible_max = vismax; m_visible_tot = vistot; }
   //
   void setDisplayParams(const DisplayParams& dp)
   { m_display_params = dp; }
   const DisplayParams& getDisplayParams() const
   { return m_display_params; }
   //
   void reset();
   void rename(const std::string& name) {
      m_name = name;
   }
   // user-interface locking unlocking
   bool isLocked() const
   { return m_locked; }
   void setLock(bool lock = true)
   { m_locked = lock; }
   //
   bool read(std::istream &stream, int version );
   bool write( std::ofstream& stream, int version );
   friend bool operator == (const AttributeColumn& a, const AttributeColumn& b);
   friend bool operator < (const AttributeColumn& a, const AttributeColumn& b);
   friend bool operator > (const AttributeColumn& a, const AttributeColumn& b);
};
inline bool operator == (const AttributeColumn& a, const AttributeColumn& b)
{ return a.m_name == b.m_name; }
inline bool operator < (const AttributeColumn& a, const AttributeColumn& b)
{ return a.m_name < b.m_name; }
inline bool operator > (const AttributeColumn& a, const AttributeColumn& b)
{ return a.m_name > b.m_name; }

class AttributeTable
{
protected:
   std::unordered_map<int, AttributeRow> rows;
   std::vector<int> rowKeys;
   std::string m_name;
   std::vector<AttributeColumn> m_columns;
   std::vector<std::pair<int,AttributeRow> > m_data;
   // display parameters for the reference id column
   DisplayParams m_ref_display_params;
   //
   long m_available_layers;
   std::map<long,std::string> m_layers;
   mutable long m_visible_layers;
   mutable int m_visible_size;
   //
   std::string g_ref_number_name;    // = std::string("Ref Number");
   std::string g_ref_number_formula; // = std::string("");
   //
public:
   //
   AttributeTable(const std::string& name = std::string());
   //
   std::vector<AttributeColumn>::iterator insertColumn(const std::string& name = std::string());
   void removeColumn(int col);
   int renameColumn(int col, const std::string& name = std::string());
   int insertRow(int key);
   void removeRow(int key);
   void removeRowids(const std::vector<int>& list) {
       int count = 0;
       rows.erase(std::remove_if(rows.begin(), rows.end(), [&count,&list]() {
           return find(list.begin(), list.end(), count) != list.end();
       }));
   }

   // note... retrieves from column index (which are sorted by name), not physical column
   const std::string& getColumnName(int col) const
      { return col != -1 ? m_columns[col].m_name : g_ref_number_name; }
   int getColumnIndex(const std::string& name) const
      { auto iter = std::find(m_columns.begin(), m_columns.end(), name);
        return (iter == m_columns.end()) ? -1 : int(std::distance(m_columns.begin(), iter));} // note use -1 rather than paftl::npos for return value
   int getColumnCount() const
      { return (int) m_columns.size(); }
   int getOrInsertColumnIndex(const std::string& name) {
       return int(std::distance(m_columns.begin(), insertColumn(name)));
   }
   int getOrInsertLockedColumnIndex(const std::string& name) {
       auto iter = std::find(m_columns.begin(), m_columns.end(), name);
       if (iter == m_columns.end()) {
           iter = insertColumn(name);
           iter->setLock(true);
       }
       return int(std::distance(m_columns.begin(), iter));
   }
   bool isValidColumn(const std::string& name) const
      { return std::find(m_columns.begin(), m_columns.end(), name) == m_columns.end() || name == g_ref_number_name; }
   //
   int getRowKey(int index) const
      { return rowKeys[size_t(index)]; }
   int getRowid(const int key) const
      { auto iter = std::find(rowKeys.begin(), rowKeys.end(), key); return (iter == rowKeys.end()) ? -1 : int(std::distance(rowKeys.begin(), iter));} // note use -1 rather than paftl::npos for return value
   int getRowCount() const
      { return int(rowKeys.size()); }
   int getVisibleRowCount() const
      { return m_visible_size; }
   int getMaxRowKey() const
      { return rowKeys.back(); }

   float getValue(int key, const AttributeRow &row, int col) const
      { return col != -1 ? row.cells[size_t(m_columns[size_t(col)].m_physical_col)] : key; }
   // this version uses known row and col indices
   float getValue(int rowIdx, int col) const
      { int key = rowKeys[size_t(rowIdx)]; return getValue(key, rows.at(key), col); }
   // this version is meant to use row key and col name
   float getValue(int rowIdx, const std::string& name) const
      { return getValue(rowIdx, getColumnIndex(name)); }

   float getNormValue(int key, const AttributeRow &row, int col) const
      { return col != -1 ? m_columns[col].makeNormValue(row.cells.at(m_columns[col].m_physical_col)) : (float) (double(key)/double(rowKeys.back())); }
   float getNormValue(int rowIdx, int col) const
      { int key = rowKeys[size_t(rowIdx)]; return getNormValue(key, rows.at(rowKeys[size_t(rowIdx)]), col); }

   void setValue(AttributeRow &row, int col, float val)
      { row.cells[size_t(m_columns[size_t(col)].m_physical_col)] = val; m_columns[size_t(col)].setValue(val); }
   void setValue(int rowIdx, int col, float val)
      { setValue(rows.at(rowKeys[size_t(rowIdx)]), col, val); }
   void setValue(int rowIdx, const std::string& name, float val)
      { int col = getColumnIndex(name); if (col != -1) setValue(rowIdx,col,val); }

   void changeValue(AttributeRow &row, int col, float val)
      { float& theval = row.cells[size_t(m_columns[size_t(col)].m_physical_col)]; m_columns[col].changeValue(theval,val); theval = val; }
   void changeValue(int rowIdx, int col, float val)
      { changeValue(rows.at(rowKeys[size_t(rowIdx)]), col, val); }
   void changeValue(int rowIdx, const std::string& name, float val)
      { int col = getColumnIndex(name); if (col != -1) changeValue(rowIdx,col,val); }
   void changeSelValues(int col, float val)
      { for (auto& keyRow: rows) { if (keyRow.second.m_selected) changeValue(keyRow.second,col,val);} }

   void incrValue(AttributeRow &row, int col, float amount = 1.0f)
      { float& v = row.cells.at(m_columns[col].m_physical_col); v = (v == -1.0f) ? amount : v+amount ; m_columns[col].changeValue(v-amount,v); }
   void incrValue(int rowIdx, int col, float amount = 1.0f)
      { incrValue(rows.at(rowKeys[size_t(rowIdx)]), col, amount); }
   void incrValue(int rowIdx, const std::string& name, float amount = 1.0f)
      { int col = getColumnIndex(name);  if (col != -1) incrValue(rowIdx,col,amount); }


   void decrValue(AttributeRow &row, int col, float amount = 1.0f)
      { float& v = row.cells.at(m_columns[col].m_physical_col); v = (v != -1.0f) ? v-amount : -1.0f; m_columns[col].changeValue(v+amount,v); }
   void decrValue(int rowIdx, int col, float amount = 1.0f)
      { decrValue(rows.at(rowKeys[size_t(rowIdx)]), col, amount); }
   void decrValue(int rowIdx, const std::string& name, float amount = 1.0f)
      { int col = getColumnIndex(name);  if (col != -1) decrValue(rowIdx,col,amount); }

   void setColumnValue(int col, float val);
   double getMinValue(int col) const
      { return col != -1 ? m_columns[col].getMinValue() : rowKeys.front(); }
   double getMaxValue(int col) const
      { return col != -1 ? m_columns[col].getMaxValue() : rowKeys.back(); }
   double getAvgValue(int col) const
      { return col != -1 ? m_columns[col].getTotValue() / double(getRowCount()) : -1.0; }
   //
   double getVisibleMinValue(int col) const
      { return col != -1 ? m_columns[col].getVisibleMinValue() : rowKeys.front(); }
   double getVisibleMaxValue(int col) const
      { return col != -1 ? m_columns[col].getVisibleMaxValue() : rowKeys.back(); }
   double getVisibleAvgValue(int col) const
      { return col != -1 ? m_columns[col].getVisibleTotValue() / double(getVisibleRowCount()) : -1.0; }
   //
   void setColumnInfo(int col, double min, double max, double tot, double vismin, double vismax, double vistot) const
      { m_columns[col].setInfo(min,max,tot,vismin,vismax,vistot); }
   //
   const std::string& getColumnFormula(int col) const
      { return col != -1 ? m_columns[col].m_formula : g_ref_number_formula; }
   void setColumnFormula(int col, const std::string& formula)
      { m_columns[col].m_formula = formula; }
   //
   // user-interface locking unlocking
   bool isColumnLocked(int col) const
   { return (col == -1) ? true : m_columns[col].isLocked(); }
protected:
   // Selection:
   mutable int m_sel_count;
   mutable double m_sel_value;
public:
   bool selectRowByIndex(int index) const;
   bool selectRowByKey(int key) const;
   void deselectAll() const;
   void addSelValue(double value) const
   { m_sel_value += (value != -1.0f) ? value : 0.0; }
   double getSelAvg() const
   { return m_sel_value / m_sel_count; }
   bool isSelected(int index) const
   { return rows.at(rowKeys[size_t(index)]).m_selected; }
   // Display:
   mutable int m_display_column;
   mutable AttributeIndex m_display_index;
   mutable DisplayParams m_display_params;
   // Underlying layer control:
   void setVisibleLayers(long layers, bool override = false);
   const long getVisibleLayers() const
      { return m_visible_layers; }
   // More user friendly layer control:
   bool selectionToLayer(const std::string& name);
   const std::map<long,std::string>& getLayers() const
      { return m_layers; }
   bool isLayerVisible(int layerIdx) const
      { return ((depthmapX::getMapAtIndex(m_layers, layerIdx)->first & m_visible_layers) != 0); }
   void setLayerVisible(int layer, bool show);
   //
   bool isVisible(int rowIdx) const
      { return (m_visible_layers & (rows.at(rowKeys[size_t(rowIdx)]).m_layers)) != 0; }
   //
   void setDisplayColumn(int col, bool override = false) const;
   const int getDisplayColumn() const
      { return m_display_column; }
   const int getDisplayPos(int rowIdx) const
      { return rows.at(rowKeys[size_t(rowIdx)]).m_display_info.index; }
   const int getDisplayColor(int rowIdx) const
      { PafColor color; auto& row = rows.at(rowKeys[size_t(rowIdx)]); return row.m_selected ? PafColor(SALA_SELECTED_COLOR) : color.makeColor(row.m_display_info.value,m_display_params); }
   const int getDisplayColorByKey(int key) const
      { PafColor color; return color.makeColor(rows.at(key).m_display_info.value,m_display_params); }
   // this also doubles up to reset the selection total:
   void setDisplayInfo(int rowIdx, ValuePair vp) const
      { auto& row = rows.at(rowKeys[size_t(rowIdx)]); row.m_display_info = vp; if (row.m_selected) addSelValue((double)vp.value); }
   //
   // set display params for all attributes in table
   void setDisplayParams(const DisplayParams& dp);
   // set display params for a single column
   void setDisplayParams(int col, const DisplayParams& dp);
   const DisplayParams& getDisplayParams(int col) const
      { return m_display_params; }
   //
public:
   // misc
   void clear()  // <- totally destroy, not just clear values
   { m_columns.clear(); rowKeys.clear(); rows.clear(); }
   //
   void setName(const std::string& name)
   { m_name = name; }
   const std::string& getName() const
   { return m_name; }
   //
   // read / write
   bool read(std::istream &stream, int version );
   bool write( std::ofstream& stream, int version );
   //
   bool outputHeader( std::ostream& stream, char delim = '\t', bool updated_only = false ) const;
   bool outputRow( int row, std::ostream& stream, char delim = '\t', bool updated_only = false ) const;
   //
   bool exportTable(std::ostream& stream, bool updated_only);
   bool importTable(std::istream& stream, bool merge);
};

#endif
