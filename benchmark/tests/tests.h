#include <string>
#include <chrono>
#include <atomic>
#include <functional>
#include <thread>
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <rapidjson/document.h>
#include <fmt/printf.h>

template<typename T>
void runTimer(int seconds, std::atomic<bool>& timerExpired) {
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    timerExpired.store(true, std::memory_order_release);
}

template<typename T>
unsigned long long measure(const std::string& name, int testTime, T f, const std::string& csv_prefix = "") {
    std::atomic<bool> timerExpired(false);
    unsigned long long count_requests = 0;
    unsigned long long count_results = 0;
    std::thread timerThread(runTimer<T>, testTime, std::ref(timerExpired));
    auto beg = std::chrono::high_resolution_clock::now();

    while (!timerExpired.load(std::memory_order_acquire)
            && f(count_requests, count_results)) {
        count_requests++;
    }
 
    auto end = std::chrono::high_resolution_clock::now();
    timerThread.join();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - beg);
    if(csv_prefix.empty()) {
        fmt::print("{:>17}: {:>10}x /{}s ({})\n", name, count_requests, duration.count()/1000000.0, count_results);
    } else {
        fmt::print("{},{},{},{},{}\n", csv_prefix, name, count_requests, duration.count()/1000000.0, count_results);
    }
    return count_results;
}

constexpr unsigned int CACHE_JSON = 0;
constexpr unsigned int CACHE_REGEX = 1;
constexpr unsigned int CACHE_CAPTURE = 2;
typedef std::vector<std::array<std::string, 3>> request_pattern_t;
typedef std::array<std::vector<std::reference_wrapper<std::string>>, 3> request_pattern_ref_t;
struct Method {
    std::string name;
    std::function<void*(const request_pattern_ref_t&)> setup;
    std::function<void(void*)> release;
    std::function<bool(void*)> stage_build;
    std::function<unsigned int(void*, const std::string&)> stage_scan;
};


#define MREG std::vector<Method> mreg
extern MREG;

struct Registrator {
    Registrator(const Method& m) {
        mreg.push_back(m);
    }
};
#define ADD_METHOD(N, set, rel, build, scan) \
static Registrator __reg(Method{(N), (set), (rel), (build), (scan)})

constexpr unsigned int CACHE_NOT_FOUND = 0xffffffff;
//// Basic search
// Custom hash and equality for heterogeneous lookup
struct StringHash {
    using is_transparent = void;
    size_t operator()(std::string_view txt) const { return std::hash<std::string_view>{}(txt); }
    size_t operator()(const std::string& txt) const { return std::hash<std::string>{}(txt); }
};

struct StringEqual {
    using is_transparent = void;
    bool operator()(std::string_view lhs, std::string_view rhs) const { return lhs == rhs; }
    bool operator()(const std::string& lhs, std::string_view rhs) const { return lhs == rhs; }
    bool operator()(std::string_view lhs, const std::string& rhs) const { return lhs == rhs; }
    bool operator()(const std::string& lhs, const std::string& rhs) const { return lhs == rhs; }
};
template<typename T>
struct basic_cache_t {
    std::unordered_map<T, unsigned int, StringHash, StringEqual> cache;
    std::vector<std::string_view> request_parts;
    basic_cache_t(const request_pattern_ref_t& patt) {
        for(unsigned int x = 0; x < patt[CACHE_JSON].size(); x++) {
            rapidjson::Document doc;
            doc.Parse(patt[CACHE_JSON][x].get().c_str());
            std::string key = doc["UpdateExpression"].GetString();
            cache[key] = x;
        }
    }
    bool printed = true;
    unsigned int find() {
        if (request_parts.size() == 0) {
            return CACHE_NOT_FOUND;
        }

        if(!printed) {
            std::cout << request_parts.capacity() << " | ";
            for(unsigned int i = 0; i < request_parts.size(); i++) {
                std::cout << request_parts[i] << " | ";
            }
            std::cout << "\n";
            printed = true;
        }

        auto it = cache.find(request_parts.back());
        request_parts.clear();
        if (it == cache.end()) {
            return CACHE_NOT_FOUND;
        }
        return it->second;
    }
};

template<typename T=std::string>
void* basic_cache_setup(const request_pattern_ref_t& patt) {
    return new basic_cache_t<T>(patt);
}

template<typename T=std::string>
void basic_cache_release(void* context) {
    delete (basic_cache_t<T>*)context;
}

template<typename T>
struct built_context_t {
    const request_pattern_ref_t& patterns;
    T built_cache;
    built_context_t(const request_pattern_ref_t& patt)
     : patterns(patt), built_cache(patterns){
    }
};

template<typename T>
void* setup(const request_pattern_ref_t& patt) {
    built_context_t<T>* ret = new built_context_t<T>(patt);
    return ret;
}

template<typename T>
void release(void* context) {
    delete (built_context_t<T>*)context;
}