// Copyright (C) 2020 Petros Koutsolampros

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

#include "salalib/mgraph.h"
#include "salalib/pointdata.h"

#include "genlib/comm.h"

class IAnalysis {
  public:
    virtual std::string getAnalysisName() const = 0;
    virtual bool run(Communicator *comm) = 0;
    virtual ~IAnalysis() {}
};
