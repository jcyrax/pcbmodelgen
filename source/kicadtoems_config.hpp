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

#ifndef kicadtoems_config_h
#define kicadtoems_config_h

#include <string>
#include <vector>

namespace kicad_to_ems
{

class Configuration
{
public:
    void LoadConfig(const char* ConfigFile);

    struct MaterialProps {
        std::string Name;
        bool IsPEC;
        double Epsilon;
        double Mue;
        double Kappa;
        double Sigma;
        double Density;
        bool boundary_one_third_rule;
        bool boundary_additional_lines;
        double boundary_rule_distance;
    };
    template <typename T> struct xyz_triplet {
        T X;
        T Y;
        T Z;
    };

    struct conversion_settings_t {
        double pcb_height;
        double pcb_metal_thickness;
        bool pcb_metal_zero_thick;
        size_t corner_approximation;
    } conversion_settings;

    struct mesh_params_t {
        struct automatic_mesh_t {
            bool insert_automatic_mesh; // insert additional manual mesh lines
            bool smooth_mesh_lines;
            double smth_neighbor_size_diff;
            bool remove_small_cells;
            size_t pcb_z_lines;
            xyz_triplet<double> min_cell_size; // minimum mesh cell size
            xyz_triplet<double> max_cell_size; // maximum mesh cell size
        } automatic_mesh;
        struct manual_mesh_t {
            bool insert_manual_mesh; // insert additional manual mesh lines
            std::vector<double> X;   // mesh lines for X
            std::vector<double> Y;   // mesh lines for Y
            std::vector<double> Z;   // mesh lines for Z
        } manual_mesh;
    } mesh_params;

    struct SimulationBox_t {
        bool SimBoxUsed;
        xyz_triplet<double> min; // box boundary minimal values for xyz
        xyz_triplet<double> max; // box boundary maximal values for xyz
        struct box_fill_t {
            bool use_box_fill;          // if set simulation box is filled with material
            MaterialProps box_material; // material for simulation box fill
        } box_fill;
        struct materials_t {
            MaterialProps pcb;
            MaterialProps metal_top;
            MaterialProps metal_bot;
            MaterialProps hole_fill;
        } materials;
    } SimulationBox;
};

} // namespace kicad_to_ems

#endif // kicadtoems_config_h
