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

#include "srecs.hpp"

#include <cctype>
#include <vector>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <exception>
#include <system_error>

using namespace kicad_to_ems::srecs;

class srec_exc : public std::runtime_error
{
public:
    srec_exc(const char* Msg) : std::runtime_error(Msg) {}
};


void SREC::SetPosition(charvec_t::iterator& Position)
{
    m_Current = Position;
}

std::string SREC::GetRecord()
{
    charvec_t::iterator rec_start = GetPosition();
    FindRecEnd();
    charvec_t::iterator rec_end = GetPosition();
    if(rec_end != m_End) rec_end++;
    SetPosition(rec_start);
    std::string record(rec_start, rec_end);
    return record;
}

bool SREC::GetNext()
{
    if(m_Current == m_Begin && m_FirstCall)
    {
        m_FirstCall = false;
        if(!FindRecStart()) return false;
    }else
    {
        FindRecEnd();
        if(!FindRecStart()) return false;
    }

    if(IsEnd())
    {
        return false;
    }
    return true;
}

bool SREC::GetNext(const char* Name)
{
    charvec_t::iterator pos = GetPosition();
    while(GetNext())
    {
        std::string rec_name = GetRecName();
        if(rec_name.compare(Name) == 0)
        {
            return true;
        }
    }
    //reset position
    SetPosition(pos);
    return false;
}

bool SREC::GetChild()
{
    return FindChildRec();
}

bool SREC::GetChild(const char* Name)
{
    charvec_t::iterator pos = GetPosition();
    if(GetChild())
    {
        std::string name = GetRecName();
        if(name.compare(Name) == 0)
        {
            return true;
        }
        if(GetNext(Name))
        {
            return true;
        }
    }
    //reset position
    SetPosition(pos);
    return false;
}













std::string SREC::GetRecName()
{
    charvec_t::iterator cur_pos = GetPosition();
    EnterRecord();
    if(IsEnd()) throw srec_exc("Record name read error");
    std::string str = GetCurString();
    SetPosition(cur_pos);
    return str;
}

bool SREC::IsEnd()
{
    if(m_Current == m_End)
    {
        return true;
    }
    return false;
}

charvec_t::iterator SREC::GetPosition()
{
    return m_Current;
}

bool SREC::FindRecStart()
{
    bool first = true;
    while(m_Current != m_End)
    {
        if(m_Current != m_Begin || !first)
        {
            m_Current++;
        }
        char c = *m_Current;
        if(c == '(')
        {
            return true;
        }
        if(c == ')')
        {
            break;
        }
        first = false;
    }
    return false;
}

bool SREC::FindChildRec()
{
    charvec_t::iterator it = m_Current;

    bool quoted = false;
    size_t skip_n = 0;
    while(m_Current != m_End)
    {
        m_Current++;
        char c = *m_Current;
        if(c == '\\')
        {
            skip_n = 2;
        }
        if(skip_n == 0)
        {
            if(c == '"')
            {
                quoted = !quoted;
            }
            if(!quoted)
            {
                if(c == '(')
                {
                    return true;
                }
                if(c == ')')
                {
                    m_Current = it;
                    break;
                }
            }
        }
        if(skip_n > 0) skip_n--;
    }
    return false;
}

void SREC::FindRecEnd()
{
    size_t depth = 1;
    bool quoted = false;
    size_t skip_n = 0;
    while(m_Current != m_End)
    {
        m_Current++;
        char c = *m_Current;
        if(c == '\\')
        {
            skip_n = 2;
        }
        if(skip_n == 0)
        {
            if(c == '"')
            {
                quoted = !quoted;
            }
            if(!quoted)
            {
                if(c == '(')
                {
                    depth++;
                }
                if(c == ')')
                {
                    if(depth > 0) depth--;
                    if(depth == 0)
                    {
                        break;
                    }
                }
            }
        }
        if(skip_n > 0) skip_n--;
    }
}

void SREC::EnterRecord()
{
    if(m_Current != m_End)
    {
        m_Current++;
    }
}

std::string SREC::GetCurString()
{
    std::string str;
    while(m_Current != m_End)
    {
        char c = *m_Current;
        if(c == ')' || c == ' ' || iscntrl(c) || c == '(')    // or control chars
        {
            break;
        }
        str.push_back(c);
        m_Current++;
    }
    return str;
}




