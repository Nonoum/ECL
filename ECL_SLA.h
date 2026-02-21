#pragma once
#include "Utils.h"
/*************************************************************************************
// -------------------------------------------------------------------------------- //
// ----------------------- SEQUENCE LEVELS ANALYZER LIBRARY ----------------------- //
// --------------------- Copyright (C) 2012  Evstratov Evgeniy -------------------- //
// -------------------------------------------------------------------------------- //
*************************************************************************************/

namespace SequenceLevelsAnalyzer {

// SLA 8-bit codec

extern const char SLA8_FLAWLESS_HEADER;

ECL_usize Analyze(const uint8_t* src, ECL_usize size, char* dst_header);
/*	'Analyze' makes analysis of (src) with (size) samples by 8 bits each.
	returns best FULL-bit count for compressed block (including header),
	and writes header signature in (*dst_header) contained 1 byte.
*/

void Compress(const uint8_t* src, ECL_usize size, char pre_calc_header, void* dst, ECL_usize* dst_byte, uint8_t* dst_bit);
/*	'Compress' makes COMPLETE-stream from (size) samples of (src) by 8 bits to (dst) from position
	(byte = *dst_byte, bit = *dst_bit).
	uses (pre_calc_header) that was created before.
	if (pre_calc_header) is wrong then stream can DAMAGE information.
	stream DOESN'T contain information about size of input or output data.
*/

void Decompress(const void* src, ECL_usize* src_byte, uint8_t* src_bit, uint8_t* dst, ECL_usize size);
/*	'Decompress' repacks complete-stream from (src) from position (byte = *src_byte, bit = *src_bit)
	to (dst) expecting that there is (size) of encoded samples by 8-bit each.
*/

}; // end SequenceLevelsAnalyzer
