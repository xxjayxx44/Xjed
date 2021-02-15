/* xmlcore
 * Copyright (c) 2018-2021 SChernykh   <https://github.com/SChernykh>
 * Copyright (c) 2016-2021 xmlcore       <support@xmlcore.com>
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

#ifndef xmlcore_HWLOCCPUINFO_H
#define xmlcore_HWLOCCPUINFO_H


#include "backend/cpu/platform/BasicCpuInfo.h"
#include "base/tools/Object.h"


using hwloc_const_bitmap_t  = const struct hwloc_bitmap_s *;
using hwloc_obj_t           = struct hwloc_obj *;
using hwloc_topology_t      = struct hwloc_topology *;


namespace xmlcore {


class HwlocCpuInfo : public BasicCpuInfo
{
public:
    xmlcore_DISABLE_COPY_MOVE(HwlocCpuInfo)


    enum Feature : uint32_t {
        SET_THISTHREAD_MEMBIND = 1
    };


    HwlocCpuInfo();
    ~HwlocCpuInfo() override;

    static inline bool hasFeature(Feature feature)              { return m_features & feature; }

    inline const std::vector<uint32_t> &nodeset() const         { return m_nodeset; }
    inline hwloc_topology_t topology() const                    { return m_topology; }

    bool membind(hwloc_const_bitmap_t nodeset);

protected:
    CpuThreads threads(const Algorithm &algorithm, uint32_t limit) const override;

    inline const char *backend() const override     { return m_backend; }
    inline size_t cores() const override            { return m_cores; }
    inline size_t L2() const override               { return m_cache[2]; }
    inline size_t L3() const override               { return m_cache[3]; }
    inline size_t nodes() const override            { return m_nodes; }
    inline size_t packages() const override         { return m_packages; }

private:
    CpuThreads allThreads(const Algorithm &algorithm, uint32_t limit) const;
    void processTopLevelCache(hwloc_obj_t obj, const Algorithm &algorithm, CpuThreads &threads, size_t limit) const;
    void setThreads(size_t threads);

    static uint32_t m_features;

    char m_backend[20]          = { 0 };
    hwloc_topology_t m_topology = nullptr;
    size_t m_cache[5]           = { 0 };
    size_t m_cores              = 0;
    size_t m_nodes              = 0;
    size_t m_packages           = 0;
    std::vector<uint32_t> m_nodeset;
};


} /* namespace xmlcore */


#endif /* xmlcore_HWLOCCPUINFO_H */
