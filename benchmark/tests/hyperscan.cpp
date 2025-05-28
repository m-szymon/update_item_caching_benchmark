#include "tests.h"
#include <hs.h>
#include <ch.h>

namespace bench_hyperscan {

    struct context_t {
        hs_database_t *database = nullptr;
        hs_scratch_t *scratch = nullptr;
        const char** expressions;
        unsigned int* flags;
        unsigned int* ids;
        void clean() {
            if(scratch != nullptr) {
                hs_free_scratch(scratch);
                scratch = nullptr;
            }
            if(database != nullptr) {
                hs_free_database(database);
                database = nullptr;
            }
        }
        context_t(const request_pattern_ref_t& patterns){
            expressions = new const char*[patterns[CACHE_REGEX].size()];
            flags = new unsigned int [patterns[CACHE_REGEX].size()];
            ids = new unsigned int [patterns[CACHE_REGEX].size()];
            for(int i = 0; i < patterns[CACHE_REGEX].size(); i++) {
                expressions[i] = patterns[CACHE_REGEX][i].get().c_str();
                ids[i] = i;
                flags[i] = HS_FLAG_DOTALL | HS_CPU_FEATURES_AVX512VBMI;// | HS_FLAG_SINGLEMATCH;
            }
        }
        ~context_t(){
            clean();
            delete[] expressions;
            delete[] flags;
        }
    };

    bool build(void* context) {
        built_context_t<context_t>& c = *(built_context_t<context_t>*)context;
        c.built_cache.clean();
        hs_compile_error_t *compile_err;
        if (hs_compile_multi(c.built_cache.expressions, c.built_cache.flags, c.built_cache.ids, c.patterns[CACHE_REGEX].size(), HS_MODE_BLOCK, NULL, &c.built_cache.database, &compile_err) != HS_SUCCESS) {
            fprintf(stderr, "ERROR: Unable to compile pattern: %s\n", compile_err->message);
            hs_free_compile_error(compile_err);
            return false;
        }
        hs_alloc_scratch(c.built_cache.database, &c.built_cache.scratch);
        return true;
    }

    int match_event_handler(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags, void *context) {
        *((unsigned int*)context) = id;
        //printf("Match %u at %llu to %llu\n", id, from, to);
        return 1;
    }

    unsigned int request(void* context, const std::string& json) {
        context_t& c = ((built_context_t<context_t>*)context)->built_cache;
        unsigned int ret = CACHE_NOT_FOUND;
        hs_scan(c.database, json.c_str(), json.length(), 0, c.scratch, match_event_handler, &ret);
        return ret;
    }

    ADD_METHOD("R-hyperscan", setup<context_t>, release<context_t>, build, request);
}
namespace bench_chimera {

    struct context_t {
        ch_database_t *database = nullptr;
        ch_scratch_t *scratch = nullptr;
        const char** expressions;
        unsigned int* flags;
        unsigned int* ids;
        void clean() {
            if(scratch != nullptr) {
                ch_free_scratch(scratch);
                scratch = nullptr;
            }
            if(database != nullptr) {
                ch_free_database(database);
                database = nullptr;
            }
        }
        context_t(const request_pattern_ref_t& patterns){
            expressions = new const char*[patterns[CACHE_CAPTURE].size()];
            flags = new unsigned int [patterns[CACHE_CAPTURE].size()];
            ids = new unsigned int [patterns[CACHE_CAPTURE].size()];
            for(int i = 0; i < patterns[CACHE_CAPTURE].size(); i++) {
                expressions[i] = patterns[CACHE_CAPTURE][i].get().c_str();
                ids[i] = i;
                flags[i] = CH_FLAG_DOTALL;// | CH_FLAG_SINGLEMATCH;
            }
        }
        ~context_t(){
            clean();
            delete[] expressions;
            delete[] flags;
        }
    };

    bool build(void* context) {
        built_context_t<context_t>& c = *(built_context_t<context_t>*)context;
        c.built_cache.clean();
        ch_compile_error_t *compile_err;
        if (ch_compile_multi(c.built_cache.expressions, c.built_cache.flags, c.built_cache.ids, c.patterns[CACHE_CAPTURE].size(), CH_MODE_GROUPS, NULL, &c.built_cache.database, &compile_err) != CH_SUCCESS) {
            fprintf(stderr, "ERROR: Unable to compile pattern: %s\n", compile_err->message);
            ch_free_compile_error(compile_err);
            return false;
        }
        ch_alloc_scratch(c.built_cache.database, &c.built_cache.scratch);
        return true;
    }

    int match_event_handler(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags, unsigned int size, const ch_capture_t *captured, void *context) {
        *((unsigned int*)context) = id;
        //printf("Match %u at %llu to %llu\n", id, from, to);
        return CH_CALLBACK_TERMINATE;
    }

    unsigned int request(void* context, const std::string& json) {
        context_t& c = ((built_context_t<context_t>*)context)->built_cache;
        unsigned int ret = CACHE_NOT_FOUND;
        ch_scan(c.database, json.c_str(), json.length(), 0, c.scratch, match_event_handler, NULL, &ret);
        return ret;
    }

    ADD_METHOD("R-chimera", setup<context_t>, release<context_t>, build, request);
}