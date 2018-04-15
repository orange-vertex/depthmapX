#pragma once

#include "salalib/pixelref.h"
#include "genlib/p2dpoly.h"

class PixelBase
{
protected:
   int m_rows;
   int m_cols;
   QtRegion m_region;
public:
   PixelBase() {;}
   // constrain is constrain to bounding box (i.e., in row / col bounds)
   virtual PixelRef pixelate(const Point2f&, bool constrain = true, int scalefactor = 1 ) const = 0;
   PixelRefVector pixelateLine( Line l, int scalefactor = 1 ) const;
   PixelRefVector pixelateLineTouching( Line l, double tolerance ) const;
   PixelRefVector quickPixelateLine(PixelRef p, PixelRef q);
   bool includes(const PixelRef pix) const {
      return (pix.x >= 0 && pix.x < m_cols && pix.y >= 0 && pix.y < m_rows);
   }
   int getCols() const {
      return m_cols;
   }
   int getRows() const {
      return m_rows;
   }
   const QtRegion& getRegion() const {
      return m_region;
   }
};
