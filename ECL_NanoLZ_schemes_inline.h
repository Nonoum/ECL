// schemes inline file

// scheme 1 coder & decoder --------------------------------------------------------------------------------------
#if ECL_NANO_LZ_IS_SCHEME_ENABLED(1)

static bool ECL_NanoLZ_Write_Scheme1_copy(ECL_NanoLZ_CompressorState* state) {
    ECL_usize estimate_bits, offset_sub;
    ECL_ASSERT(state->n_copy >= 2);

    if(state->n_copy == 2) {
        if((state->n_new <= 3) && (state->offset <= 8)) { // opcode 0
            // commented-out lines are replaced with single call:
            //ECL_JH_Write(&state->stream, 0, 3);
            //ECL_JH_Write(&state->stream, state->n_new, 2);
            //ECL_JH_Write(&state->stream, state->offset - 1, 3);
            ECL_JH_Write(&state->stream, ((state->n_new << 3) | ((state->offset - 1) << 5)), 8);
            return true;
        } else if((state->n_new <= 8) && (state->offset <= 32)) { // opcode 1
            if(state->n_new) {
                //ECL_JH_Write(&state->stream, 1, 4);
                //ECL_JH_Write(&state->stream, state->n_new - 1, 3);
                ECL_JH_Write(&state->stream, (state->n_new << 4) + 0xF1, 7);
            } else {
                ECL_JH_Write(&state->stream, 0x9, 4);
            }
            ECL_JH_Write(&state->stream, state->offset - 1, 5);
            return true;
        } else if((state->n_new <= 8) && (state->offset <= (32 + 128))) { // opcode 2
            if(state->n_new) {
                //ECL_JH_Write(&state->stream, 2, 4);
                //ECL_JH_Write(&state->stream, state->n_new - 1, 3);
                ECL_JH_Write(&state->stream, (state->n_new << 4) + 0xF2, 7);
            } else {
                ECL_JH_Write(&state->stream, 0xA, 4);
            }
            ECL_JH_Write(&state->stream, state->offset - 1 - 32, 7);
            return true;
        } else {
            return false;
        }
    } else {
        offset_sub = 1;
        if((state->n_copy <= 10) && (state->n_new <= 8)) {
            if(state->offset <= 8) { // opcode 3
                if(state->n_new) {
                    //ECL_JH_Write(&state->stream, 3, 4);
                    //ECL_JH_Write(&state->stream, state->n_new - 1, 3);
                    ECL_JH_Write(&state->stream, (state->n_new << 4) + 0xF3, 7);
                } else {
                    ECL_JH_Write(&state->stream, 0xB, 4);
                }
                ECL_JH_Write(&state->stream, state->n_copy - 3, 3);
                ECL_JH_Write(&state->stream, state->offset - 1, 3);
                return true;
            } else if(state->offset <= (8 + 32)) { // opcode 4
                if(state->n_new) {
                    //ECL_JH_Write(&state->stream, 4, 4);
                    //ECL_JH_Write(&state->stream, state->n_new - 1, 3);
                    ECL_JH_Write(&state->stream, (state->n_new << 4) + 0xF4, 7);
                } else {
                    ECL_JH_Write(&state->stream, 0xC, 4);
                }
                ECL_JH_Write(&state->stream, state->n_copy - 3, 3);
                ECL_JH_Write(&state->stream, state->offset - 1 - 8, 5);
                return true;
            } else if(state->offset <= (8 + 32 + 64)) { // opcode 5
                if(state->n_new) {
                    //ECL_JH_Write(&state->stream, 5, 4);
                    //ECL_JH_Write(&state->stream, state->n_new - 1, 3);
                    ECL_JH_Write(&state->stream, (state->n_new << 4) + 0xF5, 7);
                } else {
                    ECL_JH_Write(&state->stream, 0xD, 4);
                }
                ECL_JH_Write(&state->stream, state->n_copy - 3, 3);
                ECL_JH_Write(&state->stream, state->offset - 1 - 8 - 32, 6);
                return true;
            } else if(state->offset <= (8 + 32 + 64 + 256)) { // opcode 6
                if(state->n_new) {
                    //ECL_JH_Write(&state->stream, 6, 4);
                    //ECL_JH_Write(&state->stream, state->n_new - 1, 3);
                    ECL_JH_Write(&state->stream, (state->n_new << 4) + 0xF6, 7);
                } else {
                    ECL_JH_Write(&state->stream, 0xE, 4);
                }
                ECL_JH_Write(&state->stream, state->n_copy - 3, 3);
                ECL_JH_Write(&state->stream, state->offset - 1 - 8 - 32 - 64, 8);
                return true;
            } else {
                offset_sub = 1 + 8 + 32 + 64 + 256;
            }
        }
    }
    estimate_bits = (state->n_new ? ECL_Evaluate_E2(state->n_new - 1) : 0)
                  + ECL_Evaluate_E3(state->n_copy - 2)
                  + ECL_Evaluate_E6E3(state->offset - offset_sub)
                  + (3 + 1);
    if(estimate_bits <= (state->n_copy << 3)) {
        if(! state->n_new) {
            ECL_JH_Write(&state->stream, 0x0F, 4);
        } else {
            ECL_JH_Write(&state->stream, 0x07, 4);
            ECL_JH_Write_E2(&state->stream, state->n_new - 1);
        }
        ECL_JH_Write_E3(&state->stream, state->n_copy - 2);
        ECL_JH_Write_E6E3(&state->stream, state->offset - offset_sub);
        return true;
    }
    return false;
}

static bool ECL_NanoLZ_Write_Scheme1_nocopy(ECL_NanoLZ_CompressorState* state) {
    ECL_ASSERT(! state->n_copy);
    if(! state->n_new) {
        ECL_JH_Write(&state->stream, 0x0F, 8); // glued {111, 1, (0000 = 0 in E3 format)}
        // stream alignment opcode for flow mode (reserved for possible addition of streaming modes)
        ECL_JH_Write(&state->stream, 0, state->stream.n_bits);
    } else {
        ECL_JH_Write(&state->stream, 0x07, 4);
        ECL_JH_Write_E2(&state->stream, state->n_new - 1);
        ECL_JH_Write(&state->stream, 0, 4); // 0 in E3 format
    }
    return true;
}

static void ECL_NanoLZ_Read_Scheme1(ECL_NanoLZ_DecompressorState* state) {
    uint8_t tmp;
    const uint8_t opcode = ECL_JH_Read(&state->stream, 3);
    ECL_NANO_LZ_COUNTER_APPEND(opcode, 1)
    switch (opcode) {
    case 0:
        tmp = ECL_JH_Read(&state->stream, 5);
        state->n_new = tmp & 0x03;
        state->n_copy = 2;
        state->offset = (tmp >> 2);
        break;
    case 1:
        state->n_new = ECL_JH_Read(&state->stream, 1) ? 0 : (ECL_JH_Read(&state->stream, 3) + 1);
        state->n_copy = 2;
        state->offset = ECL_JH_Read(&state->stream, 5);
        break;
    case 2:
        state->n_new = ECL_JH_Read(&state->stream, 1) ? 0 : (ECL_JH_Read(&state->stream, 3) + 1);
        state->n_copy = 2;
        state->offset = ECL_JH_Read(&state->stream, 7) + 32;
        break;
    case 3:
        state->n_new = ECL_JH_Read(&state->stream, 1) ? 0 : (ECL_JH_Read(&state->stream, 3) + 1);
        tmp = ECL_JH_Read(&state->stream, 6);
        state->n_copy = (tmp & 0x07) + 3;
        state->offset = (tmp >> 3);
        break;
    case 4:
        state->n_new = ECL_JH_Read(&state->stream, 1) ? 0 : (ECL_JH_Read(&state->stream, 3) + 1);
        tmp = ECL_JH_Read(&state->stream, 8);
        state->n_copy = (tmp & 0x07) + 3;
        state->offset = (tmp >> 3) + 8;
        break;
    case 5:
        state->n_new = ECL_JH_Read(&state->stream, 1) ? 0 : (ECL_JH_Read(&state->stream, 3) + 1);
        state->n_copy = ECL_JH_Read(&state->stream, 3) + 3;
        state->offset = ECL_JH_Read(&state->stream, 6) + 8 + 32;
        break;
    case 6:
        state->n_new = ECL_JH_Read(&state->stream, 1) ? 0 : (ECL_JH_Read(&state->stream, 3) + 1);
        state->n_copy = ECL_JH_Read(&state->stream, 3) + 3;
        state->offset = ECL_JH_Read(&state->stream, 8) + 8 + 32 + 64;
        break;
    case 7:
        state->n_new = ECL_JH_Read(&state->stream, 1) ? 0 : (ECL_JH_Read_E2(&state->stream) + 1);
        state->n_copy = ECL_JH_Read_E3(&state->stream);
        if(state->n_copy) {
            state->n_copy += 2;
            state->offset = ECL_JH_Read_E6E3(&state->stream);
            if((state->n_copy <= 10) && (state->n_new <= 8)) {
                state->offset += 8 + 32 + 64 + 256;
            }
        }
        break;
    }
    ++(state->offset);
    ECL_NANO_LZ_COUNTER_APPEND(8, state->n_new)
}

#endif
// scheme 2 coder & decoder --------------------------------------------------------------------------------------
#if ECL_NANO_LZ_IS_SCHEME_ENABLED(2)

static bool ECL_NanoLZ_Write_Scheme2_copy(ECL_NanoLZ_CompressorState* state) {
    ECL_ASSERT(state->n_copy >= 2);
    if(state->n_copy >= 5) { // ignore smaller match - this weakens compression and increases decompression speed
        // we estimated roughly - in some cases it can result in compressed output > original data
        ECL_JH_Write_E3(&state->stream, state->n_new);
        ECL_JH_Write_E3(&state->stream, state->n_copy - 4);
        ECL_JH_Write_E7(&state->stream, state->offset - 1);
        return true;
    }
    return false;
}

static bool ECL_NanoLZ_Write_Scheme2_nocopy(ECL_NanoLZ_CompressorState* state) {
    ECL_ASSERT(! state->n_copy);
    ECL_JH_Write_E3(&state->stream, state->n_new);
    ECL_JH_Write_E3(&state->stream, 0); // n_copy = 0
    return true;
}

static void ECL_NanoLZ_Read_Scheme2(ECL_NanoLZ_DecompressorState* state) {
    state->n_new = ECL_JH_Read_E3(&state->stream);
    state->n_copy = ECL_JH_Read_E3(&state->stream);
    if(state->n_copy) { // 0 is 0 and assumes no offset encoded (last block in stream)
        state->n_copy += 4;
        state->offset = ECL_JH_Read_E7(&state->stream) + 1;
    }
}

#endif
// ---------------------------------------------------------------------------------------------------------------

// returns a coder that expects only sequences with n_copy >= 2
static ECL_NanoLZ_SchemeCoder ECL_NanoLZ_GetSchemeCoder(ECL_NanoLZ_Scheme scheme) {
    switch(scheme) {
#if ECL_NANO_LZ_IS_SCHEME_ENABLED(1)
    case ECL_NANOLZ_SCHEME1: return ECL_NanoLZ_Write_Scheme1_copy;
#endif
#if ECL_NANO_LZ_IS_SCHEME_ENABLED(2)
    case ECL_NANOLZ_SCHEME2_DEMO: return ECL_NanoLZ_Write_Scheme2_copy;
#endif
    default: break;
    }
    return 0;
}

// returns a coder that expects only sequences with n_copy == 0
static ECL_NanoLZ_SchemeCoder ECL_NanoLZ_GetSchemeCoderNoCopy(ECL_NanoLZ_Scheme scheme) {
    switch(scheme) {
#if ECL_NANO_LZ_IS_SCHEME_ENABLED(1)
    case ECL_NANOLZ_SCHEME1: return ECL_NanoLZ_Write_Scheme1_nocopy;
#endif
#if ECL_NANO_LZ_IS_SCHEME_ENABLED(2)
    case ECL_NANOLZ_SCHEME2_DEMO: return ECL_NanoLZ_Write_Scheme2_nocopy;
#endif
    default: break;
    }
    return 0;
}

// returns a decoder
static ECL_NanoLZ_SchemeDecoder ECL_NanoLZ_GetSchemeDecoder(ECL_NanoLZ_Scheme scheme) {
    switch(scheme) {
#if ECL_NANO_LZ_IS_SCHEME_ENABLED(1)
    case ECL_NANOLZ_SCHEME1: return ECL_NanoLZ_Read_Scheme1;
#endif
#if ECL_NANO_LZ_IS_SCHEME_ENABLED(2)
    case ECL_NANOLZ_SCHEME2_DEMO: return ECL_NanoLZ_Read_Scheme2;
#endif
    default: break;
    }
    return 0;
}
