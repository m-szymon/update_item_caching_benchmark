#include "tests.h"
#include <simdjson.h>

namespace {
    unsigned int request(void* context, const std::string& json) {
        simdjson::ondemand::parser parser;
        simdjson::ondemand::document doc = parser.iterate(json.c_str(), json.length(), json.capacity());

        simdjson::ondemand::object obj = doc.get_object();
        basic_cache_t<std::string>& c = *((basic_cache_t<std::string>*)context);
        c.request_parts.push_back(obj["TableName"].get_string().value());
        c.request_parts.push_back(simdjson::to_json_string(obj["Key"]).value());
        for(auto field : obj["ExpressionAttributeValues"].get_object()) {
            c.request_parts.push_back(simdjson::to_json_string(field.value()).value());
        }
        c.request_parts.push_back(obj["UpdateExpression"].get_string().value());
        return c.find();
    }
    ADD_METHOD("J-simdjson", basic_cache_setup<std::string>, basic_cache_release<std::string>, nullptr, request);
}