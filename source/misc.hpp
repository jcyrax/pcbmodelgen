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

#ifndef misc_hpp
#define misc_hpp

#include <algorithm>
#include <complex>

namespace kicad_to_ems
{
namespace pems
{

class Misc
{
public:
    static bool lines_intersect(std::complex<double> L1_a,
                                std::complex<double> L1_b,
                                std::complex<double> L2_a,
                                std::complex<double> L2_b)
    {
        std::complex<double> vect1, vect2;

        vect1 = L1_b - L1_a;
        vect2 = L2_b - L2_a;

        // get line equations Y = N * X + C
        double C1, C2, N1, N2;

        N1 = tan(std::arg(vect1));
        N2 = tan(std::arg(vect2));

        C1 = L1_a.imag() - (N1 * L1_a.real());
        C2 = L2_a.imag() - (N2 * L2_a.real());

        // find intersection
        double X_intersect = (C2 - C1) / (N1 - N2);
        double Y_intersect = N1 * X_intersect + C1;

        bool l1_hor = false;
        bool l1_vert = false;
        bool l2_hor = false;
        bool l2_vert = false;

        if (L1_a.real() == L1_b.real())
            l1_vert = true;
        if (L2_a.real() == L2_b.real())
            l2_vert = true;
        if (L1_a.imag() == L1_b.imag())
            l1_hor = true;
        if (L2_a.imag() == L2_b.imag())
            l2_hor = true;

        // check if in line range

        bool l1_x_in_range = (l1_vert && fabs(X_intersect - L1_a.real()) < 1e-8) ||
                             (X_intersect >= std::min(L1_a.real(), L1_b.real()) &&
                              X_intersect <= std::max(L1_a.real(), L1_b.real()));

        bool l2_x_in_range = (l2_vert && fabs(X_intersect - L2_a.real()) < 1e-8) ||
                             (X_intersect >= std::min(L2_a.real(), L2_b.real()) &&
                              X_intersect <= std::max(L2_a.real(), L2_b.real()));

        bool l1_y_in_range = (l1_hor && fabs(Y_intersect - L1_a.imag()) < 1e-8) ||
                             (Y_intersect >= std::min(L1_a.imag(), L1_b.imag()) &&
                              Y_intersect <= std::max(L1_a.imag(), L1_b.imag()));

        bool l2_y_in_range = (l2_hor && fabs(Y_intersect - L2_a.imag()) < 1e-8) ||
                             (Y_intersect >= std::min(L2_a.imag(), L2_b.imag()) &&
                              Y_intersect <= std::max(L2_a.imag(), L2_b.imag()));

        return l1_x_in_range && l2_x_in_range && l1_y_in_range && l2_y_in_range;
    }
};

} // namespace pems
} // namespace kicad_to_ems

#endif // misc_hpp
