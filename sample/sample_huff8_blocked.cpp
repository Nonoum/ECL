#include "../ECL_common.c"
#include "../ECL_Huff8.c"
#include "../ECL_utils.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string.h>
#include <cassert>
#include <chrono>

uint64_t GetTimeMicroseconds() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

void s_show_usage() {
    std::cout << "-- Usage compress: sample c 32768 my-src-file my-output-compressed-file" << std::endl;
    std::cout << "-- Usage decompress: sample d my-compressed-file my-output-file" << std::endl;
    std::cout << "  where '32768' is block_size for Huffman processing, which has to be > 0 and < 65536" << std::endl;
    std::cout << "  e.g:" << std::endl;
    std::cout << "    sample c 20 111.txt 111.txt.huff8" << std::endl;
    std::cout << "    sample d 111.txt.huff8 111.txt.recovered" << std::endl;
    std::cout << "  will provide '111.txt.huff8' compressed file, and '111.txt.recovered' decompressed file matching original '111.txt'" << std::endl;
    std::cout << "  OPERANDS ORDER IS STRICT." << std::endl;
}

typedef std::vector<uint8_t> Raw;

bool s_file_exists(const char* fname) {
    std::ifstream ifs(fname, std::ios::binary);
    return !!ifs;
}

Raw s_read_file(const char* fname) {
    std::ifstream ifs(fname, std::ios::binary);
    if(! ifs) {
        return Raw();
    }
    ifs.seekg(0, ifs.end);
    auto size = ifs.tellg();
    ifs.seekg(0, ifs.beg);
    if(size) {
        Raw data;
        data.resize(size);
        if(ifs.read((char*)data.data(), size)) {
            return data;
        }
    }
    return Raw();
}

Raw s_handle_src(const char* src_fname) {
    if(! s_file_exists(src_fname)) {
        std::cout << "- error: file '" << src_fname << "' doesn't exist / can't be opened" << std::endl;
        return Raw();
    }
    auto src = s_read_file(src_fname);
    if(! src.size()) {
        std::cout << "- error: can't read file '" << src_fname << "' or it's empty" << std::endl;
        return Raw();
    }
    return src;
}

bool s_write_file_part(std::ostream& file, const uint8_t* data, size_t size) {
    if(data && size) {
        if(! file.write((const char*)data, size)) {
            std::cout << "- error: can't write file" << std::endl;
            return false;
        }
    }
    return true;
}

bool s_write_file(const char* fname, const uint8_t* hdr, size_t hdr_size, const Raw& data) {
    std::ofstream ofs(fname, std::ios::binary);
    if(! ofs) {
        std::cout << "- error: file '" << fname << "' can't be written" << std::endl;
        return false;
    }
    return s_write_file_part(ofs, hdr, hdr_size)
        && s_write_file_part(ofs, data.data(), data.size());
}

//#define SAMPLE_USE_MEMCPY_REPLACEMENT // as CPU/MEMORY performance reference

bool s_try_compress(const char* src_fname, const char* dst_fname, int block_size) {
    assert(block_size > 0);
    std::cout << "compressing '" << src_fname << "' to '" << dst_fname << "' with block_size=" << block_size << std::endl;
    const Raw src = s_handle_src(src_fname);
    if(! src.size()) {
        return false;
    }
    Raw output;
    const auto enough_size = ECL_HUFF8_GET_BOUND(src.size()); // ~roughly enough size, we do block-wise compression and add extra markers
    output.reserve(enough_size); // by fact it's just a little optimization as we'll append data block-by-block, ensure we won't crash on allocation in the end for large file
    /// SAMPLE SPECIFIC PART START /// fill output block-by-block
    const auto time_us_before = GetTimeMicroseconds();
    {
        Raw tmp_comp_block;

        for(size_t start = 0; start < src.size(); ) {
            auto portion_size = std::min<size_t>((src.size() - start), size_t(block_size));
            auto comp_alloc_size = ECL_HUFF8_GET_BOUND(portion_size);
            assert(portion_size);

            const size_t prefix_size = 2; // uint16_t 'current block size' aka number of bytes in current input
            tmp_comp_block.resize( comp_alloc_size + prefix_size );
            tmp_comp_block[0] = uint8_t(portion_size & 0xFF);
            tmp_comp_block[1] = uint8_t((portion_size >> 8) & 0xFF);

            auto comp_dst = tmp_comp_block.data() + prefix_size;

#ifdef SAMPLE_USE_MEMCPY_REPLACEMENT
            memcpy(comp_dst, src.data() + start, portion_size);
            const auto csize = ECL_usize(portion_size) * 8;
#elif 0 // try ECL_Huff8_TryCompress16_TSpec512_Raw (less memory usage, restricted block size - depends on actual data)
            // *compress extra buffers for work
            uint16_t buf536_u16[536/2];
            uint8_t buf256_u8[256/1];
            uint8_t buf768[768/1];
            const auto csize = ECL_Huff8_TryCompress16_TSpec512_Raw(src.data() + start, uint16_t(portion_size), 1, buf536_u16, buf256_u8, buf768, comp_dst, comp_alloc_size);
#elif 1 // try ECL_Huff8_TryCompress16_TSpec768_Raw (less memory usage, restricted block size - depends on actual data)
            // *compress extra buffers for work
            uint16_t buf800[800/2];
            uint8_t buf768[768/1];
            const auto csize = ECL_Huff8_TryCompress16_TSpec768_Raw(src.data() + start, uint16_t(portion_size), 1, buf800, buf768, comp_dst, comp_alloc_size);
#else // default - ECL_Huff8_Compress16_ULM_Raw (more memory usage, not restricted)
            // *compress extra buffers for work
            uint32_t buf1024[1024/4];
            uint8_t buf44_u8[44/1];
            uint8_t buf768[768/1];
            const auto csize = ECL_Huff8_Compress16_ULM_Raw(src.data() + start, uint16_t(portion_size), 1, buf1024, buf44_u8, buf768, comp_dst, comp_alloc_size);
#endif

            if(! csize) {
                std::cout << "- error: unexpected compression error :|" << std::endl;
                return false;
            }
            const auto comp_block_size = prefix_size + ((csize + 7) / 8);

            auto dst_start_index = output.size();
            output.resize(output.size() + comp_block_size);
            memcpy(output.data() + dst_start_index, tmp_comp_block.data(), comp_block_size);

            start += portion_size;
        }
    }
    const auto time_us_after = GetTimeMicroseconds();
    const auto seconds_spent = double(time_us_after - time_us_before) / 1000000.;
    const auto mbytes_per_sec = double(src.size()) / (double(1024*1024) * seconds_spent);
    /// SAMPLE SPECIFIC PART END   ///
    const auto comp_size = output.size();
    // encode original file size in header - encode as E7 number
    uint8_t hdr[10];
    const auto hdr_end = ECL_Helper_WriteE7(hdr, sizeof(hdr), src.size());
    if(! hdr_end) {
        std::cout << "- error: unknown stream error :|" << std::endl;
        return false;
    }
    const auto hdr_size = hdr_end - hdr;
    const auto total_size = comp_size + hdr_size;
    // write file data
    std::cout << "- successfully compressed in " << seconds_spent << " seconds (" << mbytes_per_sec << " mb/s); original size = " << src.size() << std::endl;
    std::cout << "compressed stream size = " << comp_size
              << " (with " << hdr_size << " byte header = " << total_size << ")" << std::endl;
    std::cout << "ratio = " << std::fixed << (double(comp_size) / double(src.size())) << std::endl;
    return s_write_file(dst_fname, hdr, hdr_size, output);
}

bool s_try_decompress(const char* src_fname, const char* dst_fname) {
    std::cout << "decompressing '" << src_fname << "' to '" << dst_fname << "'" << std::endl;
    const Raw src = s_handle_src(src_fname);
    if(! src.size()) {
        return false;
    }
    ECL_usize original_size;
    const auto hdr_end = ECL_Helper_ReadE7(src.data(), src.size(), &original_size);
    if(! hdr_end) {
        std::cout << "- error: invalid file content" << std::endl;
        return false;
    }
    const auto comp_start = hdr_end;
    const auto comp_end = src.data() + src.size();
    Raw recovered;
    Raw tmp_output;
    recovered.reserve(original_size);
    /// SAMPLE SPECIFIC PART START /// fill output block-by-block
    const auto time_us_before = GetTimeMicroseconds();
    for(auto start_ptr = comp_start; start_ptr != comp_end; ) {
        const size_t prefix_size = 2; // uint16_t 'current block size' aka number of bytes in current block
        if(uintptr_t(comp_end - start_ptr) < prefix_size) {
            std::cout << "- file error: can't read block size" << std::endl;
            break; // error
        }
        auto portion_size = uint16_t(uint16_t(start_ptr[0]) | (uint16_t(start_ptr[1]) << 8));
        if(! portion_size) {
            std::cout << "- file error: zero block size" << std::endl;
            break; // error
        }
        start_ptr += prefix_size;

        tmp_output.resize(portion_size);
        auto left_size = comp_end - start_ptr;

#ifdef SAMPLE_USE_MEMCPY_REPLACEMENT
        if(left_size < ECL_usize(portion_size)) {
            break;
        }
        memcpy(tmp_output.data(), start_ptr, portion_size);
        auto consumed_size = ECL_usize(portion_size);
#elif 0 // decompress default
        uint16_t buf1024_u16[1024/2];
        auto consumed_size = ECL_Huff8_Decompress_Raw(start_ptr, left_size, buf1024_u16, tmp_output.data(), portion_size, 1);
        assert(consumed_size);
#else // decompress FAST (>3x faster on 'silesia.tar' 202mb file)
        uint16_t buf1024_u16[1024/2];
        uint16_t buf768_u16[768/2];
        auto consumed_size = ECL_Huff8_DecompressWithDTable768_Raw(start_ptr, left_size, buf1024_u16, buf768_u16, tmp_output.data(), portion_size, 1);
        assert(consumed_size);
#endif
        if(consumed_size > left_size) {
            assert(false);
            break; // hard error, supposed to be handled inside ECL_Huff8_Decompress_Raw
        }

        // append decompressed block to output
        auto dst_start_index = recovered.size();
        recovered.resize(recovered.size() + portion_size);
        memcpy(recovered.data() + dst_start_index, tmp_output.data(), portion_size);

        start_ptr += consumed_size;
    }
    const auto time_us_after = GetTimeMicroseconds();
    const auto seconds_spent = double(time_us_after - time_us_before) / 1000000.;
    const auto mbytes_per_sec = double(original_size) / (double(1024*1024) * seconds_spent);
    /// SAMPLE SPECIFIC PART END   ///
    if(recovered.size() != original_size) {
        std::cout << "- error: decompression failed - invalid file content" << std::endl;
        return false;
    }
    std::cout << "- successfully decompressed in " << seconds_spent << " seconds (" << mbytes_per_sec << " mb/s); size = " << original_size << std::endl;
    return s_write_file(dst_fname, nullptr, 0, recovered);
}

bool s_cmp_operand(const char* str, const char* expected) {
    return str && expected && (0 == strcmp(str, expected));
}

int main(int argc, char* argv[]) {
    std::cout << "*** Sample ECL program to compress/decompress with ECL:Huff8 (with data split in blocks) ***" << std::endl;
    if((argc == 4) && s_cmp_operand(argv[1], "d")) {
        if(s_try_decompress(argv[2], argv[3])) {
            return 0;
        }
        std::cout << std::endl;
    } else if((argc == 5) && s_cmp_operand(argv[1], "c")) {
        int block_size = atoi(argv[2]);
        if((block_size < 1) || (block_size >= 65536)) {
            std::cout << "- error: block_size has to be > 0 and < 65536" << std::endl;
        } else if(s_try_compress(argv[3], argv[4], block_size)) {
            return 0;
        }
        std::cout << std::endl;
    }
    s_show_usage();
    return 0;
}
