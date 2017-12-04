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

#ifndef ems_h
#define ems_h

#include "ems_prims.hpp"
#include <tinyxml2.h>

namespace kicad_to_ems
{

class Configuration;

namespace ems
{


class PCB_EMS_Model
{
public:
    PCB_EMS_Model(srecs::charvec_t& Data, Configuration& Config);

    std::string GetModelScript();
    std::string GetMeshScript();

    void InjectOpenEMS_Script(const std::string& SourceFile);

private:
    std::vector<pems::Segment> m_Segments;
    std::vector<pems::Via> m_Vias;
    std::vector<pems::Zone> m_Polys;
    std::vector<pems::Line> m_PCB_OutlineElements;
    Configuration& m_Config;
    Configuration::SimulationBox_t& m_SimBox;
    Configuration::mesh_params_t& m_MeshParams;
    Configuration::conversion_settings_t& m_ConvSet;
    size_t m_MetalPriority;
    size_t m_PCBPriority;
    std::complex<double> m_AuxAxisOrigin;
    bool m_AuxAxisIsOrigin;

    bool m_RescueViaDrill;
    double m_LastViaDrill;

    bool GetSegment(srecs::SREC Srec);
    bool GetVia(srecs::SREC Srec);
    bool GetZone(srecs::SREC Srec);
    bool GetModule(srecs::SREC Srec);
    bool GetPCB(srecs::SREC Srec);
    bool GetPad(srecs::SREC Srec, double ModuleX, double ModuleY, double ModuleRot);

    void GenPCB_Polygon();

    void GenMaterialSection_XML(Configuration::MaterialProps& Material, tinyxml2::XMLElement* Node);

    pems::MeshLines GetOmptimalMesh();

    std::complex<double> MovePoint(std::complex<double> Point, bool Move);

    void FilterMesh(std::set<double>& Mesh, double MinGap);
    void SmoothMesh(std::set<double>& Mesh, double MaxGap);

    void MeshTransition(std::set<double>& Mesh, double MaxGap, double DLeft, double DRight, std::set<double>::iterator it0,
                        std::set<double>::iterator it1);
};





}
}

#endif // ems_h


