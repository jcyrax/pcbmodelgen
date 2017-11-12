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

#include "misc.hpp"
#include "ems_prims.hpp"
#include "boost/format.hpp"
#include <iostream>
#include <iomanip>
#include <cassert>
#include <sstream>
#include <cstring>


using namespace kicad_to_ems;
using namespace kicad_to_ems::pems;
using namespace kicad_to_ems::srecs;
using std::complex;


namespace kicad_to_ems
{
namespace pems
{
void insert_arc(std::vector<std::complex<double>>& Points, std::complex<double> Center,
                                    std::complex<double> Start, size_t Approx);
double round_to_n_digits(double x, size_t n);
}
}

ExtrudedPolygon::ExtrudedPolygon(std::vector<std::complex<double>>& Outline, double Z_Height, double Thickness,
                                 size_t Priority, std::string& MaterialName)
                : m_PolyOutline(Outline), m_Z_Height(Z_Height), m_Thickness(Thickness), m_Priority(Priority), m_MaterialName(MaterialName)
{
}

std::string ExtrudedPolygon::GetCSX_Script()
{
    using boost::format;
    std::string str_points;
    std::string str_full;
    size_t points = m_PolyOutline.size();
    if(points == 0) return str_full;

    complex<double> last_point = m_PolyOutline[0];

    size_t p = 0;
    for(size_t i = 0; i < points; ++i)
    {
        if(i > 0 && last_point == m_PolyOutline[i]) continue;
        last_point = m_PolyOutline[i];
        format fmt = format("p(1,%1%)=%2$.6f;p(2,%1%)=%3$.6f;\n") % (p + 1) % m_PolyOutline[i].real() % m_PolyOutline[i].imag();
        str_points += fmt.str();
        p++;
    }

    format fmt = format("p=zeros(2,%1%);\n") % p;
    str_full += fmt.str();
    str_full += str_points;

    if(m_Thickness == 0)
    {
        fmt = format("CSX = AddPolygon(CSX, '%1%', %3%, 2, %2%, p);\n") % m_MaterialName % m_Z_Height % m_Priority;
    }
    else
    {
        fmt = format("CSX = AddLinPoly(CSX, '%1%', %4%, 2, %2%, p, %3%);\n") % m_MaterialName % m_Z_Height % m_Thickness % m_Priority;
    }
    str_full += fmt.str();

    return str_full;
}

void ExtrudedPolygon::GetXML_Primitive(tinyxml2::XMLElement* InsertNode, std::string& GetByMaterial)
{
    // insert only if materials match
    if(GetByMaterial == m_MaterialName)
    {
        size_t points = m_PolyOutline.size();
        if(points == 0) return;

        tinyxml2::XMLElement* lin_poly;
        if(m_Thickness != 0) lin_poly = InsertNode->GetDocument()->NewElement("LinPoly");
        else lin_poly = InsertNode->GetDocument()->NewElement("Polygon");
        if(lin_poly == nullptr) throw ems_exc("TinyXML2 Create NewElement() failed");
        lin_poly->SetAttribute("Priority", (unsigned int)m_Priority);
        lin_poly->SetAttribute("Elevation", m_Z_Height);
        if(m_Thickness != 0) lin_poly->SetAttribute("Length", m_Thickness);
        lin_poly->SetAttribute("NormDir", 2);

        complex<double> last_point = m_PolyOutline[0];

        for(size_t i = 0; i < points; ++i)
        {
            if(i > 0 && last_point == m_PolyOutline[i]) continue;
            last_point = m_PolyOutline[i];

            tinyxml2::XMLElement* vertex = lin_poly->GetDocument()->NewElement("Vertex");
            if(vertex == nullptr) throw ems_exc("TinyXML2 Create NewElement() failed");

            vertex->SetAttribute("X1", m_PolyOutline[i].real());
            vertex->SetAttribute("X2", m_PolyOutline[i].imag());
            lin_poly->InsertEndChild(vertex);
        }

        InsertNode->InsertEndChild(lin_poly);
    }
}

Segment::Segment(std::complex<double>& P1,
                 std::complex<double>& P2,
                 double Z,
                 double W,
                 double T,
                 size_t Priority,
                 size_t Approx,
                 Configuration::MaterialProps& Material)

               : m_Start(P1), m_End(P2), m_Width(W), m_Z(Z), m_T(T), m_Priority(Priority), m_CornerApprox(Approx),
                 m_Material(Material)
{
    GenPolyOutline();
    GenMeshLines();
}

std::string Segment::GetCSX_Script()
{
    ExtrudedPolygon polygon(m_PolyOutline, m_Z, m_T, m_Priority, m_Material.Name);
    return polygon.GetCSX_Script();
}

void Segment::GetXML_Primitive(tinyxml2::XMLElement* InsertNode, std::string& Material)
{
    ExtrudedPolygon polygon(m_PolyOutline, m_Z, m_T, m_Priority, m_Material.Name);
    polygon.GetXML_Primitive(InsertNode, Material);
}

MeshLines Segment::GetMeshData()
{
    return m_Mesh;
}

void Segment::GenMeshLines()
{
    for(size_t i = 0; i < m_PolyOutline.size(); ++i)
    {
        m_Mesh.X.insert(round_to_n_digits(m_PolyOutline[i].real(), m_PrecisionDigits));
        m_Mesh.Y.insert(round_to_n_digits(m_PolyOutline[i].imag(), m_PrecisionDigits));
    }
    m_Mesh.Z.insert(m_Z);
    m_Mesh.Z.insert(m_Z + m_T);
}

void Segment::GenPolyOutline()
{
    complex<double> nvect;
    if(m_Start == m_End)
    {
        nvect = 1;
    }else
    {
        nvect = (m_End - m_Start) / abs(m_End - m_Start);
    }

    complex<double> nleft = rot_vector(nvect, -M_PI_2);
    complex<double> nright = rot_vector(nvect, +M_PI_2);
    complex<double> end_point = m_End;
    complex<double> point = m_Start;

    insert_arc(m_PolyOutline, m_Start, nright * m_Width / 2.0, m_CornerApprox);
    insert_arc(m_PolyOutline, end_point, nleft * m_Width / 2.0, m_CornerApprox);
    point += nright * m_Width / 2.0;
    m_PolyOutline.push_back(point);
}



Via::Via(std::complex<double>& P1,
         std::complex<double>& P2,
         double Z,
         double W,
         double T,
         size_t Priority,
         size_t Approx,
         double WMill,
         Configuration::MaterialProps& MaterialRing,
         Configuration::MaterialProps& MaterialHole)

       : m_DrillSize(WMill),
         m_MetalSize(W),
         m_Cilinder(P1, P2, Z, W, T, Priority, (Approx == 0 ? 1 : Approx), MaterialRing),
         m_Mill(P1, P2, Z, WMill, T, Priority + 1, (Approx == 0 ? 1 : Approx), MaterialHole)
{
}

std::string Via::GetCSX_Script()
{
    if(m_DrillSize >= m_MetalSize)
    {
        return m_Mill.GetCSX_Script();
    }
    return m_Cilinder.GetCSX_Script() + m_Mill.GetCSX_Script();
}

MeshLines Via::GetMeshData()
{
    MeshLines mesh = m_Cilinder.GetMeshData();
    MeshLines mesh_mill = m_Mill.GetMeshData();

    mesh.X.insert(mesh_mill.X.begin(), mesh_mill.X.end());
    mesh.Y.insert(mesh_mill.Y.begin(), mesh_mill.Y.end());
    mesh.Z.insert(mesh_mill.Z.begin(), mesh_mill.Z.end());

    return mesh;
}

void Via::GetXML_Primitive(tinyxml2::XMLElement* InsertNode, std::string& Material)
{
    m_Mill.GetXML_Primitive(InsertNode, Material);

    if(m_DrillSize < m_MetalSize)
    {
        m_Cilinder.GetXML_Primitive(InsertNode, Material);
    }
}



Zone::Zone(std::vector<std::complex<double>>& OutlineCenterPts,
             double Z,
             double W,
             double T,
             size_t Priority,
             size_t Approx,
             Configuration::MaterialProps& Material,
             bool OutlineIsCenter)

           : m_Z(Z),
             m_T(T),
             m_Priority(Priority),
             m_Approx(Approx),
             m_Material(Material)
{
    using boost::format;

    if(OutlineCenterPts.size() < 3) return;

    std::vector<std::complex<double>> outline_center = OutlineCenterPts;

    // get winding order (clockwise or counterclockwise)
    bool is_clkwise = IsClockWiseOrder(outline_center);
    if(!is_clkwise)
    {
        std::reverse(outline_center.begin(), outline_center.end());
    }

    // Generate inner outline and real zone outline.
    // Inner outline is used for one large zone, outer outline is used to construct
    // smaller polygons around the center zone.
    // This is because KiCAD zones outline is stored as center of draw tool and
    // openEMS doesn't allow self intersecting polygons.
    std::vector<std::complex<double>>::iterator prev;
    std::vector<std::complex<double>>::iterator center;
    std::vector<std::complex<double>>::iterator next;
    for(size_t i = 0, index = 2; i < outline_center.size(); ++i, ++index)
    {
        next = outline_center.begin() + (index >= outline_center.size() ? index - outline_center.size() : index);
        center = outline_center.begin() + (index - 1 >= outline_center.size() ? index - 1 - outline_center.size() : index - 1);
        prev = outline_center.begin() + (index - 2 >= outline_center.size() ? index - 2 - outline_center.size() : index - 2);

        // first line data
        std::complex<double> line_first = *center - *prev;
        std::complex<double> vnorm_first = line_first / abs(line_first);
        std::complex<double> vnorm_right_first = rot_vector(vnorm_first, -M_PI_2);

        // second line data
        std::complex<double> line_second = *next - *center;
        std::complex<double> vnorm_second = line_second / abs(line_second);
        std::complex<double> vnorm_right_second = rot_vector(vnorm_second, -M_PI_2);

        std::complex<double> voffset_right = vnorm_right_first * (OutlineIsCenter ? 0.0 : 0.001);
        std::complex<double> voffset_left = -vnorm_right_first * (W / 2.0);

        double angle = getVectAngle(vnorm_right_first, vnorm_right_second);
        double offset_right_mag = abs(voffset_right) / cos(angle / 2);
        double offset_left_mag = abs(voffset_left) / cos(angle / 2);

        std::complex<double> vect_new_point_right = std::polar(offset_right_mag, std::arg(vnorm_right_first + vnorm_right_second));
        std::complex<double> vect_new_point_left = std::polar(offset_left_mag, std::arg(vnorm_right_first + vnorm_right_second) - M_PI);

        m_RealOutline.push_back(*center + vect_new_point_left);
        m_InnerOutline.push_back(*center + vect_new_point_right);
    }

    for(size_t i = 1; i < outline_center.size() + 1; ++i)
    {
        std::vector<std::complex<double>> points;

        points.push_back(m_RealOutline[i >= outline_center.size() ? 0 : i]);

        // if lines intersect switch order
        if(pems::Misc::lines_intersect(m_RealOutline[i >= outline_center.size() ? 0 : i],
                                       m_InnerOutline[i >= outline_center.size() ? 0 : i],
                                       m_RealOutline[i - 1],
                                       m_InnerOutline[i - 1]))
        {
            points.push_back(m_InnerOutline[i - 1]);
            points.push_back(m_InnerOutline[i >= outline_center.size() ? 0 : i]);
        }
        else
        {
            points.push_back(m_InnerOutline[i >= outline_center.size() ? 0 : i]);
            points.push_back(m_InnerOutline[i - 1]);
        }

        points.push_back(m_RealOutline[i - 1]);

        ExtrudedPolygon polygon(points, m_Z, m_T, m_Priority, m_Material.Name);
        m_OutlinePolys.push_back(polygon);
    }
}

std::string Zone::GetCSX_Script()
{
    std::string segment_blocks;

    for(size_t i = 0; i < m_OutlinePolys.size(); i++)
    {
        segment_blocks += m_OutlinePolys[i].GetCSX_Script();
    }
    ExtrudedPolygon polygon(m_InnerOutline, m_Z, m_T, m_Priority, m_Material.Name);

    return segment_blocks + polygon.GetCSX_Script();
}

void Zone::GetXML_Primitive(tinyxml2::XMLElement* InsertNode, std::string& Material)
{
    ExtrudedPolygon polygon(m_InnerOutline, m_Z, m_T, m_Priority, m_Material.Name);
    polygon.GetXML_Primitive(InsertNode, Material);

    for(size_t i = 0; i < m_OutlinePolys.size(); i++)
    {
        m_OutlinePolys[i].GetXML_Primitive(InsertNode, Material);
    }
}

MeshLines Zone::GetMeshData()
{
    MeshLines mesh;

    bool one_third_rule = m_Material.boundary_one_third_rule;
    bool boundary_lines = m_Material.boundary_additional_lines;
    double rule_distance = m_Material.boundary_rule_distance;

    for(size_t i = 1; i < m_RealOutline.size() + 1; i++)
    {
        size_t index = i;
        size_t prev_index = i - 1;
        if(index >= m_RealOutline.size()) index -= m_RealOutline.size();
        if(prev_index >= m_RealOutline.size()) prev_index -= m_RealOutline.size();
        std::complex<double> line = m_RealOutline[index] - m_RealOutline[prev_index];
        double angle = std::arg(line);
        double abs_angle = std::fabs(angle);
        double e = 0.01745;         // ~1deg

        if(abs_angle < M_PI_2 + e && abs_angle > M_PI_2 - e)
        {
            // vertical
            bool up = angle > 0 ? true : false;

            if(one_third_rule)
            {
                if(up)
                {
                    mesh.X.insert(m_RealOutline[index].real() + rule_distance * 0.333);
                    mesh.X.insert(m_RealOutline[index].real() - rule_distance * 0.667);
                }else
                {
                    mesh.X.insert(m_RealOutline[index].real() - rule_distance * 0.333);
                    mesh.X.insert(m_RealOutline[index].real() + rule_distance * 0.667);
                }
            }
            else
            {
                mesh.X.insert(m_RealOutline[index].real());
                if(boundary_lines)
                {
                    mesh.X.insert(m_RealOutline[index].real() - rule_distance);
                    mesh.X.insert(m_RealOutline[index].real() + rule_distance);
                }
            }
        }
        else if(abs_angle < e || abs_angle > M_PI - e)
        {
            // horizontal
            bool fwd = abs_angle < e ? true : false;

            if(one_third_rule)
            {
                if(fwd)
                {
                    mesh.Y.insert(m_RealOutline[index].imag() - rule_distance * 0.333);
                    mesh.Y.insert(m_RealOutline[index].imag() + rule_distance * 0.667);
                }else
                {
                    mesh.Y.insert(m_RealOutline[index].imag() + rule_distance * 0.333);
                    mesh.Y.insert(m_RealOutline[index].imag() - rule_distance * 0.667);
                }
            }
            else
            {
                mesh.Y.insert(m_RealOutline[index].imag());
                if(boundary_lines)
                {
                    mesh.Y.insert(m_RealOutline[index].imag() - rule_distance);
                    mesh.Y.insert(m_RealOutline[index].imag() + rule_distance);
                }
            }
        }
    }

    mesh.Z.insert(m_Z);
    if(m_T != 0)
    {
        mesh.Z.insert(m_Z + m_T);
    }

    return mesh;
}

void Zone::ApproximatePolygon(std::vector<std::complex<double>>& Points, double MaxError)
{
    for(size_t i = 2; i < Points.size();)
    {
        std::complex<double> line_a = Points[i - 1] - Points[i - 2];
        std::complex<double> line_b = Points[i] - Points[i - 2];
        double angle_diff = getVectAngle(line_a, line_b);
        double error = fabs(sin(angle_diff) * fabs(line_a));
        if(error < MaxError)
        {
            // erase center point if error in allowed range
            Points.erase(Points.begin() + i - 1);
        }else
        {
            ++i;
        }
    }
}

double Zone::getVectAngle(std::complex<double> U, std::complex<double> V)
{
    double dot_prod = U.real() * V.real() + U.imag() * V.imag();
    double mag_prod = abs( U ) * abs( V );
    double angle = acos( dot_prod / mag_prod );
    double phase_diff = angle;
    return phase_diff;
}

void pems::insert_arc(std::vector<std::complex<double>>& Points, std::complex<double> Center,std::complex<double> Start, size_t Approx)
{
    complex<double> arc_vect = Start;
    Points.push_back(Center + Start);

    size_t insert_points = Approx + 1;
    for(size_t i = 0; i < insert_points; i++)
    {
        arc_vect = rot_vector(arc_vect, M_PI / insert_points);
        Points.push_back(Center + arc_vect);
    }
}

std::complex<double> pems::rot_vector(std::complex<double> Vect, double RotAngle)
{
    return std::polar(abs(Vect), arg(Vect) + RotAngle);
}

bool pems::IsClockWiseOrder(std::vector<std::complex<double>>& Data)
{
    double sum = 0;
    for(size_t i = 0; i < Data.size(); i++)
    {
        if(i + 1 == Data.size())
        {
            sum += (Data[0].real() - Data[i].real()) * (Data[0].imag() + Data[i].imag());
        }else
        {
            sum += (Data[i+1].real() - Data[i].real()) * (Data[i+1].imag() + Data[i].imag());
        }
    }
    return sum > 0;
}

double pems::round_to_n_digits(double x, size_t n)
{
    double factor = pow(10.0, n);
    return round(x * factor) / factor;
}

double pems::deg_to_radian(double degrees)
{
    return degrees * M_PI / 180;
}




//
