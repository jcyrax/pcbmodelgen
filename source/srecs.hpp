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

#ifndef srecs_h
#define srecs_h

#include <vector>
#include <string>

namespace kicad_to_ems
{
namespace srecs
{

typedef std::vector<char> charvec_t;

class SREC
{
public:
    SREC(charvec_t::iterator Begin, charvec_t::iterator End)
        : m_Begin(Begin), m_Current(Begin), m_End(End), m_FirstCall(true)
    {}

    void SetPosition(charvec_t::iterator& Position);

    bool GetNext();
    bool GetNext(const char* Name);
    bool GetChild();
    bool GetChild(const char* Name);

    std::string GetRecord();
    std::string GetRecName();
    charvec_t::iterator GetPosition();
    bool IsEnd();

private:
    charvec_t::iterator m_Begin;
    charvec_t::iterator m_Current;
    charvec_t::iterator m_End;
    bool m_FirstCall;

    bool FindRecStart();
    void FindRecEnd();
    bool FindChildRec();
    void EnterRecord();
    std::string GetCurString();
};

} // namespace srecs
} // namespace kicad_to_ems

#endif // srecs_h
