/*
 * Copyright 2017 Jānis Skujenieks
 *
 * This file is part of pcbmodelgen.
 *
 * pcbmodelgen is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * pcbmodelgen is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with pcbmodelgen.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <vector>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>

#include <tclap/CmdLine.h>

#include "kicadtoems_ui.hpp"

#include "misc.hpp"

#define s_VERSION "0.2.0"

int main(int argc, char* argv[])
{
    bool grid_arg_set = false;
    bool model_arg_set = false;
    bool xml_arg_set = false;

    std::string config_file;
    std::string grid_file;
    std::string model_file;
    std::string xml_inject_file;
    std::string pcb_file;

    try
    {
        TCLAP::CmdLine cmd("KiCAD PCB file to openEMS converter", ' ', s_VERSION);

        TCLAP::ValueArg<std::string> config_arg(
            "c", "config", "(IN) JSON configuration file name.",
            true, "config.json", "string");
        TCLAP::ValueArg<std::string> grid_arg(
            "g", "grid", "(optional) (OUT) Model mesh grid output file name. Octave function file.",
            false, "mesh.m", "string");
        TCLAP::ValueArg<std::string> model_arg(
            "m", "model", "(optional) (OUT) Model data output file name. Octave function file.",
            false, "model.m", "string");
        TCLAP::ValueArg<std::string> xml_arg(
            "x", "xml", "(optional) (IN) openEMS xml settings file into which model data are injected.",
            false, "simulation.xml", "string");
        TCLAP::ValueArg<std::string> kicad_arg(
            "p", "pcb", "(IN) KiCAD PCB file to convert.",
            true, "pcb.kicad_pcb", "string");

        cmd.add(config_arg);
        cmd.add(grid_arg);
        cmd.add(model_arg);
        cmd.add(xml_arg);
        cmd.add(kicad_arg);
        cmd.parse(argc, argv);

        grid_arg_set = grid_arg.isSet();
        model_arg_set = model_arg.isSet();
        xml_arg_set = xml_arg.isSet();

        config_file = config_arg.getValue();
        grid_file = grid_arg.getValue();
        model_file = model_arg.getValue();
        xml_inject_file = xml_arg.getValue();
        pcb_file = kicad_arg.getValue();

    } catch (TCLAP::ArgException& e)
    {
        std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
        exit(-1);
    }

    // load or set configuration
    kicad_to_ems::Configuration conf;
    conf.LoadConfig(config_file.c_str());

    // convert PCB
    kicad_to_ems::KiCAD_to_openEMS converter(conf, pcb_file.c_str());

    // write output files
    if (grid_arg_set)
    {
        converter.WriteMesh_Octave(grid_file.c_str());
    }
    if (model_arg_set)
    {
        converter.WriteModel_Octave(model_file.c_str());
    }
    if (xml_arg_set)
    {
        if (conf.SimulationBox.SimBoxUsed)
        {
            converter.InjectModelData(xml_inject_file.c_str());
        }
        else
        {
            std::cerr << "Must include 'SimulationBox' parameters in JSON configuration file\n";
            exit(-1);
        }
    }

    return 0;
}
