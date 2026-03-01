#include "../ECL_SLA.h"
#include "ntest/ntest.h"

#include <vector>

NTEST(test_SLA_PackU16) {
    NTEST_SUPPRESS_UNUSED;
    std::vector<uint16_t> tmp_src, recovered;
    std::vector<uint8_t> buffers;

    const std::vector<uint16_t> sample_data = {7345, 46, 9825, 9999, 111, 23897, 55555};

    { // test with offset == 0
        const ECL_usize offset = 0;
        for(ECL_usize size = 1; size < ECL_usize(sample_data.size() - offset); ++size) {
            tmp_src.resize(size);
            memcpy(tmp_src.data(), sample_data.data() + offset, size * sizeof(sample_data[0]));
            buffers.resize(size * sizeof(sample_data[0]));

            approve(ECL_SLA_PackU16(tmp_src.data(), offset, size, buffers.data()) > 0);

            recovered.resize(size);
            memset(recovered.data(), 0, size * sizeof(sample_data[0]));
            approve(ECL_SLA_UnpackU16(buffers.data(), recovered.data(), offset, size) > 0);

            ECL_TEST_COMPARE_CUSTOM(tmp_src, recovered)
        }
    }

    { // test with offset != 0 - split in 2 parts
        tmp_src = sample_data;
        recovered.resize(tmp_src.size());
        memset(recovered.data(), 0, tmp_src.size() * sizeof(sample_data[0]));

        buffers.resize(tmp_src.size() * sizeof(sample_data[0]));

        {
            ECL_usize offset = 0;
            ECL_usize size = 4;
            approve(ECL_SLA_PackU16(tmp_src.data(), offset, size, buffers.data()) > 0);
            approve(ECL_SLA_UnpackU16(buffers.data(), recovered.data(), offset, size) > 0);
        }
        {
            ECL_usize offset = 4;
            ECL_usize size = tmp_src.size() - offset;
            approve(ECL_SLA_PackU16(tmp_src.data(), offset, size, buffers.data()) > 0);
            approve(ECL_SLA_UnpackU16(buffers.data(), recovered.data(), offset, size) > 0);
        }
        ECL_TEST_COMPARE_CUSTOM(tmp_src, recovered)
    }
}

NTEST(test_SLA_PackU32) {
    NTEST_SUPPRESS_UNUSED;
    std::vector<uint32_t> tmp_src, recovered;
    std::vector<uint8_t> buffers;

    const std::vector<uint32_t> sample_data = {757345, 466, 985725, 99777799, 156756711, 25367897, 55555, 9, 9877};

    { // test with offset == 0
        const ECL_usize offset = 0;
        for(ECL_usize size = 1; size < ECL_usize(sample_data.size() - offset); ++size) {
            tmp_src.resize(size);
            memcpy(tmp_src.data(), sample_data.data() + offset, size * sizeof(sample_data[0]));
            buffers.resize(size * sizeof(sample_data[0]));

            approve(ECL_SLA_PackU32(tmp_src.data(), offset, size, buffers.data()) > 0);

            recovered.resize(size);
            memset(recovered.data(), 0, size * sizeof(sample_data[0]));
            approve(ECL_SLA_UnpackU32(buffers.data(), recovered.data(), offset, size) > 0);

            ECL_TEST_COMPARE_CUSTOM(tmp_src, recovered)
        }
    }

    { // test with offset != 0 - split in 2 parts
        tmp_src = sample_data;
        recovered.resize(tmp_src.size());
        memset(recovered.data(), 0, tmp_src.size() * sizeof(sample_data[0]));

        buffers.resize(tmp_src.size() * sizeof(sample_data[0]));

        {
            ECL_usize offset = 0;
            ECL_usize size = 4;
            approve(ECL_SLA_PackU32(tmp_src.data(), offset, size, buffers.data()) > 0);
            approve(ECL_SLA_UnpackU32(buffers.data(), recovered.data(), offset, size) > 0);
        }
        {
            ECL_usize offset = 4;
            ECL_usize size = tmp_src.size() - offset;
            approve(ECL_SLA_PackU32(tmp_src.data(), offset, size, buffers.data()) > 0);
            approve(ECL_SLA_UnpackU32(buffers.data(), recovered.data(), offset, size) > 0);
        }
        ECL_TEST_COMPARE_CUSTOM(tmp_src, recovered)
    }
}
