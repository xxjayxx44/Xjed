/* xmlcore
 * Copyright (c) 2018-2021 SChernykh   <https://github.com/SChernykh>
 * Copyright (c) 2016-2021 xmlcore       <https://github.com/xmlcore>, <support@xmlcore.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef xmlcore_PCITOPOLOGY_H
#define xmlcore_PCITOPOLOGY_H


#include <cstdio>


#include "base/tools/String.h"


namespace xmlcore {


class PciTopology
{
public:
    PciTopology() = default;
    PciTopology(uint32_t bus, uint32_t device, uint32_t function) : m_valid(true), m_bus(bus), m_device(device), m_function(function) {}

    inline bool isEqual(const PciTopology &other) const     { return m_valid == other.m_valid && toUint32() == other.toUint32(); }
    inline bool isValid() const                             { return m_valid; }
    inline uint8_t bus() const                              { return m_bus; }
    inline uint8_t device() const                           { return m_device; }
    inline uint8_t function() const                         { return m_function; }

    inline bool operator!=(const PciTopology &other) const  { return !isEqual(other); }
    inline bool operator<(const PciTopology &other) const   { return toUint32() < other.toUint32(); }
    inline bool operator==(const PciTopology &other) const  { return isEqual(other); }

    String toString() const
    {
        if (!isValid()) {
            return "n/a";
        }

        char *buf = new char[8]();
        snprintf(buf, 8, "%02hhx:%02hhx.%01hhx", bus(), device(), function());

        return buf;
    }

private:
    inline uint32_t toUint32() const { return m_bus << 16 | m_device << 8 | m_function;  }

    bool m_valid         = false;
    uint8_t m_bus        = 0;
    uint8_t m_device     = 0;
    uint8_t m_function   = 0;
};


} // namespace xmlcore


#endif /* xmlcore_PCITOPOLOGY_H */
