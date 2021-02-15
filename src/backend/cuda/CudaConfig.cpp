/* xmlcore
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018-2020 SChernykh   <https://github.com/SChernykh>
 * Copyright 2016-2020 xmlcore       <https://github.com/xmlcore>, <support@xmlcore.com>
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


#include "backend/cuda/CudaConfig.h"
#include "3rdparty/rapidjson/document.h"
#include "backend/common/Tags.h"
#include "backend/cuda/CudaConfig_gen.h"
#include "backend/cuda/wrappers/CudaLib.h"
#include "base/io/json/Json.h"
#include "base/io/log/Log.h"


namespace xmlcore {


static bool generated           = false;
static const char *kBfactorHint = "bfactor-hint";
static const char *kBsleepHint  = "bsleep-hint";
static const char *kDevicesHint = "devices-hint";
static const char *kEnabled     = "enabled";
static const char *kLoader      = "loader";

#ifdef xmlcore_FEATURE_NVML
static const char *kNvml        = "nvml";
#endif


extern template class Threads<CudaThreads>;


}


rapidjson::Value xmlcore::CudaConfig::toJSON(rapidjson::Document &doc) const
{
    using namespace rapidjson;
    auto &allocator = doc.GetAllocator();

    Value obj(kObjectType);

    obj.AddMember(StringRef(kEnabled),  m_enabled, allocator);
    obj.AddMember(StringRef(kLoader),   m_loader.toJSON(), allocator);

#   ifdef xmlcore_FEATURE_NVML
    if (m_nvmlLoader.isNull()) {
        obj.AddMember(StringRef(kNvml), m_nvml, allocator);
    }
    else {
        obj.AddMember(StringRef(kNvml), m_nvmlLoader.toJSON(), allocator);
    }
#   endif

    m_threads.toJSON(obj, doc);

    return obj;
}


std::vector<xmlcore::CudaLaunchData> xmlcore::CudaConfig::get(const Miner *miner, const Algorithm &algorithm, const std::vector<CudaDevice> &devices) const
{
    auto deviceIndex = [&devices](uint32_t index) -> int {
        for (uint32_t i = 0; i < devices.size(); ++i) {
            if (devices[i].index() == index) {
                return i;
            }
        }

        return -1;
    };

    std::vector<CudaLaunchData> out;
    const auto &threads = m_threads.get(algorithm);

    if (threads.isEmpty()) {
        return out;
    }

    out.reserve(threads.count());

    for (const auto &thread : threads.data()) {
        const int index = deviceIndex(thread.index());
        if (index == -1) {
            LOG_INFO("%s" YELLOW(" skip non-existing device with index ") YELLOW_BOLD("%u"), cuda_tag(), thread.index());
            continue;
        }

        out.emplace_back(miner, algorithm, thread, devices[static_cast<size_t>(index)]);
    }

    return out;
}


void xmlcore::CudaConfig::read(const rapidjson::Value &value)
{
    if (value.IsObject()) {
        m_enabled   = Json::getBool(value, kEnabled, m_enabled);
        m_loader    = Json::getString(value, kLoader);
        m_bfactor   = std::min(Json::getUint(value, kBfactorHint, m_bfactor), 12u);
        m_bsleep    = Json::getUint(value, kBsleepHint, m_bsleep);

        setDevicesHint(Json::getString(value, kDevicesHint));

#       ifdef xmlcore_FEATURE_NVML
        auto &nvml = Json::getValue(value, kNvml);
        if (nvml.IsString()) {
            m_nvmlLoader = nvml.GetString();
        }
        else if (nvml.IsBool()) {
            m_nvml = nvml.GetBool();
        }
#       endif

        m_threads.read(value);

        generate();
    }
    else if (value.IsBool()) {
        m_enabled = value.GetBool();

        generate();
    }
    else {
        m_shouldSave = true;

        generate();
    }
}


void xmlcore::CudaConfig::generate()
{
    if (generated) {
        return;
    }

    if (!isEnabled() || m_threads.has("*")) {
        return;
    }

    if (!CudaLib::init(loader())) {
        return;
    }

    if (!CudaLib::runtimeVersion() || !CudaLib::driverVersion() || !CudaLib::deviceCount()) {
        return;
    }

    const auto devices = CudaLib::devices(bfactor(), bsleep(), m_devicesHint);
    if (devices.empty()) {
        return;
    }

    size_t count = 0;

    count += xmlcore::generate<Algorithm::CN>(m_threads, devices);
    count += xmlcore::generate<Algorithm::CN_LITE>(m_threads, devices);
    count += xmlcore::generate<Algorithm::CN_HEAVY>(m_threads, devices);
    count += xmlcore::generate<Algorithm::CN_PICO>(m_threads, devices);
    count += xmlcore::generate<Algorithm::RANDOM_X>(m_threads, devices);
    count += xmlcore::generate<Algorithm::ASTROBWT>(m_threads, devices);
    count += xmlcore::generate<Algorithm::KAWPOW>(m_threads, devices);

    generated    = true;
    m_shouldSave = count > 0;
}


void xmlcore::CudaConfig::setDevicesHint(const char *devicesHint)
{
    if (devicesHint == nullptr) {
        return;
    }

    const auto indexes = String(devicesHint).split(',');
    m_devicesHint.reserve(indexes.size());

    for (const auto &index : indexes) {
        m_devicesHint.push_back(strtoul(index, nullptr, 10));
    }
}
