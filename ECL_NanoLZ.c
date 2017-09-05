#include "ECL_NanoLZ.h"
#include "ECL_JH_States.h"

typedef struct {
    ECL_JH_WState stream;
    ECL_usize n_new;
    ECL_ssize offset;
    ECL_usize n_copy;
} ECL_NanoLZ_CompressorState;

// ending seq to dump last bytes
static void ECL_NanoLZ_WriteSeqOnlyNew(ECL_NanoLZ_CompressorState* state, const uint8_t* src) {
    ECL_ASSERT(state->n_copy == 0);

    ECL_JH_Write(&state->stream, 6, 3);
    ECL_JH_Write_E4(&state->stream, state->n_new);
    ECL_JH_Write(&state->stream, 0, 8);
    ECL_JH_Write(&state->stream, 0, 5);

    if(state->n_new) {
        uint8_t* dst = state->stream.next;
        ECL_JH_WJump(&state->stream, state->n_new);
        if(state->stream.is_valid) {
            memcpy(dst, src, state->n_new);
        }
    } else { // stream alignment opcode for flow mode
        ECL_JH_Write(&state->stream, 0, state->stream.n_bits);
    }
}

// tolerating seq to potentionally improve next seq; opcode 7
static void ECL_NanoLZ_WriteSeqPass(ECL_NanoLZ_CompressorState* state, const uint8_t* src) {
    ECL_ASSERT(state->n_copy == 1);
    ECL_ASSERT(state->n_new < 32);

    ECL_JH_Write(&state->stream, 7, 3);
    ECL_JH_Write(&state->stream, state->n_new, 5);
    if(state->n_new) {
        uint8_t* dst = state->stream.next;
        ECL_JH_WJump(&state->stream, state->n_new);
        if(state->stream.is_valid) {
            memcpy(dst, src, state->n_new);
        }
    }
}

static bool ECL_NanoLZ_WriteSeqNormal(ECL_NanoLZ_CompressorState* state, const uint8_t* src) {
    ECL_ASSERT(state->n_copy >= 2);

    // TODO optimize/minimize checks
    if(state->n_copy == 2) {
        if((state->n_new <= 3) && (state->offset <= 8)) {
            // TODO opcode 0
        } else if((state->n_new <= 15) && (state->offset <= 16)) {
            // TODO opcode 1
        } else if((state->n_new <= 15) && (state->offset <= 144)) {
            // TODO opcode 2
        } else if((state->n_new <= 143) && (state->offset <= 16)) {
            // TODO opcode 3
        } else {
            return false;
        }
    } else {
        if(state->n_copy <= 10) {
            if((state->n_new <= 15) && (state->offset <= 32)) {
                // TODO opcode 4
            } else if((state->n_new <= 15) && (state->offset <= 288)) {
                // TODO opcode 5
            } else if(1) { // TODO evaluate whether worth compressing
                // TODO opcode 6
            } else {
                return false;
            }
        } else { // only opcode 6
            if(1) { // TODO evaluate whether worth compressing
                // TODO opcode 6
            } else {
                return false;
            }
        }
    }
    return true;
}

ECL_usize ECL_NanoLZ_Compress(const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size) {
    ECL_NanoLZ_CompressorState state;
    const uint8_t* src_start;
    const uint8_t* src_end;
    const uint8_t* search_end;

    ECL_JH_WInit(&state.stream, dst, dst_size, 1);
    if((! src) || (! state.stream.is_valid)) {
        return 0;
    }
    src_start = src;
    src_end = src + src_size;
    search_end = src_end - 1;
    *dst = *src;
    for(++src; src < search_end; ) {
        // TODO
    }
    // TODO last
    if(! state.stream.is_valid) {
        return 0;
    }
    return state.stream.next - dst;
}

// -----------------------------------------------------------------------

typedef struct {
    ECL_JH_RState stream;
    ECL_usize n_new;
    ECL_ssize offset;
    ECL_usize n_copy;
} ECL_NanoLZ_DecompressorState;

static void ECL_NanoLZ_ReadOpcode(ECL_NanoLZ_DecompressorState* state) {
    uint8_t opcode;
    opcode = ECL_JH_Read(&state->stream, 3);
    switch (opcode) {
    case 0:
        state->n_new = ECL_JH_Read(&state->stream, 2);
        state->offset = ECL_JH_Read(&state->stream, 3) + 1;
        state->n_copy = 2;
        break;
    case 1:
        state->n_new = ECL_JH_Read(&state->stream, 4);
        state->offset = ECL_JH_Read(&state->stream, 4) + 1;
        state->n_copy = 2;
        break;
    case 2:
        state->n_new = ECL_JH_Read(&state->stream, 4);
        state->offset = ECL_JH_Read(&state->stream, 7) + 17;
        state->n_copy = 2;
        break;
    case 3:
        state->n_new = ECL_JH_Read(&state->stream, 7) + 16;
        state->offset = ECL_JH_Read(&state->stream, 4) + 1;
        state->n_copy = 2;
        break;
    case 4:
        state->n_new = ECL_JH_Read(&state->stream, 4);
        state->offset = ECL_JH_Read(&state->stream, 5) + 1;
        state->n_copy = ECL_JH_Read(&state->stream, 3) + 3;
        break;
    case 5:
        state->n_new = ECL_JH_Read(&state->stream, 4);
        state->offset = ECL_JH_Read(&state->stream, 8) + 33;
        state->n_copy = ECL_JH_Read(&state->stream, 3) + 3;
        break;
    case 6:
        state->n_new = ECL_JH_Read_E4(&state->stream);
        state->offset = ECL_JH_Read_E7E4(&state->stream) + 1;
        state->n_copy = ECL_JH_Read_E4(&state->stream) + 2;
        if(state->n_copy == 2) {
            state->n_copy = 0;
        }
        break;
    case 7:
        state->n_new = ECL_JH_Read(&state->stream, 5);
        state->offset = state->n_new + 1;
        state->n_copy = 1;
        break;
    }
}

ECL_usize ECL_NanoLZ_Decompress(const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size) {
    ECL_NanoLZ_DecompressorState state;
    ECL_usize dst_pos;

    ECL_JH_RInit(&state.stream, src, src_size, 1);
    if((! dst) || (! state.stream.is_valid) || (! dst_size)) {
        return 0;
    }
    *dst = *src; // copy first byte as is
    for(dst_pos = 1; dst_pos < dst_size;) {
        ECL_NanoLZ_ReadOpcode(&state);
        if(! state.stream.is_valid) {
            break; // error in stream
        }
        if((state.n_new + state.n_copy) > (dst_size - dst_pos)) {
            break; // output doesn't fit this
        }
        if(state.n_new) {
            const uint8_t* src_block_start = state.stream.next;
            ECL_JH_RJump(&state.stream, state.n_new);
            if(! state.stream.is_valid) {
                break; // don't have enough data in stream
            }
            memcpy(dst + dst_pos, src_block_start, state.n_new);
            dst_pos += state.n_new;
        }
        if(state.offset > dst_pos) {
            break; // points outside of data. error in stream
        }
        if(state.offset >= state.n_copy) { // regular block
            memcpy(dst + dst_pos, dst + dst_pos - state.offset, state.n_copy);
        } else { // interleaved (cycled) block
            if(state.offset == 1) {
                memset(dst + dst_pos, dst[dst_pos - 1], state.n_copy);
            } else {
                uint8_t* ptr_end;
                uint8_t* ptr = dst + dst_pos;
                for(ptr_end = ptr + state.n_copy; ptr < ptr_end; ++ptr) {
                    *ptr = *(ptr - state.offset);
                }
            }
        }
        dst_pos += state.n_copy;
    }
    return dst_pos;
}
