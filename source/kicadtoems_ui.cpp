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
#include <fstream>
#include <iterator>
#include "kicadtoems_ui.hpp"

using namespace kicad_to_ems;

std::vector<char> ReadFile(const char* filename);

KiCAD_to_openEMS::KiCAD_to_openEMS(Configuration& Config, const char* KiCAD_PCB_File)
    : m_Config(Config)
{
    // load pcb file
    std::vector<char> pcb_file = ReadFile(KiCAD_PCB_File);

    // generate models
    m_Model = new ems::PCB_EMS_Model(pcb_file, m_Config);
}
KiCAD_to_openEMS::~KiCAD_to_openEMS() { delete m_Model; }

void KiCAD_to_openEMS::WriteModel_Octave(const char* File)
{
    std::string model_script = GetModel_Octave();

    std::ofstream ofile(File, std::ios::out);
    if (ofile.is_open())
    {
        ofile.write(model_script.c_str(), model_script.size());
    }
}

std::string KiCAD_to_openEMS::GetModel_Octave() { return m_Model->GetModelScript(); }

void KiCAD_to_openEMS::WriteMesh_Octave(const char* File)
{
    std::string mesh_script = GetMesh_Octave();

    std::ofstream ofile(File, std::ios::out);
    if (ofile.is_open())
    {
        ofile.write(mesh_script.c_str(), mesh_script.size());
    }
}

std::string KiCAD_to_openEMS::GetMesh_Octave() { return m_Model->GetMeshScript(); }

void KiCAD_to_openEMS::InjectModelData(const char* XML_Settings)
{
    std::string file_name(XML_Settings);
    m_Model->InjectOpenEMS_Script(file_name);
}

std::vector<char> ReadFile(const char* filename)
{
    // open the file:
    std::ifstream file(filename, std::ios::in);

    // Stop eating new lines in binary mode!!!
    file.unsetf(std::ios::skipws);

    // get its size:
    std::streampos fileSize;

    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // reserve capacity
    std::vector<char> vec;
    vec.reserve(fileSize);

    // read the data:
    vec.insert(vec.begin(), std::istream_iterator<char>(file), std::istream_iterator<char>());

    return vec;
}
