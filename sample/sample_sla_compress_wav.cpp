#include "../ECL_common.c"
#include "../ECL_SLA.c"
#include "../ECL_utils.h"

#include "HelperTimeMeasurer.h"

/*
    A thirdparty dependency for parsing wav files:
        https://github.com/KanijiroSakado/WavFileReader
        tested @ commit 734aac2f98ddc493cfeb6fae89ba17f26cbb8775
*/
#include "../../WavFileReader/wav_file_reader.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string.h>
#include <cassert>
#include <algorithm>
#include <numeric>

bool EnsureAboveZero(uint8_t val) {
    assert(val > 0);
    return val > 0;
}

void s_show_usage() {
    std::cout << "-- Usage: sample 128 my_wav_file.wav" << std::endl;
    std::cout << "  where '128' is block_size for SLA processing, which has to be > 0 (reasonable is 32 .. 256)" << std::endl;
    std::cout << "  OPERANDS ORDER IS STRICT." << std::endl;
    std::cout << "  The sample will unpack wav file (relies on thirdparty library), compress/decompress/verify it in memory" << std::endl;
    std::cout << "  and print stats, without output file. Currently only 16-bit-per-sample, 1-2 channel wav is supported." << std::endl;
    std::cout << "  For estimating compression for higher sample bitness you can presume that additional info (precision) normally can't" << std::endl;
    std::cout << "  be compressed by SLA, meaning that 24bit compression is 16bit compression + 8bit as-is." << std::endl;
}

static double TestAudio_GetMonoCompressionRatio16(
    const std::vector<uint16_t>& samples, uint16_t samples_in_block
    , HelperTimeMeasurer& comp_measurer, HelperTimeMeasurer& decomp_measurer
) {
    using Samp_Ty = uint16_t; // further logic is still hardcoded for 2-byte sample size
    const size_t Samp_Size = sizeof(Samp_Ty);
    //
    const size_t n_blocks = (samples.size() + samples_in_block - 1) / samples_in_block;
    std::vector<uint8_t> block_buf_data; // actually contains 2 compressible blocks (one for lower sample bytes, one for higher)
    block_buf_data.resize(samples_in_block * Samp_Size);

    std::vector<uint8_t> tmp_compr; // compressed data representing whole samples vector
    const size_t enough_size = ECL_SLA_GET_BOUND(samples_in_block) * n_blocks * Samp_Size;
    tmp_compr.resize(enough_size);
    // set up wstream
    ECL_WSTREAM_Type wstream;
    ECL_WSTREAM_JHx_Init(&wstream, tmp_compr.data(), tmp_compr.size());
    // --------------

    //
    comp_measurer.startResume();
    for(size_t i_block = 0; i_block < n_blocks; ++i_block) {
        const size_t samples_offs = i_block * samples_in_block;
        const size_t n_samples = std::min<size_t>(samples.size() - samples_offs, samples_in_block); // in current block
        assert(n_samples > 0);
        assert(n_samples <= samples_in_block);
        EnsureAboveZero( ECL_SLA_PackU16(samples.data(), samples_offs, n_samples, block_buf_data.data()) );
        //
        for(size_t i = 0; i < Samp_Size; ++i) {
            // compress subblock i
            char tmp_sla_header;
            auto subbuffer = block_buf_data.data() + n_samples * i;
            auto bits_size = ECL_SLA_Analyze(subbuffer, n_samples, &tmp_sla_header);
            (void)bits_size;
            EnsureAboveZero( ECL_SLA_Compress(subbuffer, n_samples, tmp_sla_header, &wstream) );
        }
    }
    comp_measurer.stop();
    const size_t compr_in_n_bytes = size_t(wstream.next - tmp_compr.data());

    { // decompress, verify
        // set up rstream
        ECL_RSTREAM_Type rstream;
        ECL_RSTREAM_JHx_Init(&rstream, tmp_compr.data(), compr_in_n_bytes);
        // decompress to temp output
        std::vector<uint16_t> recovered_samples;
        recovered_samples.resize(samples.size());

        decomp_measurer.startResume();
        for(size_t i_block = 0; i_block < n_blocks; ++i_block) {
            const size_t samples_offs = i_block * samples_in_block;
            const size_t n_samples = std::min<size_t>(samples.size() - samples_offs, samples_in_block); // in current block
            assert(n_samples > 0);
            assert(n_samples <= samples_in_block);
            //
            for(size_t i = 0; i < Samp_Size; ++i) {
                // decompress subblock i
                EnsureAboveZero( ECL_SLA_Decompress(&rstream, block_buf_data.data() + n_samples * i, n_samples) );
            }
            //
            EnsureAboveZero( ECL_SLA_UnpackU16(block_buf_data.data(), recovered_samples.data(), samples_offs, n_samples) );
        }
        decomp_measurer.stop();
        if(recovered_samples != samples) {
            assert(false);
            return 0;
        }
    }
    // --------------

    double ratio = double(compr_in_n_bytes) / double(samples.size() * sizeof(samples[0]));
    return ratio;
}

static double TestAudio_GetStereoCompressionRatio16(
    const std::vector<uint16_t>& samp_l, const std::vector<uint16_t>& samp_r, uint16_t samples_in_block
    , HelperTimeMeasurer& comp_measurer, HelperTimeMeasurer& decomp_measurer)
{
    if(samp_l.size() != samp_r.size()) {
        throw std::logic_error("err");
    }
    std::vector<uint16_t> tmp_diff_chan_2; // with tested audio samples diff/bearing-channel option doesn't help compression at all
    tmp_diff_chan_2.resize(samp_l.size());
    for(size_t i = 0; i < samp_l.size(); ++i) {
        tmp_diff_chan_2[i] = samp_r[i] - samp_l[i];
    }
    //
    double ratio1 = TestAudio_GetMonoCompressionRatio16(samp_l, samples_in_block, comp_measurer, decomp_measurer);
    double ratio2 = TestAudio_GetMonoCompressionRatio16(samp_r, samples_in_block, comp_measurer, decomp_measurer);
    double ratiodiff = TestAudio_GetMonoCompressionRatio16(tmp_diff_chan_2, samples_in_block, comp_measurer, decomp_measurer);

    auto chosen_2nd = std::min(ratio2, ratiodiff);
    auto ratio_avg = (ratio1 + chosen_2nd) / 2.; // we can simply avg them as source sizes are equaL
    return ratio_avg;
}

bool TestSLA_CompressDecompressAudio(const char* fname, int block_size, std::ostream& log) {
    static_assert((sizeof(short) == sizeof(uint16_t)) && "using WavFileReader samples in 'short' format");

	std::vector<uint16_t> bufL, bufR;
    const auto n_samples = sakado::WavFileReader(fname, 1).NumData;
    sakado::WavFileReader wfr(fname, n_samples);
    if(! n_samples) {
        log << "- error: failure parsing .wav file \"" << fname << "\"" << std::endl;
        return false;
    }

    //If the format is Mono, same values will be loaded to bufL and bufR
    bufL.resize(n_samples);
    bufR.resize(n_samples);
    wfr.ReadLR((short*)(bufL.data()), (short*)(bufR.data()), n_samples);


    if(1 /* print wav info */) {
        log << " Input wav file info:" << std::endl;
        log << "  fname: \"" << fname << "\"" << std::endl;
        log << "  NumChannels: " << wfr.NumChannels << std::endl;
        log << "  SampleRate: " << wfr.SampleRate << std::endl;
        log << "  BitsPerSample: " << wfr.BitsPerSample << std::endl;
        log << "  DataSize: " << wfr.DataSize << " (n bytes in samples)" << std::endl;
        log << "  NumData: " << wfr.NumData << " (n samples)" << std::endl;
        auto avg_l = double( std::accumulate(bufL.begin(), bufL.end(), uint64_t(0), [](uint64_t a, uint64_t b) { return a+b; }) ) / double(bufL.size());
        auto minmax_l = std::minmax_element(bufL.begin(), bufL.end(), std::less<uint16_t>());
        log << "  avg_l: " << avg_l << " (average sample value on left channel; out of 65535 max)" << std::endl;
        if(bufL.size()) { // ensure can deref iterators.... just.. in.. case
            log << "  min_l: " << *minmax_l.first << std::endl;
            log << "  max_l: " << *minmax_l.second << std::endl;
        }
        if(wfr.NumChannels > 1) {
            auto avg_r = double( std::accumulate(bufR.begin(), bufR.end(), uint64_t(0), [](uint64_t a, uint64_t b) { return a+b; }) ) / double(bufR.size());
            auto minmax_r = std::minmax_element(bufR.begin(), bufR.end(), std::less<uint16_t>());
            log << "  avg_r: " << avg_r << " (average sample value on right channel; out of 65535 max)" << std::endl;
            if(bufR.size()) { // ensure can deref iterators.... just.. in.. case
                log << "  min_r: " << *minmax_r.first << std::endl;
                log << "  max_r: " << *minmax_r.second << std::endl;
            }
        }
    }

    if((wfr.NumChannels >= 1) && (wfr.NumChannels <= 2) && (wfr.BitsPerSample == 16)) {
        log << "  ----- " << std::endl;
        if(0 /* test on filled random values &-truncated with mask */) {
            std::vector<uint16_t> randd;
            randd.resize(bufL.size());
            uint16_t rand_mask = 0x0FFF;
            for(size_t i = 0; i < randd.size(); ++i) {
                randd[i] = rand() & rand_mask;
            }
            HelperTimeMeasurer comp_measurer, decomp_measurer;
            auto rmt = TestAudio_GetMonoCompressionRatio16(randd, block_size, comp_measurer, decomp_measurer);
            log << "  for block_size=" << block_size << ", rand_mask=" << std::hex << rand_mask << std::dec << " randd ratio=" << rmt << std::endl;
        } else {
            // in case file is MONO - both channels have equal data
            double rm_l = 1;
            double rm_r = 1;
            {
                HelperTimeMeasurer comp_measurer, decomp_measurer;
                rm_l = TestAudio_GetMonoCompressionRatio16(bufL, block_size, comp_measurer, decomp_measurer);
                log << "  for block_size=" << block_size << ":"
                    << "\n    channel_0 compress ratio = " << rm_l
                    << "\n    channel_0 compress seconds = " << comp_measurer.getTotalSeconds()
                    << "\n    channel_0 decompress seconds = " << decomp_measurer.getTotalSeconds()
                    << "\n    channel_0 compress mb/s = " << comp_measurer.calcSpeed_Mb_S(bufL.size() * sizeof(bufL[0]))
                    << "\n    channel_0 decompress mb/s = " << decomp_measurer.calcSpeed_Mb_S(bufL.size() * sizeof(bufL[0]))
                    << std::endl;
            }

            {
                HelperTimeMeasurer comp_measurer, decomp_measurer;
                rm_r = TestAudio_GetMonoCompressionRatio16(bufR, block_size, comp_measurer, decomp_measurer);
                log << "  for block_size=" << block_size << ":"
                    << "\n    channel_1 compress ratio = " << rm_r
                    << "\n    channel_1 compress seconds = " << comp_measurer.getTotalSeconds()
                    << "\n    channel_1 decompress seconds = " << decomp_measurer.getTotalSeconds()
                    << "\n    channel_1 compress mb/s = " << comp_measurer.calcSpeed_Mb_S(bufR.size() * sizeof(bufR[0]))
                    << "\n    channel_1 decompress mb/s = " << decomp_measurer.calcSpeed_Mb_S(bufR.size() * sizeof(bufR[0]))
                    << std::endl;
            }

            double overall_ratio = 1;
            if(1 /* try compress as stereo */) {
                // tests on a few files result in uselessness of TestAudio_GetStereoCompressionRatio16 for those particular files (no compression improvement)
                //   however it can be beneficial for "some" files or can be reworked with some phase matching algorithms (for potential improvement)
                HelperTimeMeasurer comp_measurer, decomp_measurer;
                auto rs = TestAudio_GetStereoCompressionRatio16(bufL, bufR, block_size, comp_measurer, decomp_measurer); // contains 3 TestAudio_GetMonoCompressionRatio16 calls (+aux data copying code)
                log << "  for block_size=" << block_size << ":"
                    << "\n    stereo compress ratio = " << rs
                    << "\n    stereo compress seconds = " << comp_measurer.getTotalSeconds()
                    << "\n    stereo decompress seconds = " << decomp_measurer.getTotalSeconds()
                    << "\n    stereo compress mb/s = " << comp_measurer.calcSpeed_Mb_S(bufR.size() * sizeof(bufR[0]) * 2)
                    << "\n    stereo decompress mb/s = " << decomp_measurer.calcSpeed_Mb_S(bufR.size() * sizeof(bufR[0]) * 2)
                    << std::endl;

                overall_ratio = rs;
            } else {
                overall_ratio = (rm_l + rm_r) / 2; // simple avg as channel sources have same size
            }
            //
            auto estm_uncompr_16bit_fsize = double(bufL.size() * sizeof(bufL[0]) * 2);
            auto estm_compr_16bit_fsize = estm_uncompr_16bit_fsize * overall_ratio;

            log << std::endl;
            log << "  raw data size = " << size_t(estm_uncompr_16bit_fsize) << std::endl;
            log << "  sla data size = " << size_t(estm_compr_16bit_fsize) << std::endl;
            log << "  overall data compression ratio = " << (estm_compr_16bit_fsize / estm_uncompr_16bit_fsize) << std::endl;
        }
    } else {
        log << "- error: unsupported wav format (sample needs updating)" << std::endl;
        return false;
    }
    return true;
}

bool s_file_exists(const char* fname) {
    std::ifstream ifs(fname, std::ios::binary);
    return !!ifs;
}

int main(int argc, char* argv[]) {
    std::cout << "*** Sample ECL program to test SLA compression on a .wav file ***" << std::endl;
    if(argc == 3) {
        int block_size = atoi(argv[1]);
        if((block_size < 1) || (block_size > 65535)) {
            std::cout << "- error: block_size has to be > 0 and < 65535" << std::endl;
            s_show_usage();
            return 1;
        }
        auto fname = argv[2];
        if(! s_file_exists(fname)) {
            std::cout << "- error: file \"" << fname << "\" doesn't exist" << std::endl;
            s_show_usage();
            return 2;
        }
        bool result = TestSLA_CompressDecompressAudio(fname, block_size, std::cout);
        if(! result) {
            std::cout << "- error processing wav." << std::endl;
            return 3;
        }
        return 0;
    }
    s_show_usage();
    return 0;
}
