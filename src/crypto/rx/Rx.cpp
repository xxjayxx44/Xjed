/* xmlcore
 * Copyright (c) 2018-2019 tevador     <tevador@gmail.com>
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


#include "crypto/rx/Rx.h"
#include "backend/cpu/CpuConfig.h"
#include "backend/cpu/CpuThreads.h"
#include "crypto/rx/RxConfig.h"
#include "crypto/rx/RxQueue.h"
#include "crypto/randomx/randomx.h"
#include "crypto/randomx/aes_hash.hpp"


#ifdef xmlcore_FEATURE_MSR
#   include "crypto/rx/RxFix.h"
#   include "crypto/rx/RxMsr.h"
#endif


namespace xmlcore {


class RxPrivate;


static bool osInitialized   = false;
static RxPrivate *d_ptr     = nullptr;


class RxPrivate
{
public:
    inline RxPrivate(IRxListener *listener) : queue(listener) {}

    RxQueue queue;
};


} // namespace xmlcore


xmlcore::HugePagesInfo xmlcore::Rx::hugePages()
{
    return d_ptr->queue.hugePages();
}


xmlcore::RxDataset *xmlcore::Rx::dataset(const Job &job, uint32_t nodeId)
{
    return d_ptr->queue.dataset(job, nodeId);
}


void xmlcore::Rx::destroy()
{
#   ifdef xmlcore_FEATURE_MSR
    RxMsr::destroy();
#   endif

    delete d_ptr;

    d_ptr = nullptr;
}


void xmlcore::Rx::init(IRxListener *listener)
{
    d_ptr = new RxPrivate(listener);
}


template<typename T>
bool xmlcore::Rx::init(const T &seed, const RxConfig &config, const CpuConfig &cpu)
{
    const Algorithm::Family f = seed.algorithm().family();
    if ((f != Algorithm::RANDOM_X)
#       ifdef xmlcore_ALGO_CN_HEAVY
        && (f != Algorithm::CN_HEAVY)
#       endif
        ) {
#       ifdef xmlcore_FEATURE_MSR
        RxMsr::destroy();
#       endif

        return true;
    }

#   ifdef xmlcore_FEATURE_MSR
    if (!RxMsr::isInitialized()) {
        RxMsr::init(config, cpu.threads().get(seed.algorithm()).data());
    }
#   endif

#   ifdef xmlcore_ALGO_CN_HEAVY
    if (f == Algorithm::CN_HEAVY) {
        return true;
    }
#   endif

    randomx_set_scratchpad_prefetch_mode(config.scratchpadPrefetchMode());
    randomx_set_huge_pages_jit(cpu.isHugePagesJit());
    randomx_set_optimized_dataset_init(config.initDatasetAVX2());

    if (!osInitialized) {
#       ifdef xmlcore_FIX_RYZEN
        RxFix::setupMainLoopExceptionFrame();
#       endif

        if (!cpu.isHwAES()) {
            SelectSoftAESImpl(cpu.threads().get(seed.algorithm()).count());
        }
        osInitialized = true;
    }

    if (isReady(seed)) {
        return true;
    }

    d_ptr->queue.enqueue(seed, config.nodeset(), config.threads(cpu.limit()), cpu.isHugePages(), config.isOneGbPages(), config.mode(), cpu.priority());

    return false;
}


template<typename T>
bool xmlcore::Rx::isReady(const T &seed)
{
    return d_ptr->queue.isReady(seed);
}


#ifdef xmlcore_FEATURE_MSR
bool xmlcore::Rx::isMSR()
{
    return RxMsr::isEnabled();
}
#endif


namespace xmlcore {


template bool Rx::init(const RxSeed &seed, const RxConfig &config, const CpuConfig &cpu);
template bool Rx::isReady(const RxSeed &seed);
template bool Rx::init(const Job &seed, const RxConfig &config, const CpuConfig &cpu);
template bool Rx::isReady(const Job &seed);


} // namespace xmlcore
