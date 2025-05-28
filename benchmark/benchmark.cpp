#include "tests/tests.h"
MREG;
#include <fmt/format.h>
#include <regex>
#include <unordered_set>
#include <simdjson.h>

typedef std::unordered_map<std::string, std::vector<unsigned int>> result_t;
enum PATTERNSET_OPTIONS { SET_ORDERED_FIRST, SET_SHUFFLE, SET_ORDERED_LAST, SET_NO_MATCH};
// Function to convert PATTERNSET_OPTIONS to string for output
inline std::string patternset_options_to_string(PATTERNSET_OPTIONS option) {
    switch (option) {
        case SET_ORDERED_FIRST: return "F";
        case SET_SHUFFLE: return "H";
        case SET_ORDERED_LAST: return "L";
        case SET_NO_MATCH: return "N";
        default: return "U";
    }
}

template<PATTERNSET_OPTIONS M>
void test(request_pattern_t& req, request_pattern_t& cache, int test_time, result_t& res, const std::string& csv_prefix = "") {
    auto cache_size = cache.size() + (M != SET_NO_MATCH? req.size() : 0);
    std::vector<unsigned int> ids(req.size(), CACHE_NOT_FOUND);
    std::unordered_set<unsigned int> shuffle;
    if (M == SET_SHUFFLE) {
        while (shuffle.size() < req.size()) {
            shuffle.insert(rand() % cache_size);
        }
    } else if (M == SET_ORDERED_FIRST || M == SET_ORDERED_LAST) {
        for (int i = 0; i < req.size(); i++) {
            shuffle.insert((M == SET_ORDERED_FIRST ? i : cache_size - i - 1));
        }
    }
    request_pattern_ref_t patt;
    patt[CACHE_JSON].reserve(cache_size);
    patt[CACHE_REGEX].reserve(cache_size);
    patt[CACHE_CAPTURE].reserve(cache_size);
    int i_req = 0, i_cache = 0;
    for (int i = 0; i < cache_size; i++) {
        if(shuffle.find(i) == shuffle.end())
        {
            patt[CACHE_JSON].push_back(cache[i_cache][CACHE_JSON]);
            patt[CACHE_REGEX].push_back(cache[i_cache][CACHE_REGEX]);
            patt[CACHE_CAPTURE].push_back(cache[i_cache][CACHE_CAPTURE]);
            i_cache++;
        } else {
            patt[CACHE_JSON].push_back(req[i_req][CACHE_JSON]);
            patt[CACHE_REGEX].push_back(req[i_req][CACHE_REGEX]);
            patt[CACHE_CAPTURE].push_back(req[i_req][CACHE_CAPTURE]);
            ids[i_req] = i;
            i_req++;
        }
    }

    if (csv_prefix.empty()) {
        for(int i = 0; i < ids.size(); i++) {
            std::cout << "Find id: " << ids[i] << " (req len: " << req[i][CACHE_JSON].length() << ")\n";
        }
    }

    for (auto m : mreg) {
        void* context = m.setup(patt);
        if(m.stage_build) {
            auto build = [&](unsigned long long count, unsigned long long& hit) {
                if(m.stage_build(context)) hit++;
                return true;
            };
            auto name = m.name + "_build";
            res[name].push_back(measure(name, test_time, build, csv_prefix));
        }

        if(m.stage_scan) {
            auto scan = [&](unsigned long long count, unsigned long long& hit) {
                if(m.stage_scan(context, req[count % req.size()][CACHE_JSON]) == ids[count % req.size()]) hit++;
                return true;
            };
            auto name = m.name + "_scan";
            res[name].push_back(measure(name, test_time, scan, csv_prefix));
        }
        m.release(context);
    }
}

template<PATTERNSET_OPTIONS M>
void dataset_test(const std::string& name, result_t& result, int cache_size = 1000, int test_time = 10, int retry = 1, bool csv = false) {
    std::cerr << "Dataset: " << name << "\n";
    std::string path = "../data/";
    path += name;
    path += "/";
    request_pattern_t req(1);
    {
        std::ifstream stream(path + "request.txt");
        if (!stream.is_open()) {
            std::cerr << "Error opening file: " << path + "request.txt" << "\n";
            return;
        }
        stream.seekg(0, std::ios::end);
        req[0][CACHE_JSON].reserve(stream.tellg());
        stream.seekg(0, std::ios::beg);
        req[0][CACHE_JSON].assign((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        req[0][CACHE_JSON].reserve(req[0][CACHE_JSON].length() + (long int)(simdjson::SIMDJSON_PADDING)); // helper for simdjson
    }
    {
        std::ifstream stream(path + "pattern.txt");
        if (!stream.is_open()) {
            std::cerr << "Error opening file: " << path + "pattern.txt" << "\n";
            return;
        }
        stream.seekg(0, std::ios::end);
        req[0][CACHE_REGEX].reserve(stream.tellg());
        stream.seekg(0, std::ios::beg);
        req[0][CACHE_REGEX].assign((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    }
    {
        std::ifstream stream(path + "capture.txt");
        if (!stream.is_open()) {
            std::cerr << "Error opening file: " << path + "pattern.txt" << "\n";
            return;
        }
        stream.seekg(0, std::ios::end);
        req[0][CACHE_CAPTURE].reserve(stream.tellg());
        stream.seekg(0, std::ios::beg);
        req[0][CACHE_CAPTURE].assign((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    }
    request_pattern_t patt(cache_size);
    for (int i = 0; i < cache_size; i++) {
        auto i_req = i % req.size();
        patt[i][CACHE_JSON] = std::regex_replace(req[i_req][CACHE_JSON], std::regex(":val"), fmt::format(":v{}", i));
        patt[i][CACHE_REGEX] = std::regex_replace(req[i_req][CACHE_REGEX], std::regex(":val"), fmt::format(":v{}", i));
        patt[i][CACHE_CAPTURE] = std::regex_replace(req[i_req][CACHE_CAPTURE], std::regex(":val"), fmt::format(":v{}", i));
    }

    std::string csv_prefix = "";
    if (csv) {
        csv_prefix = fmt::format("{},{},{},{}", name, cache_size, patternset_options_to_string(M), req[0][CACHE_JSON].length());
    }
    for (int retries = 0; retries < retry; retries++) {
        test<M>(req, patt, test_time, result, csv_prefix);
    }
}

int main(int argc, char* argv[]) {
    int cache_size = argc < 2 ? 1000 : std::stoi(argv[1]);
    int test_time = argc < 3 ? 10 : std::stoi(argv[2]);
    int retry = argc < 4 ? 1 : std::stoi(argv[3]);
    PATTERNSET_OPTIONS match_opt = SET_SHUFFLE;
    //srand(static_cast<unsigned int>(time(nullptr)));
    srand(17);
    std::vector<std::string> datasets;
    bool csv = false;
    for (int i = 4; i < argc; i++) {
        if (std::string(argv[i]) == "-n") {
            match_opt = SET_NO_MATCH;
            continue;
        }
        if (std::string(argv[i]) == "-f") {
            match_opt = SET_ORDERED_FIRST;
            continue;
        }
        if (std::string(argv[i]) == "-l") {
            match_opt = SET_ORDERED_LAST;
            continue;
        }
        if (std::string(argv[i]) == "-c") {
            csv = true;
            continue;
        }
        datasets.push_back(argv[i]);
    }
    if (datasets.size() == 0) {
        datasets.push_back("0");
    }
    std::vector<result_t> datasets_results(datasets.size());
    for (int i = 0; i < datasets.size(); i++) {
        if (match_opt == SET_NO_MATCH) dataset_test<SET_NO_MATCH>(datasets[i], datasets_results[i], cache_size, test_time, retry, csv);
        else if (match_opt == SET_SHUFFLE) dataset_test<SET_SHUFFLE>(datasets[i], datasets_results[i], cache_size, test_time, retry, csv);
        else if (match_opt == SET_ORDERED_FIRST) dataset_test<SET_ORDERED_FIRST>(datasets[i], datasets_results[i], cache_size, test_time, retry, csv);
        else if (match_opt == SET_ORDERED_LAST) dataset_test<SET_ORDERED_LAST>(datasets[i], datasets_results[i], cache_size, test_time, retry, csv);
    }
    if (!csv) {
        std::map<std::string, std::array<unsigned int, 3>> results;
        for (auto& data : datasets_results) {
            for (auto& m : data) {
                results[m.first] = {0, 0xffffffff, 0};
            }
        }
        for (auto& aggr : results) {
            int count = 0;
            for (auto& set_res : datasets_results) {
                auto it = set_res.find(aggr.first);
                if (it != set_res.end()) {
                    for (auto& single_v : it->second) {
                        count++;
                        aggr.second[0] += single_v;
                        if (single_v < aggr.second[1]) {
                            aggr.second[1] = single_v;
                        }
                        if (single_v > aggr.second[2]) {
                            aggr.second[2] = single_v;
                        }
                    }
                }
            }
            aggr.second[0] /= count;
            aggr.second[0] /= test_time;
            aggr.second[1] /= test_time;
            aggr.second[2] /= test_time;
        }
        std::vector<std::pair<std::string, std::array<unsigned int, 3>>> sorted_results(results.begin(), results.end());
        std::sort(sorted_results.begin(), sorted_results.end(), [](const auto& a, const auto& b) {
            return a.second[0] < b.second[0];
        });
        fmt::print("=======================================\n");
        fmt::print("Cache size: {:>4}, Test time: {:>4}\n", cache_size, test_time);
        for (const auto& result : sorted_results) {
            fmt::print("{:>17}: {:>8}x [{:>8} - {:>8}]\n", result.first, result.second[0], result.second[1], result.second[2]);
        }
    }

    return 0;
}