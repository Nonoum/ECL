// schemes inline file

// scheme 1 coder & decoder --------------------------------------------------------------------------------------
static bool ECL_NanoLZ_Write_Scheme1(ECL_NanoLZ_CompressorState* state) {
    ECL_usize estimate_bits;
    if(! state->n_copy) { // only new bytes
        ECL_JH_Write(&state->stream, 7, 3);
        ECL_JH_Write_E4(&state->stream, state->n_new);
        ECL_JH_Write(&state->stream, 0, 5); // 0 in E4 format
        if(! state->n_new) { // stream alignment opcode for flow mode
            ECL_JH_Write(&state->stream, 0, state->stream.n_bits);
        }
        return true;
    }
    ECL_ASSERT(state->n_copy >= 2);

    // TODO optimize/minimize checks
    if(state->n_copy == 2) {
        if((state->n_new <= 3) && (state->offset <= 8)) { // opcode 0
            ECL_JH_Write(&state->stream, 0, 3);
            ECL_JH_Write(&state->stream, state->n_new, 2);
            ECL_JH_Write(&state->stream, state->offset - 1, 3);
            return true;
        } else if((state->n_new <= 8) && (state->offset <= 16)) { // opcode 1
            ECL_JH_Write(&state->stream, 1, 3);
            if(state->n_new) ECL_JH_Write(&state->stream, (state->n_new - 1) << 1, 4); else ECL_JH_Write(&state->stream, 1, 1);
            ECL_JH_Write(&state->stream, state->offset - 1, 4);
            return true;
        } else if((state->n_new <= 8) && (state->offset <= (16 + 128))) { // opcode 2
            ECL_JH_Write(&state->stream, 2, 3);
            if(state->n_new) ECL_JH_Write(&state->stream, (state->n_new - 1) << 1, 4); else ECL_JH_Write(&state->stream, 1, 1);
            ECL_JH_Write(&state->stream, state->offset - 1 - 16, 7);
            return true;
        } else {
            return false;
        }
    } else {
        if(state->n_copy <= 10) {
            if((state->n_new <= 8) && (state->offset <= 8)) { // opcode 3
                ECL_JH_Write(&state->stream, 3, 3);
                if(state->n_new) ECL_JH_Write(&state->stream, (state->n_new - 1) << 1, 4); else ECL_JH_Write(&state->stream, 1, 1);
                ECL_JH_Write(&state->stream, state->n_copy - 3, 3);
                ECL_JH_Write(&state->stream, state->offset - 1, 3);
                return true;
            } else if((state->n_new <= 8) && (state->offset <= (8 + 64))) { // opcode 4
                ECL_JH_Write(&state->stream, 4, 3);
                if(state->n_new) ECL_JH_Write(&state->stream, (state->n_new - 1) << 1, 4); else ECL_JH_Write(&state->stream, 1, 1);
                ECL_JH_Write(&state->stream, state->n_copy - 3, 3);
                ECL_JH_Write(&state->stream, state->offset - 1 - 8, 6);
                return true;
            } else if((state->n_new <= 8) && (state->offset <= (8 + 64 + 256))) { // opcode 5
                ECL_JH_Write(&state->stream, 5, 3);
                if(state->n_new) ECL_JH_Write(&state->stream, (state->n_new - 1) << 1, 4); else ECL_JH_Write(&state->stream, 1, 1);
                ECL_JH_Write(&state->stream, state->n_copy - 3, 3);
                ECL_JH_Write(&state->stream, state->offset - 1 - 8 - 64, 8);
                return true;
            } else if((state->n_new <= 8) && (state->offset <= (8 + 64 + 256 + 256))) { // opcode 6
                ECL_JH_Write(&state->stream, 6, 3);
                if(state->n_new) ECL_JH_Write(&state->stream, (state->n_new - 1) << 1, 4); else ECL_JH_Write(&state->stream, 1, 1);
                ECL_JH_Write(&state->stream, state->n_copy - 3, 3);
                ECL_JH_Write(&state->stream, state->offset - 1 - 8 - 64 - 256, 8);
                return true;
            }
        }
    }
    estimate_bits = ECL_Evaluate_E4(state->n_new)
                  + ECL_Evaluate_E4(state->n_copy - 2)
                  + ECL_Evaluate_E7E4(state->offset - 1)
                  + 3;
    if(estimate_bits <= (state->n_copy << 3)) {
        ECL_JH_Write(&state->stream, 7, 3);
        ECL_JH_Write_E4(&state->stream, state->n_new);
        ECL_JH_Write_E4(&state->stream, state->n_copy - 2);
        ECL_JH_Write_E7E4(&state->stream, state->offset - 1);
        return true;
    }
    return false;
}

static void ECL_NanoLZ_Read_Scheme1(ECL_NanoLZ_DecompressorState* state) {
    uint8_t opcode;
    opcode = ECL_JH_Read(&state->stream, 3);
    switch (opcode) {
    case 0:
        state->n_new = ECL_JH_Read(&state->stream, 2);
        state->n_copy = 2;
        state->offset = ECL_JH_Read(&state->stream, 3) + 1;
        break;
    case 1:
        state->n_new = ECL_JH_Read(&state->stream, 1) ? 0 : (ECL_JH_Read(&state->stream, 3) + 1);
        state->n_copy = 2;
        state->offset = ECL_JH_Read(&state->stream, 4) + 1;
        break;
    case 2:
        state->n_new = ECL_JH_Read(&state->stream, 1) ? 0 : (ECL_JH_Read(&state->stream, 3) + 1);
        state->n_copy = 2;
        state->offset = ECL_JH_Read(&state->stream, 7) + 1 + 16;
        break;
    case 3:
        state->n_new = ECL_JH_Read(&state->stream, 1) ? 0 : (ECL_JH_Read(&state->stream, 3) + 1);
        state->n_copy = ECL_JH_Read(&state->stream, 3) + 3;
        state->offset = ECL_JH_Read(&state->stream, 3) + 1;
        break;
    case 4:
        state->n_new = ECL_JH_Read(&state->stream, 1) ? 0 : (ECL_JH_Read(&state->stream, 3) + 1);
        state->n_copy = ECL_JH_Read(&state->stream, 3) + 3;
        state->offset = ECL_JH_Read(&state->stream, 6) + 1 + 8;
        break;
    case 5:
        state->n_new = ECL_JH_Read(&state->stream, 1) ? 0 : (ECL_JH_Read(&state->stream, 3) + 1);
        state->n_copy = ECL_JH_Read(&state->stream, 3) + 3;
        state->offset = ECL_JH_Read(&state->stream, 8) + 1 + 8 + 64;
        break;
    case 6:
        state->n_new = ECL_JH_Read(&state->stream, 1) ? 0 : (ECL_JH_Read(&state->stream, 3) + 1);
        state->n_copy = ECL_JH_Read(&state->stream, 3) + 3;
        state->offset = ECL_JH_Read(&state->stream, 8) + 1 + 8 + 64 + 256;
        break;
    case 7:
        state->n_new = ECL_JH_Read_E4(&state->stream);
        state->n_copy = ECL_JH_Read_E4(&state->stream);
        if(state->n_copy) {
            state->n_copy += 2;
            state->offset = ECL_JH_Read_E7E4(&state->stream) + 1;
        }
        break;
    }
}

// scheme 2 coder & decoder --------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------

static ECL_NanoLZ_SchemeCoder ECL_NanoLZ_GetSchemeCoder(ECL_NanoLZ_Scheme scheme) {
    switch(scheme) {
    case ECL_NANOLZ_SCHEME1: return ECL_NanoLZ_Write_Scheme1;
    case ECL_NANOLZ_SCHEME2: return 0;
    default: break;
    }
    return 0;
}

static ECL_NanoLZ_SchemeDecoder ECL_NanoLZ_GetSchemeDecoder(ECL_NanoLZ_Scheme scheme) {
    switch(scheme) {
    case ECL_NANOLZ_SCHEME1: return ECL_NanoLZ_Read_Scheme1;
    case ECL_NANOLZ_SCHEME2: return 0;
    default: break;
    }
    return 0;
}