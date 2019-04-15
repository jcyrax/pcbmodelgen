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

#ifndef kicadtoems_ui_h
#define kicadtoems_ui_h

#include "kicadtoems_config.hpp"
#include "ems.hpp"

namespace kicad_to_ems
{

class KiCAD_to_openEMS
{
    Configuration m_Config;
    ems::PCB_EMS_Model* m_Model;

public:
    // Perform PCB conversion
    KiCAD_to_openEMS(Configuration& Config, const char* KiCAD_PCB_File);
    ~KiCAD_to_openEMS();

    // Save Octave script file with functions to generate model and mesh
    void WriteModel_Octave(const char* File);
    // Return Octave script file with functions to generate model and mesh
    std::string GetModel_Octave();

    void WriteMesh_Octave(const char* File);
    std::string GetMesh_Octave();

    void InjectModelData(const char* XML_Settings);
};

} // namespace kicad_to_ems

#endif // kicadtoems_ui_h
