#include "../ECL_NanoLZ.h"

#include <string>
#include <sstream>
#include <algorithm>
#include <iomanip>

bool ECL_Stat_Dir_Files_NanoLZ(const std::string& dir, ECL_NanoLZ_Scheme scheme, const std::string& log_prefix, std::ostream& log) {
    ECL_NanoLZ_FastParams fp;
    ECL_NanoLZ_FastParams_Alloc1(&fp, 12);
    for(int i = 0; ; ++i) {
        auto fname = dir + '/';
        {
            std::stringstream ss;
            ss << i;
            fname += ss.str();
        }
        std::ifstream ifs(fname, std::ios::binary);
        if(! ifs) {
            return true;
        }
        ifs.seekg(0, ifs.end);
        ECL_usize src_size = ifs.tellg();
        ifs.seekg(0, ifs.beg);
        std::vector<uint8_t> src;
        src.resize(src_size);
        if(! ifs.read((char*)src.data(), src_size) ) {
            return true;
        }

        // compress
        std::vector<uint8_t> tmp, tmp_output;
        const auto enough_size = ECL_NANO_LZ_GET_BOUND(src_size);
        tmp.resize(enough_size);
        auto comp_size = ECL_NanoLZ_Compress_fast1(scheme, src.data(), src_size, tmp.data(), enough_size, -1, &fp);
        //auto comp_size = ECL_NanoLZ_Compress_fast2(scheme, src.data(), src_size, tmp.data(), enough_size, -1, &fp);
        //auto comp_size = ECL_NanoLZ_Compress_slow(scheme, src.data(), src_size, tmp.data(), enough_size, -1);

        // decompress
        tmp_output.resize(src_size);
        const auto decomp_size = ECL_NanoLZ_Decompress(scheme, tmp.data(), comp_size, tmp_output.data(), src_size);

        // stat
        auto& ctrs = ECL_NanoLZ_Decompression_OpcodePickCounters;
        log << log_prefix << fname << std::setw(8) << src_size << " -> " << std::setw(8) << comp_size << " :: ";
        for(auto c : ctrs) { log << c << " "; }
        log << std::endl;
        if(decomp_size != src_size) {
            log << log_prefix << "failed at " << fname << ". decompressed=" << decomp_size << " != " << src_size << std::endl;
            return false;
        }
        if(memcmp(src.data(), tmp_output.data(), src_size)) {
            const auto pair = std::mismatch(src.begin(), src.end(), tmp_output.begin());
            const auto pos = pair.first - src.begin();
            log << log_prefix << "failed at " << fname << ". mismatches decompressed at " << pos << std::endl;
            return false;
        }
    }
    ECL_NanoLZ_FastParams_Destroy(&fp);
}
