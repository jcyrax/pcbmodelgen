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

#ifndef ems_prims_h
#define ems_prims_h

#include "srecs.hpp"
#include "kicadtoems_config.hpp"

#include <tinyxml2.h>
#include <complex>
#include <set>
#include <stdexcept>
#include <map>

namespace kicad_to_ems
{
namespace pems
{

class ems_exc : public std::runtime_error
{
public:
    ems_exc(const char* Msg) : std::runtime_error(Msg) {}
};

#ifndef M_PI
#define M_PI 3.14159265358979323846
#define M_PI_2 (3.14159265358979323846 / 2.0)
#endif

double deg_to_radian(double degrees);
std::complex<double> rot_vector(std::complex<double> Vect, double RotAngle);
bool IsClockWiseOrder(std::vector<std::complex<double>>& Data);

struct Line {
    std::complex<double> m_Start;
    std::complex<double> m_End;
};

struct MeshLines {
    std::set<double> X;
    std::set<double> Y;
    std::set<double> Z;
};

class ExtrudedPolygon
{
    std::vector<std::complex<double>> m_PolyOutline;
    double m_Z_Height;
    double m_Thickness;
    size_t m_Priority;
    std::string m_MaterialName;

public:
    ExtrudedPolygon(std::vector<std::complex<double>>& Outline,
                    double Z_Height,
                    double Thickness,
                    size_t Priority,
                    std::string& MaterialName);
    std::string GetCSX_Script();
    void GetXML_Primitive(tinyxml2::XMLElement* InsertNode, std::string& GetByMaterial);
};

class Segment
{
private:
    std::complex<double> m_Start;
    std::complex<double> m_End;
    double m_Width;
    double m_Z;
    double m_T;
    size_t m_Priority;
    size_t m_CornerApprox;
    Configuration::MaterialProps m_Material;
    const size_t m_PrecisionDigits = 6;
    MeshLines m_Mesh;
    std::vector<std::complex<double>> m_PolyOutline;

    void GenMeshLines();
    void GenPolyOutline();

public:
    Segment(std::complex<double>& P1,
            std::complex<double>& P2,
            double Z,
            double W,
            double T,
            size_t Priority,
            size_t Approx,
            Configuration::MaterialProps& Material);

    std::string GetCSX_Script();
    MeshLines GetMeshData();
    void GetXML_Primitive(tinyxml2::XMLElement* InsertNode, std::string& Material);
};

class Via
{
    double m_DrillSize;
    double m_MetalSize;
    Segment m_Cilinder;
    Segment m_Mill;

public:
    Via(std::complex<double>& P1,
        std::complex<double>& P2,
        double Z,
        double W,
        double T,
        size_t Priority,
        size_t Approx,
        double WMill,
        Configuration::MaterialProps& MaterialRing,
        Configuration::MaterialProps& MaterialHole);

    std::string GetCSX_Script();
    MeshLines GetMeshData();
    void GetXML_Primitive(tinyxml2::XMLElement* InsertNode, std::string& Material);
};

class Zone
{
    std::vector<std::complex<double>> m_InnerOutline;
    std::vector<std::complex<double>> m_RealOutline;
    std::vector<pems::ExtrudedPolygon> m_OutlinePolys;
    std::vector<pems::Segment> m_Segments;
    double m_Z;
    double m_T;
    size_t m_Priority;
    size_t m_Approx;
    Configuration::MaterialProps m_Material;
    double getVectAngle(std::complex<double> U, std::complex<double> V);

public:
    Zone(std::vector<std::complex<double>>& OutlineCenterPts,
         double Z,
         double W,
         double T,
         size_t Priority,
         size_t Approx,
         Configuration::MaterialProps& Material,
         bool OutlineIsCenter);

    std::string GetCSX_Script();
    MeshLines GetMeshData();
    void GetXML_Primitive(tinyxml2::XMLElement* InsertNode, std::string& Material);
    void ApproximatePolygon(std::vector<std::complex<double>>& Points, double MaxError);
};

// add approximation back

} // namespace pems
} // namespace kicad_to_ems

#endif // ems_prims_h
