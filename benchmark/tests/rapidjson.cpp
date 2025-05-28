#include "tests.h"
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <vector>

namespace {

    std::string_view extract_string(std::vector<rapidjson::StringBuffer>& extractions, rapidjson::Value& keyObject) {
        extractions.emplace_back();
        rapidjson::StringBuffer& buffer = extractions.back();
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        keyObject.Accept(writer);
        return buffer.GetString();
    }

    unsigned int request(void* context, const std::string& json) {
        rapidjson::Document doc;
        doc.Parse(json.c_str());
        basic_cache_t<std::string>& c = *((basic_cache_t<std::string>*)context);
        c.request_parts.push_back(doc["TableName"].GetString());
        std::vector<rapidjson::StringBuffer> extractions;
        c.request_parts.push_back(extract_string(extractions, doc["Key"]));
        for (auto& member : doc["ExpressionAttributeValues"].GetObject()) {
            c.request_parts.push_back(extract_string(extractions, member.value));
        }

        c.request_parts.push_back(doc["UpdateExpression"].GetString());
        return c.find();
    }

    unsigned int request2(void* context, const std::string& json) {
        typedef rapidjson::GenericDocument<rapidjson::UTF8<>, rapidjson::MemoryPoolAllocator<>, rapidjson::MemoryPoolAllocator<>> DocumentType;
        char valueBuffer[4*4096];
        char parseBuffer[4*1024];
        rapidjson::MemoryPoolAllocator<> valueAllocator(valueBuffer, sizeof(valueBuffer));
        rapidjson::MemoryPoolAllocator<> parseAllocator(parseBuffer, sizeof(parseBuffer));
        DocumentType doc(&valueAllocator, sizeof(parseBuffer), &parseAllocator);
        doc.Parse(json.c_str());
        basic_cache_t<std::string>& c = *((basic_cache_t<std::string>*)context);
        c.request_parts.push_back(doc["TableName"].GetString());
        std::vector<rapidjson::StringBuffer> extractions;
        c.request_parts.push_back(extract_string(extractions, doc["Key"]));
        for (auto& member : doc["ExpressionAttributeValues"].GetObject()) {
            c.request_parts.push_back(extract_string(extractions, member.value));
        }

        c.request_parts.push_back(doc["UpdateExpression"].GetString());
        return c.find();
    }
namespace v1 {
    ADD_METHOD("J-RapidJSON", basic_cache_setup<std::string>, basic_cache_release<std::string>, nullptr, request);
}
namespace v2 {
    ADD_METHOD("J-RJSON-buf", basic_cache_setup<std::string>, basic_cache_release<std::string>, nullptr, request2);
}
}
