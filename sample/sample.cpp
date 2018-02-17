#include "../ECL_common.c"
#include "../ECL_NanoLZ.c"

#include <iostream>
#include <fstream>
#include <vector>
#include <string.h>

static const auto c_scheme = ECL_NANOLZ_SCHEME1;

void s_show_usage() {
    std::cout << "-- Usage compress: sample c 20 my-src-file my-output-compressed-file" << std::endl;
    std::cout << "-- Usage decompress: sample d my-compressed-file my-output-file" << std::endl;
    std::cout << "  where '20' is serach_limit (compression level), which has to be > 0" << std::endl;
    std::cout << "  e.g:" << std::endl;
    std::cout << "    sample c 20 111.txt 111.txt.nlz" << std::endl;
    std::cout << "    sample d 111.txt.nlz 111.txt.recovered" << std::endl;
    std::cout << "  will provide '111.txt.nlz' compressed file, and '111.txt.recovered' decompressed file matching original '111.txt'" << std::endl;
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

bool s_try_compress(const char* src_fname, const char* dst_fname, int limit) {
    std::cout << "compressing '" << src_fname << "' to '" << dst_fname << "' with limit=" << limit << std::endl;
    const auto src = s_handle_src(src_fname);
    if(! src.size()) {
        return false;
    }
    Raw output;
    const auto enough_size = ECL_NANO_LZ_GET_BOUND(src.size());
    output.resize(enough_size);
    const auto comp_size = ECL_NanoLZ_Compress_auto(c_scheme, src.data(), src.size(), output.data(), output.size(), limit);
    if((! comp_size) || (comp_size > output.size())) {
        std::cout << "- error: an unknown error has occurred, maybe file is too big" << std::endl;
        return false;
    }
    output.resize(comp_size);
    // encode original file size in header - encode as E7 number
    uint8_t hdr[10];
    ECL_JH_WState stream;
    ECL_JH_WInit(&stream, hdr, sizeof(hdr), 0);
    ECL_JH_Write_E7(&stream, src.size());
    if(! stream.is_valid) {
        std::cout << "- error: unknown stream error :|" << std::endl;
        return false;
    }
    const auto hdr_size = stream.next - hdr;
    const auto total_size = comp_size + hdr_size;
    // write file data
    std::cout << "- successfully compressed: original size = " << src.size() << std::endl;
    std::cout << "compressed stream size = " << comp_size
              << " (with " << hdr_size << " byte header = " << total_size << ")" << std::endl;
    std::cout << "ratio = " << std::fixed << (double(comp_size) / double(src.size())) << std::endl;
    return s_write_file(dst_fname, hdr, hdr_size, output);
}

bool s_try_decompress(const char* src_fname, const char* dst_fname) {
    std::cout << "decompressing '" << src_fname << "' to '" << dst_fname << "'" << std::endl;
    const auto src = s_handle_src(src_fname);
    if(! src.size()) {
        return false;
    }
    ECL_JH_RState stream;
    ECL_JH_RInit(&stream, src.data(), src.size(), 0);
    const auto original_size = ECL_JH_Read_E7(&stream);
    if(! stream.is_valid) {
        std::cout << "- error: invalid file content" << std::endl;
        return false;
    }
    const auto comp_start = stream.next;
    const auto comp_size = src.data() + src.size() - comp_start;
    Raw recovered;
    recovered.resize(original_size);
    const auto decomp_size = ECL_NanoLZ_Decompress(c_scheme, comp_start, comp_size, recovered.data(), recovered.size());
    if(decomp_size != original_size) {
        std::cout << "- error: decompression failed - invalid file content" << std::endl;
        return false;
    }
    std::cout << "- successfully decompressed: size = " << decomp_size << std::endl;
    return s_write_file(dst_fname, nullptr, 0, recovered);
}

bool s_cmp_operand(const char* str, const char* expected) {
    return str && expected && (0 == strcmp(str, expected));
}

int main(int argc, char* argv[]) {
    std::cout << "*** Sample ECL program to compress/decompress with ECL:NanoLZ format ***" << std::endl;
    do {
        if((argc == 4) && s_cmp_operand(argv[1], "d")) {
            if(s_try_decompress(argv[2], argv[3])) {
                return 0;
            }
        } else if((argc == 5) && s_cmp_operand(argv[1], "c")) {
            int limit = atoi(argv[2]);
            if(limit < 1) {
                std::cout << "- error: limit has to be > 0" << std::endl;
                break;
            }
            if(s_try_compress(argv[3], argv[4], limit)) {
                return 0;
            }
        }
    } while(0);
    s_show_usage();
    return 0;
}
