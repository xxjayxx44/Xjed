/* xmlcore
 * Copyright (c) 2014-2019 heapwolf    <https://github.com/heapwolf>
 * Copyright (c) 2018-2020 SChernykh   <https://github.com/SChernykh>
 * Copyright (c) 2016-2020 xmlcore       <https://github.com/xmlcore>, <support@xmlcore.com>
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


#include "base/net/http/HttpData.h"
#include "3rdparty/http-parser/http_parser.h"
#include "3rdparty/rapidjson/document.h"
#include "3rdparty/rapidjson/error/en.h"
#include "base/io/json/Json.h"


#include <uv.h>
#include <stdexcept>


namespace xmlcore {


const std::string HttpData::kApplicationJson    = "application/json";
const std::string HttpData::kContentType        = "Content-Type";
const std::string HttpData::kContentTypeL       = "content-type";
const std::string HttpData::kTextPlain          = "text/plain";


} // namespace xmlcore


bool xmlcore::HttpData::isJSON() const
{
    if (!headers.count(kContentTypeL)) {
        return false;
    }

    auto &type = headers.at(kContentTypeL);

    return type == kApplicationJson || type == kTextPlain;
}


const char *xmlcore::HttpData::methodName() const
{
    return http_method_str(static_cast<http_method>(method));
}


const char *xmlcore::HttpData::statusName() const
{
    if (status < 0) {
        return uv_strerror(status);
    }

    return http_status_str(static_cast<http_status>(status));
}


rapidjson::Document xmlcore::HttpData::json() const
{
    if (status < 0) {
        throw std::runtime_error(statusName());
    }

    if (!isJSON()) {
        throw std::runtime_error("the response is not a valid JSON response");
    }

    using namespace rapidjson;
    Document doc;
    if (doc.Parse(body.c_str()).HasParseError()) {
        throw std::runtime_error(GetParseError_En(doc.GetParseError()));
    }

    if (doc.IsObject() && !doc.ObjectEmpty()) {
        const char *error = Json::getString(doc, "error");
        if (error) {
            throw std::runtime_error(error);
        }
    }

    return doc;
}
