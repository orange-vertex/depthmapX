#pragma once

#include "salalib/axialmap.h"

class AllLineMap: public ShapeGraph
{
public:
    AllLineMap(std::string name): ShapeGraph(name, ShapeGraph::ALLLINEMAP) {}

    prefvec<PolyConnector> m_poly_connections;
    pqvector<RadialLine> m_radial_lines;

    bool write(std::ofstream& stream, int version) override {
        ShapeGraph::write(stream, version);
        m_poly_connections.write(stream);
        m_radial_lines.write(stream);
    }
};
