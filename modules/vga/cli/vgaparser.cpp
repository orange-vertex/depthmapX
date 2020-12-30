// Copyright (C) 2017 Christian Sailer

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

#include "vgaparser.h"
#include "depthmapXcli/exceptions.h"
#include "depthmapXcli/parsingutils.h"
#include "depthmapXcli/simpletimer.h"
#include "depthmapXcli/runmethods.h"
#include "salalib/options.h"
#include <cstring>
#include <iostream>

using namespace depthmapX;


VgaParser::VgaParser() : m_vgaMode(VgaMode::NONE), m_localMeasures(false), m_globalMeasures(false)
{}

void VgaParser::parse(int argc, char *argv[])
{
    for ( int i = 1; i < argc;  )
    {

        if ( std::strcmp ("-vm", argv[i]) == 0)
        {
            if (m_vgaMode != VgaMode::NONE)
            {
                throw CommandLineException("-vm can only be used once, modes are mutually exclusive");
            }
            ENFORCE_ARGUMENT("-vm", i)
            if ( std::strcmp(argv[i], "isovist") == 0 )
            {
                m_vgaMode = VgaMode::ISOVIST;
            }
            else if ( std::strcmp(argv[i], "visibility") == 0 )
            {
                m_vgaMode = VgaMode::VISBILITY;
            }
            else if ( std::strcmp(argv[i], "metric") == 0 )
            {
                m_vgaMode = VgaMode::METRIC;
            }
            else if ( std::strcmp(argv[i], "angular") == 0 )
            {
                m_vgaMode = VgaMode::ANGULAR;
            }
            else if ( std::strcmp(argv[i], "thruvision") == 0)
            {
                m_vgaMode = VgaMode::THRU_VISION;
            }
            else
            {
                throw CommandLineException(std::string("Invalid VGA mode: ") + argv[i]);
            }
        }
        else if ( std::strcmp(argv[i], "-vg") == 0 )
        {
            m_globalMeasures = true;
        }
        else if ( std::strcmp(argv[i], "-vl") == 0 )
        {
            m_localMeasures = true;
        }
        else if (std::strcmp(argv[i], "-vr") == 0)
        {
            ENFORCE_ARGUMENT("-vr", i)
            m_radius = argv[i];
        }
        ++i;
    }

    if(m_vgaMode == VgaMode::NONE)
    {
        m_vgaMode = VgaMode::ISOVIST;
    }

    if (m_vgaMode == VgaMode::VISBILITY && m_globalMeasures)
    {
        if (m_radius.empty())
        {
            throw CommandLineException("Global measures in VGA/visibility analysis require a radius, use -vr <radius>");
        }
        if (m_radius != "n" && !has_only_digits(m_radius))
        {
            throw CommandLineException(std::string("Radius must be a positive integer number or n, got ") + m_radius);
        }

    }
    else if (m_vgaMode == VgaMode::METRIC)
    {
        if (m_radius.empty())
        {
            throw CommandLineException("Metric vga requires a radius, use -vr <radius>");
        }
    }
}

void VgaParser::run(const CommandLineParser &clp, IPerformanceSink &perfWriter) const
{
    RadiusConverter radiusConverter;

    auto mgraph = dm_runmethods::loadGraph(clp.getFileName().c_str(), perfWriter);

    std::unique_ptr<Options> options(new Options());

    std::cout << "Getting options..." << std::flush;
    switch(getVgaMode())
    {
        case VgaParser::VgaMode::VISBILITY:
            options->output_type = Options::OUTPUT_VISUAL;
            options->local = localMeasures();
            options->global = globalMeasures();
            if (options->global )
            {
                options->radius = radiusConverter.ConvertForVisibility(getRadius());
            }
            break;
        case VgaParser::VgaMode::METRIC:
            options->output_type = Options::OUTPUT_METRIC;
            options->radius = radiusConverter.ConvertForMetric(getRadius());
            break;
        case VgaParser::VgaMode::ANGULAR:
            options->output_type = Options::OUTPUT_ANGULAR;
            break;
        case VgaParser::VgaMode::ISOVIST:
            options->output_type = Options::OUTPUT_ISOVIST;
            break;
        case VgaParser::VgaMode::THRU_VISION:
            options->output_type = Options::OUTPUT_THRU_VISION;
            break;
        default:
            throw depthmapX::SetupCheckException("Unsupported VGA mode");
    }
    std::cout << " ok\nAnalysing graph..." << std::flush;

    DO_TIMED("Run VGA", mgraph->analyseGraph(dm_runmethods::getCommunicator(clp).get(), *options, clp.simpleMode() ))
    std::cout << " ok\nWriting out result..." << std::flush;
    DO_TIMED("Writing graph", mgraph->write(clp.getOuputFile().c_str(),METAGRAPH_VERSION, false))
    std::cout << " ok" << std::endl;
}
