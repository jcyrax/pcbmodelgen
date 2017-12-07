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

#include "ems.hpp"

#include <tinyxml2.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <sstream>
#include <cstring>
#include <algorithm>

#include "kicadtoems_config.hpp"



using namespace kicad_to_ems;
using namespace ems;
using namespace pems;
using namespace srecs;
using namespace tinyxml2;
using std::complex;

std::string printMeshSet(std::set<double>& MeshSet);


void PCB_EMS_Model::InjectOpenEMS_Script(const std::string& SourceFile)
{

    XMLDocument doc;
    doc.LoadFile( SourceFile.c_str() );
    XMLElement* root = doc.RootElement();

    // openEMS.ContinuousStructure.Properties   Material | Metal
    XMLElement* node;

    node = root->FirstChildElement("ContinuousStructure");
    if(node == nullptr) throw ems_exc("Bad XML structure. Can't inject model");
    node = node->FirstChildElement("Properties");
    if(node == nullptr) throw ems_exc("Bad XML structure. Can't inject model");

    // find and delete all material and metal nodes
    XMLElement* material;
    while((material = node->FirstChildElement("Material")))
    {
        node->DeleteChild(material);
    }
    while((material = node->FirstChildElement("Metal")))
    {
        node->DeleteChild(material);
    }

    GenMaterialSection_XML(m_SimBox.materials.pcb, node);
    GenMaterialSection_XML(m_SimBox.materials.metal_top, node);
    GenMaterialSection_XML(m_SimBox.materials.metal_bot, node);
    GenMaterialSection_XML(m_SimBox.materials.hole_fill, node);
    GenMaterialSection_XML(m_SimBox.box_fill.box_material, node);


    doc.SaveFile( SourceFile.c_str() );
}

void PCB_EMS_Model::GenMaterialSection_XML(Configuration::MaterialProps& Material, XMLElement* Node)
{
    XMLElement* material;

    if(Material.IsPEC)
    {
        material = Node->GetDocument()->NewElement("Metal");
        material->SetAttribute("Name", Material.Name.c_str());
    }
    else
    {
        material = Node->GetDocument()->NewElement("Material");
        material->SetAttribute("Name", Material.Name.c_str());
        XMLElement* mat_property = Node->GetDocument()->NewElement("Property");
        if(mat_property == nullptr) throw ems_exc("TinyXML2 Create NewElement() failed");
        mat_property->SetAttribute("Epsilon", Material.Epsilon);
        mat_property->SetAttribute("Mue", Material.Mue);
        mat_property->SetAttribute("Kappa", Material.Kappa);
        mat_property->SetAttribute("Sigma", Material.Sigma);
        mat_property->SetAttribute("Density", Material.Density);

        material->InsertEndChild(mat_property);
    }

    XMLElement* primitives = Node->GetDocument()->NewElement("Primitives");
    if(primitives == nullptr) throw ems_exc("TinyXML2 Create NewElement() failed");
    material->InsertEndChild(primitives);

    for(size_t i = 0; i < m_Segments.size(); i++)
    {
        m_Segments[i].GetXML_Primitive(primitives, Material.Name);
    }

    for(size_t i = 0; i < m_Vias.size(); i++)
    {
        m_Vias[i].GetXML_Primitive(primitives, Material.Name);
    }

    for(size_t i = 0; i < m_Polys.size(); i++)
    {
        m_Polys[i].GetXML_Primitive(primitives, Material.Name);
    }

    Node->InsertEndChild(material);
}

std::string PCB_EMS_Model::GetModelScript()
{
    std::string str = "function retval = kicad_pcb_model(CSX)\n";

    for(size_t i = 0; i < m_Segments.size(); i++)
    {
        str += m_Segments[i].GetCSX_Script();
    }

    for(size_t i = 0; i < m_Vias.size(); i++)
    {
        str += m_Vias[i].GetCSX_Script();
    }

    for(size_t i = 0; i < m_Polys.size(); i++)
    {
        str += m_Polys[i].GetCSX_Script();
    }

    str += "retval = CSX;\n";
    str += "endfunction\n";

    return str;
}

pems::MeshLines PCB_EMS_Model::GetOmptimalMesh()
{
    MeshLines mesh;

    if(m_SimBox.SimBoxUsed)
    {
        mesh.X.insert(m_SimBox.min.X);
        mesh.X.insert(m_SimBox.max.X);
        mesh.Y.insert(m_SimBox.min.Y);
        mesh.Y.insert(m_SimBox.max.Y);
        mesh.Z.insert(m_SimBox.min.Z);
        mesh.Z.insert(m_SimBox.max.Z);
    }

    // generate automatic mesh from PCB model
    if(m_MeshParams.automatic_mesh.insert_automatic_mesh)
    {
        // get model mesh lines
        for(size_t i = 0; i < m_Segments.size(); i++)
        {
            MeshLines el_mesh = m_Segments[i].GetMeshData();
            mesh.X.insert(el_mesh.X.begin(), el_mesh.X.end());
            mesh.Y.insert(el_mesh.Y.begin(), el_mesh.Y.end());
            mesh.Z.insert(el_mesh.Z.begin(), el_mesh.Z.end());
        }
        for(size_t i = 0; i < m_Vias.size(); i++)
        {
            MeshLines el_mesh = m_Vias[i].GetMeshData();
            mesh.X.insert(el_mesh.X.begin(), el_mesh.X.end());
            mesh.Y.insert(el_mesh.Y.begin(), el_mesh.Y.end());
            mesh.Z.insert(el_mesh.Z.begin(), el_mesh.Z.end());
        }

        for(size_t i = 0; i < m_Polys.size(); i++)
        {
            MeshLines el_mesh = m_Polys[i].GetMeshData();
            mesh.X.insert(el_mesh.X.begin(), el_mesh.X.end());
            mesh.Y.insert(el_mesh.Y.begin(), el_mesh.Y.end());
            mesh.Z.insert(el_mesh.Z.begin(), el_mesh.Z.end());
        }

        // insert Z axis mesh
        double line_interval = m_ConvSet.pcb_height / (m_MeshParams.automatic_mesh.pcb_z_lines + 1);
        for(size_t i = 0; i < m_MeshParams.automatic_mesh.pcb_z_lines; ++i)
        {
            mesh.Z.insert((i + 1) * line_interval);
        }

        // Remove lines that are to close
        if(m_MeshParams.automatic_mesh.remove_small_cells)
        {
            FilterMesh(mesh.X, m_MeshParams.automatic_mesh.min_cell_size.X);
            FilterMesh(mesh.Y, m_MeshParams.automatic_mesh.min_cell_size.Y);
            FilterMesh(mesh.Z, m_MeshParams.automatic_mesh.min_cell_size.Z);
        }

        //SmoothMesh
        if(m_MeshParams.automatic_mesh.smooth_mesh_lines)
        {
            SmoothMesh(mesh.X, m_MeshParams.automatic_mesh.max_cell_size.X);
            SmoothMesh(mesh.Y, m_MeshParams.automatic_mesh.max_cell_size.Y);
            SmoothMesh(mesh.Z, m_MeshParams.automatic_mesh.max_cell_size.Z);
        }
    }

    // insert additional manual mesh
    if(m_MeshParams.manual_mesh.insert_manual_mesh)
    {
        mesh.X.insert(m_MeshParams.manual_mesh.X.begin(), m_MeshParams.manual_mesh.X.end());
        mesh.Y.insert(m_MeshParams.manual_mesh.Y.begin(), m_MeshParams.manual_mesh.Y.end());
        mesh.Z.insert(m_MeshParams.manual_mesh.Z.begin(), m_MeshParams.manual_mesh.Z.end());
    }

    return mesh;
}

void PCB_EMS_Model::MeshTransition(std::set<double>& Mesh, double MaxGap, double DLeft, double DRight,
                                   std::set<double>::iterator it0,
                                   std::set<double>::iterator it1)
{
    double delta = *it1 - *it0;
    bool skip_second = false;

    double mesh_neighbor_mul = m_MeshParams.automatic_mesh.smth_neighbor_size_diff;
    mesh_neighbor_mul -= mesh_neighbor_mul * 0.0001;
    if(mesh_neighbor_mul < 1.0) mesh_neighbor_mul = 1.0;

    while(DLeft > DRight * mesh_neighbor_mul)
    {
        skip_second = true;
        double needed_size = mesh_neighbor_mul * DRight;
        if(needed_size > MaxGap) needed_size = MaxGap;
        double needed_space = needed_size * 2;

        double new_mesh_line;
        if(delta >= needed_space)
        {
            new_mesh_line = *it1 - needed_size;
            DRight = *it1 - new_mesh_line;
            it1 = Mesh.insert(new_mesh_line).first;
        }
        else
        {
            if(delta * mesh_neighbor_mul / 2 >= DRight)
            {
                new_mesh_line = *it0 + (delta / 2.0);
                DRight = *it1 - new_mesh_line;
                it1 = Mesh.insert(new_mesh_line).first;
            }
            return;
        }
        delta = *it1 - *it0;
    }
    while(!skip_second && DLeft < DRight / mesh_neighbor_mul)
    {
        double needed_size = mesh_neighbor_mul * DLeft;
        if(needed_size > MaxGap) needed_size = MaxGap;
        double needed_space = needed_size * 2;

        if(delta >= needed_space)
        {
            double new_mesh_line = *it0 + needed_size;
            DLeft = new_mesh_line - *it0;
            it0 = Mesh.insert(new_mesh_line).first;
        }
        else
        {
            if(delta * mesh_neighbor_mul / 2 >= DLeft)
            {
                double new_mesh_line = *it0 + (delta / 2.0);
                DLeft = new_mesh_line - *it0;
                it0 = Mesh.insert(new_mesh_line).first;
            }
            return;
        }
        delta = *it1 - *it0;
    }

    // symmetry
    while(1)
    {
        delta = *it1 - *it0;

        double needed_size_left = mesh_neighbor_mul * DLeft;
        double needed_size_right = mesh_neighbor_mul * DRight;
        if(needed_size_left > MaxGap) needed_size_left = MaxGap;
        if(needed_size_right > MaxGap) needed_size_right = MaxGap;

        double needed_space = (needed_size_left + needed_size_right) * 2;

        if(delta >= needed_space)
        {
            double new_mesh_line_left = *it0 + needed_size_left;
            double new_mesh_line_right = *it1 - needed_size_right;
            DLeft = new_mesh_line_left - *it0;
            DRight = *it1 - new_mesh_line_right;

            it0 = Mesh.insert(new_mesh_line_left).first;
            it1 = Mesh.insert(new_mesh_line_right).first;
        }
        else
        {
            double cell_size = (DLeft + DRight) / 2;
            size_t cell_count;

            if(cell_size > MaxGap)
            {
                cell_size = MaxGap;
            }

            cell_count = std::floor(delta / cell_size);
            cell_size = delta / cell_count;
            if(cell_size > MaxGap)
            {
                cell_size = MaxGap;
                cell_count = std::ceil(delta / cell_size);
                cell_size = delta / cell_count;
            }

            for(size_t i = 1; i < cell_count; i++)
            {
                Mesh.insert(*it0 + i * cell_size);
            }
            break;
        }
    }
}

void PCB_EMS_Model::SmoothMesh(std::set<double>& Mesh, double MaxGap)
{
    std::set<double>::iterator prev;

    double mesh_neighbor_mul = m_MeshParams.automatic_mesh.smth_neighbor_size_diff;
    if(mesh_neighbor_mul < 1.0) mesh_neighbor_mul = 1.0;
    // make size transitions smooth
    if(Mesh.size() >= 3)
    {
        std::set<double>::iterator it0 = Mesh.begin();
        std::set<double>::iterator it1 = it0;
        it1++;
        std::set<double>::iterator it2 = it1;
        it2++;

        while(it2 != Mesh.end())
        {
            double delta_left = *it1 - *it0;
            double delta_right = *it2 - *it1;
            if(delta_left > delta_right * mesh_neighbor_mul)
            {
                double previous_delta;
                std::set<double>::iterator prev = it0;
                prev--;
                if(it0 != Mesh.begin() && prev != Mesh.begin()) previous_delta = *it0 - *prev;
                else previous_delta = std::numeric_limits<double>::max();

                MeshTransition(Mesh, MaxGap, previous_delta, delta_right, it0, it1);
            }
            else if(delta_left * mesh_neighbor_mul < delta_right)
            {
                double next_delta;
                std::set<double>::iterator next = it2;
                next++;
                if(it2 != Mesh.end() && next != Mesh.end()) next_delta = *next - *it2;
                else next_delta = std::numeric_limits<double>::max();

                MeshTransition(Mesh, MaxGap, delta_left, next_delta, it1, it2);
            }
            it2++;
            it1 = it2;
            it1--;
            it0 = it1;
            it0--;
        }
    }
}

void PCB_EMS_Model::FilterMesh(std::set<double>& Mesh, double MinGap)
{
    std::set<double>::iterator cur;
    std::set<double>::iterator prev;
    std::set<double>::iterator next = Mesh.begin();

    for(size_t index = 0; next != Mesh.end(); ++index, ++next)
    {
        if(index < 2)
        {
            continue;
        }
        // setup iterators
        cur = next;
        cur--;
        prev = cur;
        prev--;

        if(*cur - *prev < MinGap)
        {
            double new_line = (*cur + *prev) / 2.0;
            Mesh.erase(cur);
            Mesh.erase(prev);
            Mesh.insert(new_line);
        }
    }
}

std::string PCB_EMS_Model::GetMeshScript()
{
    MeshLines mesh = GetOmptimalMesh();

    // generate mesh data script
    std::string str;
    std::vector<double> gaps[3];
    std::vector<double> ratios[3];

    double previous;

    str += "function retval = kicad_pcb_mesh()\n";
    str += "mesh.x = [ ";
    auto it = mesh.X.begin();
    while(it != mesh.X.end())
    {
        if(it == mesh.X.begin())
        {
            previous = *it;
        }
        else
        {
            gaps[0].push_back(*it - previous);
            previous = *it;
        }
        str += std::to_string(*it);
        str += " ";
        it++;
    }
    str += " ];\n";

    str += "mesh.y = [ ";
    it = mesh.Y.begin();
    while(it != mesh.Y.end())
    {
        if(it == mesh.Y.begin())
        {
            previous = *it;
        }
        else
        {
            gaps[1].push_back(*it - previous);
            previous = *it;
        }
        str += std::to_string(*it);
        str += " ";
        it++;
    }
    str += " ];\n";

    str += "mesh.z = [ ";
    it = mesh.Z.begin();
    while(it != mesh.Z.end())
    {
        if(it == mesh.Z.begin())
        {
            previous = *it;
        }
        else
        {
            gaps[2].push_back(*it - previous);
            previous = *it;
        }
        str += std::to_string(*it);
        str += " ";
        it++;
    }
    str += " ];\n";

    str += "retval = mesh;\n";
    str += "endfunction\n";

    str += "\n";
    str += "% Mesh gap sizes:\n";

    for(size_t i = 0; i < 3; ++i)
    {
        double max_gap, min_gap;
        switch(i)
        {
        case 0:
            str += "% X:\n% ";
            max_gap = m_MeshParams.automatic_mesh.max_cell_size.X;
            min_gap = m_MeshParams.automatic_mesh.min_cell_size.X;
            break;
        case 1:
            str += "% Y:\n% ";
            max_gap = m_MeshParams.automatic_mesh.max_cell_size.Y;
            min_gap = m_MeshParams.automatic_mesh.min_cell_size.Y;
            break;
        case 2:
            str += "% Z:\n% ";
            max_gap = m_MeshParams.automatic_mesh.max_cell_size.Z;
            min_gap = m_MeshParams.automatic_mesh.min_cell_size.Z;
            break;
        }
        std::vector<size_t> bad_gap_indices;
        for(size_t k = 0; k < gaps[i].size(); ++k)
        {
            if(k != 0 && k % 10 == 0)
            {
                for(size_t j = 0; j < bad_gap_indices.size(); ++j)
                {
                    if(j == 0)
                    {
                        str += "\tWARNING mesh gap size violate configuration at columns: ";
                    }
                    str += std::to_string(bad_gap_indices[j]) + " ";
                }
                str += "\n% ";
                bad_gap_indices.clear();
            }
            str += std::to_string(gaps[i][k]) + " ";
            if(gaps[i][k] > max_gap) bad_gap_indices.push_back(k % 10);
            if(gaps[i][k] < min_gap) bad_gap_indices.push_back(k % 10);
        }
        str += "\n";
    }

    str += "\n";
    str += "% Mesh gap ratios:\n";

    for(size_t i = 0; i < 3; ++i)
    {
        switch(i)
        {
        case 0:
            str += "% X:\n% ";
            break;
        case 1:
            str += "% Y:\n% ";
            break;
        case 2:
            str += "% Z:\n% ";
            break;
        }
        double configured_gap_ratio = m_MeshParams.automatic_mesh.smth_neighbor_size_diff;
        std::vector<size_t> bad_gap_indices;
        for(size_t k = 1; k < gaps[i].size(); ++k)
        {
            double gap_ratio;
            if(gaps[i][k - 1] < gaps[i][k])
            {
                gap_ratio = gaps[i][k] / gaps[i][k - 1];
            }
            else
            {
                gap_ratio = gaps[i][k - 1] / gaps[i][k];
            }

            str += std::to_string(gap_ratio) + " ";
            if(gap_ratio > configured_gap_ratio) bad_gap_indices.push_back((k - 1) % 10);

            if(k % 10 == 0)
            {
                for(size_t j = 0; j < bad_gap_indices.size(); ++j)
                {
                    if(j == 0)
                    {
                        str += "\tWARNING mesh gap ratio violate configuration at columns: ";
                    }
                    str += std::to_string(bad_gap_indices[j]) + " ";
                }
                str += "\n% ";
                bad_gap_indices.clear();
            }
        }
        str += "\n";
    }


    bool warning = false;

    // Verify mesh quality - print warnings if deviations
    for(size_t i = 0; i < 3; ++i)
    {
        std::set<double> mesh_axis;
        double max_gap, min_gap;
        switch(i)
        {
        case 0:
            mesh_axis = mesh.X;
            max_gap = m_MeshParams.automatic_mesh.max_cell_size.X;
            min_gap = m_MeshParams.automatic_mesh.min_cell_size.X;
            break;
        case 1:
            mesh_axis = mesh.Y;
            max_gap = m_MeshParams.automatic_mesh.max_cell_size.Y;
            min_gap = m_MeshParams.automatic_mesh.min_cell_size.Y;
            break;
        case 2:
        default:
            mesh_axis = mesh.Z;
            max_gap = m_MeshParams.automatic_mesh.max_cell_size.Z;
            min_gap = m_MeshParams.automatic_mesh.min_cell_size.Z;
            break;
        }

        std::set<double>::iterator it0 = mesh_axis.begin();
        std::set<double>::iterator it1 = it0;
        it1++;
        std::set<double>::iterator it2 = it1;
        it2++;

        while(it2 != mesh_axis.end())
        {
            double size_left = *it1 - *it0;
            double size_right = *it2 - *it1;
            double gap_ratio;
            if(size_left < size_right)
            {
                gap_ratio = size_right / size_left;
            }
            else
            {
                gap_ratio = size_left / size_right;
            }
            if(gap_ratio > m_MeshParams.automatic_mesh.smth_neighbor_size_diff)
            {
                printf("WARNING: mesh gap ratio deviation. Configured max ratio %f, but actual ratio %f\n",
                       m_MeshParams.automatic_mesh.smth_neighbor_size_diff,
                       gap_ratio);
                warning = true;
            }
            if(size_left > max_gap || size_right > max_gap)
            {
                printf("ERROR: mesh gap size too large. Configured max size %f, but actual size %f\n",
                       max_gap, size_left > size_right ? size_left : size_right);
                warning = true;
            }
            if(size_left < min_gap || size_right < min_gap)
            {
                printf("ERROR: mesh gap size too small. Configured min size %f, but actual size %f\n",
                       min_gap, size_left < size_right ? size_left : size_right);
                warning = true;
            }

            it2++;
            it1++;
            it0++;
        }
    }

    if(warning)
    {
        printf("Check generated mesh file for detailed warning information\n");
    }

    return str;
}



PCB_EMS_Model::PCB_EMS_Model(srecs::charvec_t& Data, Configuration& Config)

                            : m_Config(Config),
                              m_SimBox(Config.SimulationBox),
                              m_MeshParams(Config.mesh_params),
                              m_ConvSet(Config.conversion_settings)
{
    m_MetalPriority = 1;
    m_PCBPriority = 0;
    //--------------------

    // create box segment if used
    if(Config.SimulationBox.box_fill.use_box_fill)
    {
        Configuration::xyz_triplet<double>& dim_min = Config.SimulationBox.min;
        Configuration::xyz_triplet<double>& dim_max = Config.SimulationBox.max;
        auto material = Config.SimulationBox.box_fill.box_material;

        std::complex<double> start(dim_min.X, 0);
        std::complex<double> end(dim_max.X, 0);
        Segment seg(end, start, dim_min.Z, dim_max.Y - dim_min.Y, dim_max.Z - dim_min.Z, 0, m_ConvSet.corner_approximation, material);
        m_Segments.push_back(seg);
    }

    m_LastViaDrill = 0.1;
    m_RescueViaDrill = true;
    m_AuxAxisIsOrigin = true;

    // extract metal and pcb primitives for EMS simulation
    SREC srec(Data.begin(), Data.end());

    srec.GetNext();
    if(srec.GetRecName() != "kicad_pcb")
    {
        std::cout << "Error: not kicad_pcb file format";
        return;
    }
    srec.GetChild();
    if(srec.GetRecord() != "(version 4)")
    {
        // print warning about version
        std::cout << "Warning: kicad_pcb file version not 4. May experience some errors";
    }

    srec.GetNext("setup");
    SREC setup = srec;
    if(setup.GetChild("aux_axis_origin"))
    {
        double x, y;
        std::string record;
        record = setup.GetRecord();
        if(sscanf(record.c_str(), "(aux_axis_origin %lf %lf", &x, &y) != 2) throw ems_exc("EMS_Model: 'aux_axis_origin' field read failed");
        y = -y;
        m_AuxAxisOrigin = complex<double>(x,y);
    }else
    {
        m_AuxAxisIsOrigin = false;
    }

    // go trough all records in kicad_pcb record
    while(srec.GetNext())
    {
        GetSegment(srec);
        GetVia(srec);
        GetZone(srec);
        GetModule(srec);
        GetPCB(srec);
    }

    // generate pcb outline
    GenPCB_Polygon();
}

void PCB_EMS_Model::GenPCB_Polygon()
{
    if(m_PCB_OutlineElements.size() == 0) return;
    // chain together outline line elements and generate polygon for PCB
    std::vector<std::complex<double>> points;

    size_t idx = 0;
    size_t inserted = 2;
    points.push_back(m_PCB_OutlineElements[idx].m_Start);
    points.push_back(m_PCB_OutlineElements[idx].m_End);
    std::complex<double> last_point = m_PCB_OutlineElements[idx].m_End;

    while(inserted < m_PCB_OutlineElements.size())
    {
        bool match = false;
        for(size_t n = 0; n < m_PCB_OutlineElements.size(); n++)
        {
            if(n == idx) continue;

            if(m_PCB_OutlineElements[n].m_Start == last_point)
            {
                match = true;
                last_point = m_PCB_OutlineElements[n].m_End;
            }else if(m_PCB_OutlineElements[n].m_End == last_point)
            {
                match = true;
                last_point = m_PCB_OutlineElements[n].m_Start;
            }
            if(match)
            {
                idx = n;
                points.push_back(last_point);
                inserted++;
                break;
            }
        }
        if(!match)
        {
            std::cout << "Failed to generate complete PCB outline" << "\n";
            break;
        }
    }

    bool is_clk_wise = IsClockWiseOrder(points);

    if(!is_clk_wise)
    {
        std::reverse(points.begin(), points.end());
    }

    Zone poly(points, 0, 0.2, m_ConvSet.pcb_height, m_PCBPriority, m_ConvSet.corner_approximation, m_SimBox.materials.pcb, true);
    m_Polys.push_back(poly);
}

bool PCB_EMS_Model::GetPCB(srecs::SREC Srec)
{
    std::string record_str;
    std::string rec_name = Srec.GetRecName();

    if(rec_name != "gr_line" && rec_name != "gr_circle" && rec_name != "gr_arc") return false;

    // extract layer information
    SREC rec = Srec;
    if(!rec.GetChild("layer")) throw ems_exc("GetPCB: no 'layer' field");
    record_str = rec.GetRecord();
    if(record_str.find("Edge.Cuts") != std::string::npos)
    {
        // Part of PCB edge layer
        complex<double> endp;
        complex<double> center;
        complex<double> startp;
        double angle, x, y;

        // get data
        rec = Srec;
        if(!rec.GetChild("end")) throw ems_exc("GetPCB: no 'end' field");
        record_str = rec.GetRecord();
        if(sscanf(record_str.c_str(), "(end %lf %lf", &x, &y) != 2) throw ems_exc("GetPCB: 'end' field read failed");
        y = -y;
        endp = complex<double>(x,y);

        if(rec_name == "gr_line" || rec_name == "gr_arc")
        {
            rec = Srec;
            if(!rec.GetChild("start")) throw ems_exc("GetPCB: no 'start' field");
            record_str = rec.GetRecord();
            if(sscanf(record_str.c_str(), "(start %lf %lf", &x, &y) != 2) throw ems_exc("GetPCB: 'start' field read failed");
            y = -y;
            startp = complex<double>(x,y);

            if(rec_name == "gr_arc")
            {
                rec = Srec;
                if(!rec.GetChild("angle")) throw ems_exc("GetPCB: no 'angle' field");
                record_str = rec.GetRecord();
                if(sscanf(record_str.c_str(), "(angle %lf", &angle) != 1) throw ems_exc("GetPCB: 'angle' field read failed");
            }

        }else
        {
            rec = Srec;
            if(!rec.GetChild("center")) throw ems_exc("GetPCB: no 'center' field");
            record_str = rec.GetRecord();
            if(sscanf(record_str.c_str(), "(center %lf %lf", &x, &y) != 2) throw ems_exc("GetPCB: 'center' field read failed");
            y = -y;
            center = complex<double>(x,y);
        }

        if(rec_name == "gr_line")
        {
            Line line;
            line.m_Start = MovePoint(startp, m_AuxAxisIsOrigin);
            line.m_End = MovePoint(endp, m_AuxAxisIsOrigin);
            m_PCB_OutlineElements.push_back(line);

        }else if(rec_name == "gr_arc")
        {
            // start end angle
            double approx_angle = M_PI / (m_ConvSet.corner_approximation + 1);
            double line_cnt = ceil(pems::deg_to_radian(angle) / approx_angle);
            double angle_step = pems::deg_to_radian(angle) / line_cnt;
            complex<double> first = endp;
            for(size_t i = 0; i < line_cnt; i++)
            {
                complex<double> second = rot_vector(endp - startp, -angle_step * (i + 1)) + startp;
                Line line;
                line.m_Start = MovePoint(first, m_AuxAxisIsOrigin);
                line.m_End = MovePoint(second, m_AuxAxisIsOrigin);
                m_PCB_OutlineElements.push_back(line);
                first = second;
            }

        }else if(rec_name == "gr_circle")
        {
        }
    }
    return true;
}

bool PCB_EMS_Model::GetModule(srecs::SREC Srec)
{
    if(Srec.GetRecName() != "module") return false;

    SREC module = Srec;
    SREC module_pos_rec = Srec;

    double x, y, rotation;
    rotation = 0;

    if(!module_pos_rec.GetChild("at")) throw ems_exc("GetModule: 'at' field read failed");
    std::string record = module_pos_rec.GetRecord();
    if(sscanf(record.c_str(), "(at %lf %lf %lf", &x, &y, &rotation) < 2) throw ems_exc("GetModule: 'at' field read failed");
    y = -y;

    if(module.GetChild("pad"))
    {
        do
        {
            GetPad(module, x, y, rotation);
        }while(module.GetNext("pad"));
    }

    return true;
}

bool PCB_EMS_Model::GetPad(srecs::SREC Srec, double ModuleX, double ModuleY, double ModuleRot)
{
    auto& pcb_t = m_ConvSet.pcb_metal_thickness;
    auto& pcb_h = m_ConvSet.pcb_height;
    auto& corner_approx = m_ConvSet.corner_approximation;

    SREC s_record = Srec;

    if(s_record.GetRecName() != "pad")
    {
        return false;
    }

    std::string record = s_record.GetRecord();

    char pad_id[record.size()];
    char type[record.size()];
    char shape[record.size()];
    if(sscanf(record.c_str(), "(pad %s %s %s", pad_id, type, shape) != 3) throw ems_exc("GetPad: read failed");

    double width, height, layer_height;
    double x, y;
    double rotation = 0;

    SREC data = s_record;
    if(!data.GetChild("at")) throw ems_exc("GetPad: no 'at' field");
    record = data.GetRecord();
    if(sscanf(record.c_str(), "(at %lf %lf %lf", &x, &y, &rotation) < 2) throw ems_exc("GetPad: 'at' field read failed");
    y = -y;

    data = s_record;
    if(!data.GetChild("size")) throw ems_exc("GetPad: no 'size' field");
    record = data.GetRecord();
    if(sscanf(record.c_str(), "(size %lf %lf", &width, &height) != 2) throw ems_exc("GetPad: 'size' field read failed");

    data = s_record;
    if(!data.GetChild("layers")) throw ems_exc("GetPad: no 'layers' field");
    record = data.GetRecord();
    Configuration::MaterialProps material;
    if(strstr(record.c_str(), "F.Cu") != nullptr)
    {
        layer_height = pcb_h;
        material = m_SimBox.materials.metal_top;
    }else
    {
        material = m_SimBox.materials.metal_bot;
        if(pcb_t == 0.0)
        {
            layer_height = -pcb_t;
        }
        else
        {
            layer_height = -pcb_h - pcb_t;
        }
    }

    double drill_x = 0;
    double drill_y = 0;
    if(strstr(type, "np_thru_hole") != nullptr || strstr(type, "thru_hole") != nullptr)
    {
        // drill pad
        data = s_record;
        if(!data.GetChild("drill")) throw ems_exc("GetPad: no 'drill' field");
        record = data.GetRecord();
        if(strstr(shape, "circle") != nullptr)
        {
            if(sscanf(record.c_str(), "(drill %lf", &drill_x) != 1) throw ems_exc("GetPad: 'drill' field read failed");
            drill_y = drill_x;
        }else if(strstr(shape, "oval") != nullptr)
        {
            if(sscanf(record.c_str(), "(drill oval %lf %lf", &drill_x, &drill_y) != 2) throw ems_exc("GetPad: 'drill' field read failed");
        }
    }

    // generate primitives
    if(strstr(type, "smd") != nullptr)
    {
        if(strstr(shape, "rect") != nullptr)
        {
            complex<double> startp = -width / 2;
            complex<double> endp = width / 2;
            startp = rot_vector(startp, deg_to_radian(rotation - ModuleRot)) + complex<double>(x,y);
            endp = rot_vector(endp, deg_to_radian(rotation - ModuleRot)) + complex<double>(x,y);
            startp = rot_vector(startp, deg_to_radian(ModuleRot)) + complex<double>(ModuleX,ModuleY);
            endp = rot_vector(endp, deg_to_radian(ModuleRot)) + complex<double>(ModuleX,ModuleY);
            startp = MovePoint(startp, m_AuxAxisIsOrigin);
            endp = MovePoint(endp, m_AuxAxisIsOrigin);

            Segment seg(startp, endp, layer_height, height, pcb_t, m_MetalPriority, 0, material);
            m_Segments.push_back(seg);
        }else
        {
            double segment_width;
            complex<double> startp, endp;
            if(width > height)
            {
                startp = -(width - height) / 2;
                endp = (width - height) / 2;
                segment_width = height;
            }else
            {
                startp = -(height - width) / 2;
                endp = (height - width) / 2;
                segment_width = width;
            }
            startp = rot_vector(startp, deg_to_radian(rotation - ModuleRot)) + complex<double>(x,y);
            endp = rot_vector(endp, deg_to_radian(rotation - ModuleRot)) + complex<double>(x,y);
            startp = rot_vector(startp, deg_to_radian(ModuleRot)) + complex<double>(ModuleX,ModuleY);
            endp = rot_vector(endp, deg_to_radian(ModuleRot)) + complex<double>(ModuleX,ModuleY);
            startp = MovePoint(startp, m_AuxAxisIsOrigin);
            endp = MovePoint(endp, m_AuxAxisIsOrigin);

            Segment seg(startp, endp, layer_height, segment_width, pcb_t, m_MetalPriority, corner_approx ? corner_approx : 1, material);
            m_Segments.push_back(seg);
        }
    }else
    {
        if(strstr(type, "np_thru_hole") != nullptr)
        {
            double drill;
            complex<double> startp, endp;
            if(width > height)
            {
                startp = -(width - height) / 2;
                endp = (width - height) / 2;
                drill = drill_y;
            }else
            {
                startp = -(height - width) / 2;
                endp = (height - width) / 2;
                drill = drill_x;
            }
            startp = rot_vector(startp, deg_to_radian(rotation - ModuleRot)) + complex<double>(x,y);
            endp = rot_vector(endp, deg_to_radian(rotation - ModuleRot)) + complex<double>(x,y);
            startp = rot_vector(startp, deg_to_radian(ModuleRot)) + complex<double>(ModuleX,ModuleY);
            endp = rot_vector(endp, deg_to_radian(ModuleRot)) + complex<double>(ModuleX,ModuleY);
            startp = MovePoint(startp, m_AuxAxisIsOrigin);
            endp = MovePoint(endp, m_AuxAxisIsOrigin);

            Via via(startp, endp, -pcb_t, drill / 2, pcb_h + 2 * pcb_t, m_MetalPriority,
                    corner_approx, drill, material, m_SimBox.materials.hole_fill);
            m_Vias.push_back(via);
        }else
        {
            double segment_width, drill;
            complex<double> startp, endp;
            if(width > height)
            {
                startp = -(width - height) / 2;
                endp = (width - height) / 2;
                segment_width = height;
                drill = drill_y;
            }else
            {
                startp = -(height - width) / 2;
                endp = (height - width) / 2;
                segment_width = width;
                drill = drill_x;
            }
            startp = rot_vector(startp, deg_to_radian(rotation - ModuleRot)) + complex<double>(x,y);
            endp = rot_vector(endp, deg_to_radian(rotation - ModuleRot)) + complex<double>(x,y);
            startp = rot_vector(startp, deg_to_radian(ModuleRot)) + complex<double>(ModuleX,ModuleY);
            endp = rot_vector(endp, deg_to_radian(ModuleRot)) + complex<double>(ModuleX,ModuleY);
            startp = MovePoint(startp, m_AuxAxisIsOrigin);
            endp = MovePoint(endp, m_AuxAxisIsOrigin);

            Via via(startp, endp, -pcb_t, segment_width, pcb_h + 2 * pcb_t, m_MetalPriority,
                    corner_approx, drill, material, m_SimBox.materials.hole_fill);
            m_Vias.push_back(via);
        }
    }

    return true;
}

bool PCB_EMS_Model::GetZone(SREC Srec)
{
    // get all segments of PCB
    double point_x;
    double point_y;
    double width;
    std::string layer;
    std::string record;

    if(Srec.GetRecName() != "zone") return false;

    // layer
    if(!Srec.GetChild("layer")) throw ems_exc("GetZone: no 'layer' field");
    record = Srec.GetRecord();
    layer.insert(layer.begin(), record.begin() + 7, record.end() - 1);
    double height;
    Configuration::MaterialProps material;
    if(layer == "F.Cu")
    {
        height = m_ConvSet.pcb_height;
        material = m_SimBox.materials.metal_top;
    }
    else if(layer == "B.Cu")
    {
        height = -m_ConvSet.pcb_metal_thickness;
        material = m_SimBox.materials.metal_bot;
    }
    else
    {
        // skip zone
        return true;
    }

    // width
    if(!Srec.GetNext("min_thickness")) throw ems_exc("GetZone: no 'min_thickness' field");
    record = Srec.GetRecord();
    if(sscanf(record.c_str(), "(min_thickness %lf", &width) != 1) throw ems_exc("GetZone: 'min_thickness' field read failed");

    // points
    while(Srec.GetNext("filled_polygon"))
    {
        std::vector<std::complex<double>> points;
        SREC filled_poly = Srec;

        if(!filled_poly.GetChild("pts")) throw ems_exc("GetZone: no 'pts' field for zone 'filled_polygon'");

        if(!filled_poly.GetChild("xy")) throw ems_exc("GetZone: no 'xy' field for zone 'filled_polygon' 'pts'");
        record = filled_poly.GetRecord();
        if(sscanf(record.c_str(), "(xy %lf %lf", &point_x, &point_y) != 2) throw ems_exc("GetZone: 'xy' field read failed");
        point_y = -point_y;
        auto first = complex<double>(point_x, point_y);
        first = MovePoint(first, m_AuxAxisIsOrigin);
        points.push_back(first);

        // while there are points to parse
        while(filled_poly.GetNext("xy"))
        {
            record = filled_poly.GetRecord();
            if(sscanf(record.c_str(), "(xy %lf %lf", &point_x, &point_y) != 2) throw ems_exc("GetZone: 'xy' field read failed");
            point_y = -point_y;
            auto p = complex<double>(point_x, point_y);
            p = MovePoint(p, m_AuxAxisIsOrigin);
            points.push_back(p);
        }

        // delete if last and first point match same location
        if(points[points.size() - 1] == points[0])
        {
            points.erase(points.end() - 1);
        }
        Zone poly(points, height, width, m_ConvSet.pcb_metal_thickness, m_MetalPriority, m_ConvSet.corner_approximation,
                           material, false);

        m_Polys.push_back(poly);
    }

    return true;
}

bool PCB_EMS_Model::GetSegment(SREC Srec)
{
    // get all segments of PCB
    double start_x;
    double start_y;
    double end_x;
    double end_y;
    double width;
    std::string layer;
    std::string record;

    if(Srec.GetRecName() != "segment") return false;

    if(!Srec.GetChild("start")) throw ems_exc("GetSegment: no 'start' field");
    record = Srec.GetRecord();
    if(sscanf(record.c_str(), "(start %lf %lf", &start_x, &start_y) != 2) throw ems_exc("GetSegment: 'start' field read failed");
    start_y = -start_y;

    if(!Srec.GetNext("end")) throw ems_exc("GetSegment: no 'end' field");
    record = Srec.GetRecord();
    if(sscanf(record.c_str(), "(end %lf %lf", &end_x, &end_y) != 2) throw ems_exc("GetSegment: 'end' field read failed");
    end_y = -end_y;

    if(!Srec.GetNext("width")) throw ems_exc("GetSegment: no 'width' field");
    record = Srec.GetRecord();
    if(sscanf(record.c_str(), "(width %lf", &width) != 1) throw ems_exc("GetSegment: 'width' field read failed");

    if(!Srec.GetNext("layer")) throw ems_exc("GetSegment: no 'layer' field");;
    record = Srec.GetRecord();
    layer.insert(layer.begin(), record.begin() + 7, record.end() - 1);

    auto a = std::complex<double>(start_x, start_y);
    auto b = std::complex<double>(end_x, end_y);
    a = MovePoint(a, m_AuxAxisIsOrigin);
    b = MovePoint(b, m_AuxAxisIsOrigin);
    double height;
    Configuration::MaterialProps material;
    if(layer == "F.Cu")
    {
        height = m_ConvSet.pcb_height;
        material = m_SimBox.materials.metal_top;
    }
    else
    {
        height = -m_ConvSet.pcb_metal_thickness;
        material = m_SimBox.materials.metal_bot;
    }

    Segment seg(a, b, height, width, m_ConvSet.pcb_metal_thickness, m_MetalPriority, m_ConvSet.corner_approximation, material);

    m_Segments.push_back(seg);

//    std::cout << "segment" << std::endl;

    return true;
}

bool PCB_EMS_Model::GetVia(srecs::SREC Srec)
{
    // get all segments of PCB
    double start_x;
    double start_y;
    double size, drill;
    std::string record;

    if(Srec.GetRecName() != "via") return false;

    if(!Srec.GetChild("at")) throw ems_exc("GetVia: no 'at' field");
    record = Srec.GetRecord();
    if(sscanf(record.c_str(), "(at %lf %lf", &start_x, &start_y) != 2) throw ems_exc("GetVia: 'at' field read failed");
    start_y = -start_y;

    if(!Srec.GetNext("size")) throw ems_exc("GetVia: no 'size' field");
    record = Srec.GetRecord();
    if(sscanf(record.c_str(), "(size %lf", &size) != 1) throw ems_exc("GetVia: 'size' field read failed");

    if(!Srec.GetNext("drill"))
    {
        if(!m_RescueViaDrill) throw ems_exc("GetVia: no 'drill' field");
        drill = m_LastViaDrill;
    }else
    {
        record = Srec.GetRecord();
        if(sscanf(record.c_str(), "(drill %lf", &drill) != 1) throw ems_exc("GetVia: 'drill' field read failed");
    }

    m_LastViaDrill = drill;

    auto a = std::complex<double>(start_x, start_y);
    a = MovePoint(a, m_AuxAxisIsOrigin);

    auto pcb_t = m_ConvSet.pcb_metal_thickness;
    auto pcb_h = m_ConvSet.pcb_height;
    auto corner_approx = m_ConvSet.corner_approximation;

    Via via(a, a, -pcb_t, size, pcb_h + 2 * pcb_t, m_MetalPriority, corner_approx,
            drill, m_SimBox.materials.metal_top, m_SimBox.materials.hole_fill);

    m_Vias.push_back(via);

    return true;
}

std::complex<double> PCB_EMS_Model::MovePoint(std::complex<double> Point, bool Move)
{
    if(Move)
    {
        return Point - m_AuxAxisOrigin;
    }
    return Point;
}

std::string printMeshSet(std::set<double>& MeshSet)
{
    std::string str;
    std::set<double>::iterator it = MeshSet.begin();
    while(it != MeshSet.end())
    {
        str += std::to_string(*it);
        it++;
        if(it != MeshSet.end())
        {
            str += ",";
        }
    }
    return str;
}





//
