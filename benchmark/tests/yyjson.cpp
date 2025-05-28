#include "tests.h"
#include <yyjson.h>

namespace {
    unsigned int request(void* context, const std::string& json) {
        // Read JSON and get root
        yyjson_doc *doc = yyjson_read(json.c_str(), json.length(), 0);
        yyjson_val *root = yyjson_doc_get_root(doc);

        basic_cache_t<std::string>& c = *((basic_cache_t<std::string>*)context);
        c.request_parts.push_back(yyjson_get_str(yyjson_obj_get(root, "TableName")));
        auto s = yyjson_val_write(yyjson_obj_get(root, "Key"), 0, nullptr);
        c.request_parts.push_back(s);
        free(s);
        yyjson_val* obj = yyjson_obj_get(root, "ExpressionAttributeValues");
        size_t idx, max;
        yyjson_val *key, *val;
        yyjson_obj_foreach(obj, idx, max, key, val) {
            auto s = yyjson_val_write(val, 0, nullptr);
            c.request_parts.push_back(s);
            free(s);
        }
        c.request_parts.push_back(yyjson_get_str(yyjson_obj_get(root, "UpdateExpression")));
        // Free the doc
        yyjson_doc_free(doc);

        return c.find();
    }
    ADD_METHOD("J-yyjson", basic_cache_setup<std::string>, basic_cache_release<std::string>, nullptr, request);
}