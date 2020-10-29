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

#pragma once

#include "genlib/p2dpoly.h"
#include "salalib/shapemap.h"
#include <deque>
#include <string>

class SpacePixelFile {
  protected:
    std::string m_name; // <- file name
    mutable int m_current_layer;

  public:
    std::deque<ShapeMap> m_spacePixels;
    QtRegion m_region; // easier public for now
    //
    SpacePixelFile(const std::string &name = std::string()) {
        m_name = name;
        m_current_layer = -1;
    }
    SpacePixelFile(SpacePixelFile &&other)
        : m_name(other.m_name), m_current_layer(other.m_current_layer), m_spacePixels(std::move(other.m_spacePixels)),
          m_region(std::move(other.m_region)) {}
    SpacePixelFile &operator=(SpacePixelFile &&other) {
        m_name = other.m_name;
        m_current_layer = other.m_current_layer;
        m_spacePixels = std::move(other.m_spacePixels);
        m_region = std::move(other.m_region);
        return *this;
    }
    SpacePixelFile(const SpacePixelFile &) = delete;
    SpacePixelFile &operator=(const SpacePixelFile &) = delete;

    void setName(const std::string &name) { m_name = name; }
    const std::string &getName() const { return m_name; }
    //
    QtRegion &getRegion() const { return (QtRegion &)m_region; }
    //
    // Screen functionality:
    void makeViewportShapes(const QtRegion &viewport = QtRegion()) const;
    bool findNextShape(bool &nextlayer) const;

    const SalaShape &getNextShape() const { return m_spacePixels[m_current_layer].getNextShape(); }

    // Is any one sublayer shown?

    bool isShown() const {
        for (size_t i = 0; i < m_spacePixels.size(); i++)
            if (m_spacePixels[i].isShown())
                return true;
        return false;
    }
    //
  public:
    bool read(std::istream &stream);
    bool write(std::ofstream &stream);
};
