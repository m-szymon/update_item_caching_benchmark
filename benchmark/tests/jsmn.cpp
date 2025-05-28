#include "tests.h"
#include <jsmn.h>
#include <string_view>

namespace {

    static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
        if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
            strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
            return 0;
        }
        return -1;
    }

    unsigned int request(void* context, const std::string& json) {
        jsmn_parser p;
        jsmntok_t t[256]; /* We expect no more than 128 JSON tokens */

        jsmn_init(&p);
        int r = jsmn_parse(&p, json.c_str(), json.length(), t, 256); // "s" is the char array holding the json content

        basic_cache_t<std::string>& c = *((basic_cache_t<std::string>*)context);
        for (int i = 1; i < r; ++i) {
            if (jsoneq(json.c_str(), &t[i], "UpdateExpression") == 0) {
                c.request_parts.push_back(std::string_view(json.c_str() + t[i + 1].start, t[i + 1].end - t[i + 1].start));
                break;
                //i++;
            }
        }
        return c.find();
    }
    ADD_METHOD("J-jsmn", basic_cache_setup<std::string>, basic_cache_release<std::string>, nullptr, request);
}