#include "tests.h"
#include <string_view>
#include <system_error>
#include <vector>

namespace {

    void skip_whitespace0(const std::string& json, unsigned int& i) {
        unsigned int len = json.length();
        while (i < len) {
            switch (json[i]) {
                case ' ':
                case '\t':
                case '\n':
                case '\r':
                    i++;
                    break;
                default:
                    return;
            }
        }
    }
    void skip_whitespace1(const std::string& json, unsigned int& i, const char expected) {
        unsigned int len = json.length();
        while (i < len) {
            if(json[i] == expected) {
                return;
            }
            switch (json[i]) {
                case ' ':
                case '\t':
                case '\n':
                case '\r':
                    i++;
                    break;
                default:
                    throw std::runtime_error(fmt::format("Invalid JSON format {} unexpected {}, expected {}", i, json[i], expected));
            }
        }
        throw std::runtime_error(fmt::format("Invalid JSON format {} unexpected end, expected {}", i, expected));
    }
    const char skip_whitespace2(const std::string& json, unsigned int& i, const char expected1, const char expected2) {
        unsigned int len = json.length();
        while (i < len) {
            if(json[i] == expected1) {
                return expected1;
            }
            if(json[i] == expected2) {
                return expected2;
            }
            switch (json[i]) {
                case ' ':
                case '\t':
                case '\n':
                case '\r':
                    i++;
                    break;
                default:
                    throw std::runtime_error(fmt::format("Invalid JSON format {} unexpected {}, expected {} or {}", i, json[i], expected1, expected2));
            }
        }
        throw std::runtime_error(fmt::format("Invalid JSON format {} unexpected end, expected {} or {}", i, expected1, expected2));
    }
    const char skip_whitespace3(const std::string& json, unsigned int& i, const char expected1, const char expected2, const char expected3) {
        unsigned int len = json.length();
        while (i < len) {
            if(json[i] == expected1) {
                return expected1;
            }
            if(json[i] == expected2) {
                return expected2;
            }
            if(json[i] == expected3) {
                return expected3;
            }
            switch (json[i]) {
                case ' ':
                case '\t':
                case '\n':
                case '\r':
                    i++;
                    break;
                default:
                    throw std::runtime_error(fmt::format("Invalid JSON format {} unexpected {}, expected {} or {} or {}", i, json[i], expected1, expected2, expected3));
            }
        }
        throw std::runtime_error(fmt::format("Invalid JSON format {} unexpected end, expected {} or {} or {}", i, expected1, expected2, expected3));
    }

    void check_string_part(const std::string& json, unsigned int& i, const char* expected, unsigned int expected_len) {
        if (i + expected_len + 2 >= json.length()) throw std::runtime_error(fmt::format("Invalid JSON format {} - unexpected end of data", i));
        for(int x = 0; x < expected_len; ++x) {
            if (json[++i] != expected[x]) throw std::runtime_error(fmt::format("Invalid JSON format {} {}", i, x));
        }
    }
    void check_string(const std::string& json, unsigned int& i, const char* expected, unsigned int expected_len, const char quote) {
        if (i + expected_len + 2 >= json.length()) throw std::runtime_error(fmt::format("Invalid JSON format {} - unexpected end of data", i));
        for(int x = 0; x < expected_len; ++x) {
            if (json[++i] != expected[x]) throw std::runtime_error(fmt::format("Invalid JSON format {} {}", i, x));
        }
        if (json[++i] != quote) throw std::runtime_error(fmt::format("Invalid JSON format {} {}", i, quote));
    }
    enum class UpdateItemAttributtes : int {TableName, Key, ExpressionAttributeValues, ExpressionAttributeNames, ReturnConsumedCapacity, ProjectionExpression, ConsistentRead, UpdateExpression};
    UpdateItemAttributtes get_attribute(const std::string& json, unsigned int& i, const char quote) {
        unsigned int len = json.length();
        if (i >= len) throw std::runtime_error(fmt::format("Invalid JSON format {} - unexpected end of data", i));
        switch(json[i]) {
            case 'T':
                check_string(json, i, "ableName", 8, quote);
                return UpdateItemAttributtes::TableName;
            case 'K':
                check_string(json, i, "ey", 2, quote);
                return UpdateItemAttributtes::Key;
            case 'U':
                check_string(json, i, "pdateExpression", 15, quote);
                return UpdateItemAttributtes::UpdateExpression;
            case 'E':
                check_string_part(json, i, "xpressionAttribute", 18);
                ++i;
                if (json[i] == 'V') {
                    check_string(json, i, "alues", 5, quote);
                    return UpdateItemAttributtes::ExpressionAttributeValues;
                } else if (json[i] == 'N') {
                    check_string(json, i, "ames", 4, quote);
                    return UpdateItemAttributtes::ExpressionAttributeNames;
                } else throw std::runtime_error(fmt::format("Invalid JSON format {} - unsupported attribute", i));
            case 'R':
                check_string(json, i, "eturnConsumedCapacity", 21, quote);
                return UpdateItemAttributtes::ReturnConsumedCapacity;
            case 'P':
                check_string(json, i, "rojectionExpression", 19, quote);
                return UpdateItemAttributtes::ProjectionExpression;
            case 'C':
                check_string(json, i, "onsistentRead", 13, quote);
                return UpdateItemAttributtes::ConsistentRead;
            default:
                throw std::runtime_error(fmt::format("Invalid JSON format {} - unsupported attribute", i));
        }
    }
    void skip_string(const std::string& json, unsigned int& i, const char quote) {
        unsigned int len = json.length();
        while (i < len) {
            if (json[i] == quote) {
                return;
            }
            //if (json[i] == '\\') [[unlikely]] {
            //    i += 2;
            //} else {
                i++;
            //}
        }
        throw std::runtime_error(fmt::format("Invalid JSON format {} - unexpected end of data", i));
    }   
    std::string_view get_string(const std::string& json, unsigned int& i) {
        auto q = skip_whitespace2(json, i, '"','\'');
        ++i;
        auto start = i;
        skip_string(json, i, q);
        auto end = i;
        ++i;
        return std::string_view(json.data() + start, end - start);
    }
    std::string_view get_bool(const std::string& json, unsigned int& i) {
        skip_whitespace0(json, i);
        auto start = i;
        if (json[i] == 't') {
            check_string_part(json, i, "rue", 3);
        } else if (json[i] == 'f') {
            check_string_part(json, i, "alse", 4);
        } else {
            throw std::runtime_error(fmt::format("Invalid JSON format {} - expected boolean value", i));
        }
        ++i;
        auto end = i;
        return std::string_view(json.data() + start, end - start);
    }

    void skip(const std::string& json, unsigned int& i, std::array<char, 40>& stack, unsigned int& sptr) {
        unsigned int len = json.length();
        bool is_obj = (stack[sptr] == '}');
        bool first = true;
        while (i < len) {
            skip_whitespace0(json, i);
            if (json[i] == stack[sptr]) {
                if (sptr) {
                    --sptr;
                    is_obj = (stack[sptr] == '}');
                    ++i;
                    first = false;
                } else return;
            } else {
                if (!first) {
                    if (json[i] != ',') throw std::runtime_error(fmt::format("Invalid JSON format {} - missing ',', got {}", i, json[i]));
                    ++i;
                    skip_whitespace0(json, i);
                }
                if (is_obj) {
                    auto q = json[i];
                    if(q == '"' || q == '\'') {
                        ++i;
                        skip_string(json, i, q);
                        ++i;
                        skip_whitespace1(json, i, ':');
                        ++i;
                        skip_whitespace0(json, i);
                        if (i >= len) throw std::runtime_error(fmt::format("Invalid JSON format {} - unexpected end of data", i));
                    } else throw std::runtime_error(fmt::format("Invalid JSON format {} - attribute must be string", i));
                }
                switch(json[i]) {
                    case '{':
                        ++sptr;
                        if (sptr >= stack.size()) {
                            throw std::runtime_error(fmt::format("Invalid JSON format {} - reached stack limit", i));
                        }
                        stack[sptr] = '}';
                        is_obj = true;
                        first = true;
                        ++i;
                        break;
                    case '[':
                        ++sptr;
                        if (sptr >= stack.size()) {
                            throw std::runtime_error(fmt::format("Invalid JSON format {} - reached stack limit", i));
                        }
                        stack[sptr] = ']';
                        is_obj = false;
                        first = true;
                        ++i;
                        break;
                    case '"':
                        ++i;
                        skip_string(json, i, '"');
                        ++i;
                        first = false;
                        break;
                    case '\'':
                        ++i;
                        skip_string(json, i, '\'');
                        ++i;
                        first = false;
                        break;
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                    case '0':
                    case '-':
                    case '+':
                    case '.':
                        throw std::runtime_error(fmt::format("Numbers not supported - {} at {}", json[i], i));
                        break;
                    case 'n':
                    case 't':
                    case 'f':
                    default:
                        throw std::runtime_error(fmt::format("Not supported {} at {}", json[i], i));
                };
            }
        }
        throw std::runtime_error(fmt::format("Invalid JSON format {} - unexpected end of data", i));
    }

    void skip_object(const std::string& json, unsigned int& i) {
        unsigned int sptr = 0;
        std::array<char, 40> stack;
        stack[sptr] = '}';
        ++i;
        skip(json, i, stack, sptr);
    }

    std::string_view get_object(const std::string& json, unsigned int& i) {
        skip_whitespace1(json, i, '{');
        auto start = i;
        skip_object(json, i);
        ++i;
        auto end = i;
        return std::string_view(json.data() + start, end - start);
    }

    void parse_update_item(const std::string& json, std::vector<std::string_view>& parts) {
        unsigned int i = 0;
        unsigned int required_attrs = 2;
        bool seen_attrs[static_cast<int>(UpdateItemAttributtes::UpdateExpression) + 1] = {};
        std::string_view attr_values[static_cast<int>(UpdateItemAttributtes::UpdateExpression) + 1];
        skip_whitespace1(json, i, '{');
        ++i;
        while(true) {
            auto q = skip_whitespace2(json, i, '"','\'');
            ++i;
            auto attr = get_attribute(json, i, q);
            ++i;
            if (seen_attrs[static_cast<int>(attr)]) {
                throw std::runtime_error(fmt::format("Invalid JSON format {} repeated attribute", i));
            }
            seen_attrs[static_cast<int>(attr)] = true;
            skip_whitespace1(json, i, ':');
            ++i;
            switch(attr) {
                case UpdateItemAttributtes::TableName:
                    attr_values[static_cast<int>(UpdateItemAttributtes::TableName)] = get_string(json, i);
                    --required_attrs;
                    break;
                case UpdateItemAttributtes::Key:
                    attr_values[static_cast<int>(UpdateItemAttributtes::Key)] = get_object(json, i);
                    --required_attrs;
                    break;
                case UpdateItemAttributtes::UpdateExpression:
                    attr_values[static_cast<int>(UpdateItemAttributtes::UpdateExpression)] = get_string(json, i);
                    break;
                case UpdateItemAttributtes::ExpressionAttributeValues:
                    attr_values[static_cast<int>(UpdateItemAttributtes::ExpressionAttributeValues)] = get_object(json, i);
                    break;
                case UpdateItemAttributtes::ExpressionAttributeNames:
                    attr_values[static_cast<int>(UpdateItemAttributtes::ExpressionAttributeNames)] = get_object(json, i);
                    break;
                case UpdateItemAttributtes::ReturnConsumedCapacity:
                    attr_values[static_cast<int>(UpdateItemAttributtes::ReturnConsumedCapacity)] = get_string(json, i);
                    break;
                case UpdateItemAttributtes::ProjectionExpression:
                    attr_values[static_cast<int>(UpdateItemAttributtes::ProjectionExpression)] = get_string(json, i);
                    break;
                case UpdateItemAttributtes::ConsistentRead:
                    attr_values[static_cast<int>(UpdateItemAttributtes::ConsistentRead)] = get_bool(json, i);
                    break;
            };
            if (required_attrs) {
                skip_whitespace1(json, i, ',');
            } else {
                if (skip_whitespace2(json, i, ',', '}') == '}') {
                    ++i;
                    break;
                }
            }
            ++i;
        }
        skip_whitespace0(json, i);
        if(i < json.length()) throw std::runtime_error(fmt::format("Invalid JSON format {} - unexpected data", i));
        for (int a = static_cast<int>(UpdateItemAttributtes::TableName); a <= static_cast<int>(UpdateItemAttributtes::UpdateExpression); ++a) {
            parts.push_back(attr_values[a]);
        }
    }

    unsigned int request(void* context, const std::string& json) {
        basic_cache_t<std::string>& c = *((basic_cache_t<std::string>*)context);
        parse_update_item(json, c.request_parts);
        return c.find();
    }
    ADD_METHOD("J-Custom", basic_cache_setup<std::string>, basic_cache_release<std::string>, nullptr, request);
}

namespace reference {
    unsigned int request(void* context, const std::string& json) {
        basic_cache_t<std::string>& c = *((basic_cache_t<std::string>*)context);
        c.request_parts.push_back("SET a = :val");
        return c.find();
    }
    //ADD_METHOD("J-RefSearch", basic_cache_setup<std::string>, basic_cache_release<std::string>, nullptr, request);
}