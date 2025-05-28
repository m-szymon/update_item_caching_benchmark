#include "tests.h"
#include <memory>
#include <re2/set.h>
#include <vector>

namespace {
    RE2::Options make_options() {
        RE2::Options options(RE2::DefaultOptions);
        options.set_dot_nl(true);
        options.set_max_mem(options.max_mem() * 2);
        return options;
    }
    RE2::Set make_set() {
        RE2::Set regex_set(make_options(), RE2::UNANCHORED);
        return regex_set;
    }
    struct context_t {
        bool built = false;
        RE2::Set regex_set;
        context_t(const request_pattern_ref_t&) : regex_set(make_set()){
        }
    };

    bool build(void* context) {
        built_context_t<context_t>& c = *(built_context_t<context_t>*)context;
        c.built_cache.regex_set = make_set();
        for(int x = 0; x < c.patterns[CACHE_REGEX].size(); x++) {
            c.built_cache.regex_set.Add(c.patterns[CACHE_REGEX][x].get(), NULL);
        }
        c.built_cache.built = c.built_cache.regex_set.Compile();
        return c.built_cache.built;
    }

    unsigned int request(void* context, const std::string& json) {
        std::vector<int> matching_rules;
        auto json_view = std::string_view(json);
        auto& c = ((built_context_t<context_t>*)context)->built_cache;
        if (c.built && c.regex_set.Match(json_view, &matching_rules)) {
            return matching_rules[0];
        }
        return CACHE_NOT_FOUND;
    }

    struct capture_context : public context_t {
        std::vector<std::shared_ptr<RE2>> compiled_capture_regexes;
        std::vector<re2::StringPiece> results;
        std::vector<RE2::Arg> arguments;
        std::vector<RE2::Arg*> arguments_ptrs;
        capture_context(const request_pattern_ref_t& patterns) : context_t(patterns) {
            compiled_capture_regexes.reserve(patterns[CACHE_CAPTURE].size());
        }
    };
    bool build_capture(void* context) {
        built_context_t<capture_context>& c = *(built_context_t<capture_context>*)context;
        c.built_cache.compiled_capture_regexes.clear();
        std::size_t args_count = 0;
        for (int x = 0; x < c.patterns[CACHE_CAPTURE].size(); x++) {
            c.built_cache.compiled_capture_regexes.emplace_back(std::make_shared<RE2>(c.patterns[CACHE_CAPTURE][x].get(), make_options()));
            if (!c.built_cache.compiled_capture_regexes.back()->ok()) {
                return false;
            }
            std::size_t new_args_count = c.built_cache.compiled_capture_regexes.back()->NumberOfCapturingGroups();
            if (new_args_count > args_count) {
                args_count = new_args_count;
            }
        }
        /// Adjust vectors sizes.
        c.built_cache.arguments.resize(args_count);
        c.built_cache.arguments_ptrs.resize(args_count);
        c.built_cache.results.resize(args_count);
        for (std::size_t i = 0; i < args_count; ++i) {
            c.built_cache.arguments[i] = &c.built_cache.results[i];
            c.built_cache.arguments_ptrs[i] = &c.built_cache.arguments[i];
        }
        return build(context);
    }

    unsigned int capture(void* context, const std::string& json) {
        built_context_t<capture_context>& c = *(built_context_t<capture_context>*)context;
        auto match = request(context, json);
        if (match != CACHE_NOT_FOUND) {
            re2::StringPiece piece(json);
            RE2::FindAndConsumeN(&piece, *(c.built_cache.compiled_capture_regexes[match]), c.built_cache.arguments_ptrs.data(), c.built_cache.compiled_capture_regexes[match]->NumberOfCapturingGroups());
            /*
            for (size_t i = 0; i < c.built_cache.results.size(); ++i) {
                std::cout << "Capture " << i << ": " << c.built_cache.results[i] << "\n";
            }
            */
        }
        return match;
    }

    namespace v1 {
        ADD_METHOD("R-re2", setup<context_t>, release<context_t>, build, request);
    }
    namespace v2 {
        ADD_METHOD("R-re2_cap", setup<capture_context>, release<capture_context>, build_capture, capture);
    }
}