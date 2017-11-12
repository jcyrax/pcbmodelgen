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

#include <fstream>
#include <exception>
#include <system_error>

#include "kicadtoems_config.hpp"
#include "json/json.h"

using namespace kicad_to_ems;

class load_conf_exc : public std::runtime_error
{
public:
    load_conf_exc(const char* Msg) : std::runtime_error(Msg) {}
};

Configuration::xyz_triplet<size_t> LoadTriplet_Int(Json::Value& ConfStruct);
Configuration::xyz_triplet<double> LoadTriplet_Double(Json::Value& ConfStruct);
Configuration::xyz_triplet<std::string> LoadTriplet_String(Json::Value& ConfStruct);
Configuration::MaterialProps LoadMaterial(Json::Value& ConfStruct);


void TestKey(Json::Value& Element, const char* Key);
double GetConf_asDouble(Json::Value& Element, const char* Key);
double GetConf_asDouble(Json::Value& Element, size_t Index);
std::string GetConf_asString(Json::Value& Element, const char* Key);
int GetConf_asInt(Json::Value& Element, const char* Key);
bool GetConf_asBool(Json::Value& Element, const char* Key);






/**
    @brief Load configuration parameters from json config file
*/
void Configuration::LoadConfig(const char* ConfigFile)
{
    std::fstream conf_stream(ConfigFile, std::fstream::in);
    Json::Value conf;
    conf_stream >> conf;
    conf_stream.close();


    Json::Value conv_set = conf["conversion_settings"];
    // =====================================================================================================================
    conversion_settings.pcb_height = GetConf_asDouble(conv_set, "pcb_height");
    conversion_settings.pcb_metal_thickness = GetConf_asDouble(conv_set, "pcb_metal_thickness");
    conversion_settings.pcb_metal_zero_thick = GetConf_asBool(conv_set, "pcb_metal_zero_thick");
    conversion_settings.corner_approximation = GetConf_asInt(conv_set, "corner_approximation");
    // configuration interactions
    if(conversion_settings.pcb_metal_zero_thick)
    {
        conversion_settings.pcb_metal_thickness = 0.0;
    }

    Json::Value mesh_par = conf["mesh_params"];
    // =====================================================================================================================
    mesh_params.automatic_mesh.insert_automatic_mesh = GetConf_asBool(mesh_par["automatic_mesh"], "insert_automatic_mesh");
    mesh_params.automatic_mesh.smooth_mesh_lines = GetConf_asBool(mesh_par["automatic_mesh"], "smooth_mesh_lines");
    mesh_params.automatic_mesh.smth_neighbor_size_diff = GetConf_asDouble(mesh_par["automatic_mesh"], "smth_neighbor_size_diff");
    mesh_params.automatic_mesh.pcb_z_lines = GetConf_asInt(mesh_par["automatic_mesh"], "pcb_z_lines");
    mesh_params.automatic_mesh.remove_small_cells = GetConf_asBool(mesh_par["automatic_mesh"], "remove_small_cells");
    mesh_params.automatic_mesh.min_cell_size = LoadTriplet_Double(mesh_par["automatic_mesh"]["min_cell_size"]);
    mesh_params.automatic_mesh.max_cell_size = LoadTriplet_Double(mesh_par["automatic_mesh"]["max_cell_size"]);
    mesh_params.manual_mesh.insert_manual_mesh = GetConf_asBool(mesh_par["manual_mesh"], "insert_manual_mesh");

    for(size_t i = 0; i < mesh_par["manual_mesh"]["X"].size(); ++i)
    {
        mesh_params.manual_mesh.X.push_back(GetConf_asDouble(mesh_par["manual_mesh"]["X"], i));
    }
    for(size_t i = 0; i < mesh_par["manual_mesh"]["Y"].size(); ++i)
    {
        mesh_params.manual_mesh.Y.push_back(GetConf_asDouble(mesh_par["manual_mesh"]["Y"], i));
    }
    for(size_t i = 0; i < mesh_par["manual_mesh"]["Z"].size(); ++i)
    {
        mesh_params.manual_mesh.Z.push_back(GetConf_asDouble(mesh_par["manual_mesh"]["Z"], i));
    }

    Json::Value sim_box = conf["SimulationBox"];
    // =====================================================================================================================
    if(sim_box.isMember("min") && sim_box.isMember("max"))
    {
        SimulationBox.min = LoadTriplet_Double(sim_box["min"]);
        SimulationBox.max = LoadTriplet_Double(sim_box["max"]);
        SimulationBox.SimBoxUsed = true;
    }
    else
    {
        SimulationBox.SimBoxUsed = false;
    }
    SimulationBox.box_fill.use_box_fill = GetConf_asBool(sim_box["box_fill"], "use_box_fill");
    SimulationBox.box_fill.box_material = LoadMaterial(sim_box["box_fill"]["box_material"]);
    SimulationBox.materials.pcb = LoadMaterial(sim_box["materials"]["pcb"]);
    SimulationBox.materials.metal_top = LoadMaterial(sim_box["materials"]["metal_top"]);
    SimulationBox.materials.metal_bot = LoadMaterial(sim_box["materials"]["metal_bot"]);
    SimulationBox.materials.hole_fill = LoadMaterial(sim_box["materials"]["hole_fill"]);

    // hard-coded material names
    SimulationBox.box_fill.box_material.Name = "box";
    SimulationBox.materials.pcb.Name = "pcb";
    SimulationBox.materials.metal_top.Name = "metal_top";
    SimulationBox.materials.metal_bot.Name = "metal_bot";
    SimulationBox.materials.hole_fill.Name = "hole_fill";
}




Configuration::MaterialProps LoadMaterial(Json::Value& ConfStruct)
{
    Configuration::MaterialProps material;

    material.IsPEC = ConfStruct.get("IsPEC", false).asBool();
    material.Epsilon = ConfStruct.get("Epsilon", 1).asDouble();
    material.Mue = ConfStruct.get("Mue", 1).asDouble();
    material.Kappa = ConfStruct.get("Kappa", 0.0).asDouble();
    material.Sigma = ConfStruct.get("Sigma", 0.0).asDouble();
    material.Density = ConfStruct.get("Density", 1).asDouble();

    material.boundary_one_third_rule = ConfStruct.get("boundary_one_third_rule", false).asBool();
    material.boundary_additional_lines = ConfStruct.get("boundary_additional_lines", false).asBool();
    material.boundary_rule_distance = ConfStruct.get("boundary_rule_distance", 0.0).asDouble();

    return material;
}


Configuration::xyz_triplet<size_t> LoadTriplet_Int(Json::Value& ConfStruct)
{
    Configuration::xyz_triplet<size_t> triplet;

    triplet.X = GetConf_asInt(ConfStruct, "X");
    triplet.Y = GetConf_asInt(ConfStruct, "Y");
    triplet.Z = GetConf_asInt(ConfStruct, "Z");
    return triplet;
}

Configuration::xyz_triplet<std::string> LoadTriplet_String(Json::Value& ConfStruct)
{
    Configuration::xyz_triplet<std::string> triplet;

    triplet.X = GetConf_asString(ConfStruct, "X");
    triplet.Y = GetConf_asString(ConfStruct, "Y");
    triplet.Z = GetConf_asString(ConfStruct, "Z");
    return triplet;
}

Configuration::xyz_triplet<double> LoadTriplet_Double(Json::Value& ConfStruct)
{
    Configuration::xyz_triplet<double> triplet;

    triplet.X = GetConf_asDouble(ConfStruct, "X");
    triplet.Y = GetConf_asDouble(ConfStruct, "Y");
    triplet.Z = GetConf_asDouble(ConfStruct, "Z");
    return triplet;
}



double GetConf_asDouble(Json::Value& Element, const char* Key)
{
    TestKey(Element, Key);
    return Element[Key].asDouble();
}
double GetConf_asDouble(Json::Value& Element, size_t Index)
{
    return Element[(int)Index].asDouble();
}
std::string GetConf_asString(Json::Value& Element, const char* Key)
{
    TestKey(Element, Key);
    return Element[Key].asString();
}
int GetConf_asInt(Json::Value& Element, const char* Key)
{
    TestKey(Element, Key);
    return Element[Key].asInt();
}
bool GetConf_asBool(Json::Value& Element, const char* Key)
{
    TestKey(Element, Key);
    return Element[Key].asBool();
}




void TestKey(Json::Value& Element, const char* Key)
{
    if(!Element.isMember(Key))
    {
        std::string error = "Element with key: ";
        error.append(Key);
        error.append(" is not found!");

        throw load_conf_exc(error.c_str());
    }
}






