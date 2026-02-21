#include "head.h"
#include <cstdlib> // 'NULL' declaration problems
#include <algorithm>
#include <assert.h>

/*************************************************************************************
// Sequence Levels Analyzer (SLA) algorithm (c) Evgeniy Evstratov, 2012.
// Methods encode differencial signal best
// Methods don't like noise
// Analyzer should be called before coding (and return header in parameters, for coder)

	uses by 3 bits per coding high and low level in header.
	highlevel and lowlevel are decremented values of bit-counts per code without level determination bit (LDB),
	meaning (example): highlevel = 3, then high leveled code is [level determination bit][4 lower bits of data].
	1. highlevel > lowlevel -> default situation, +1 bit before code for determine high/or low level is now; (not using 0-bits coding)
	2. highlevel = lowlevel -> special coding, determination bit is not needed; (not using 0-bits coding)
	Extensions:
	3. highlevel = 0, lowlevel > 1 (2/3/4/5/6/7) -> high level is really 0 bits, lowlevel is increased:
	[level determination bit][(lowlevel-1) bits of data]) ; (using 0-bits coding).
	4. highlevel = 0, lowlevel = 1 -> means stream of zeroes (no data to write, except header).
	5. highlevel = 5, lowlevel > 5 (6,7) -> high level is 0 bits, lowlevel is DEcreased:
	[level determination bit][(lowlevel+1) bits of data]) ; (using 0-bits coding).

SLA header codes table (in cell - variant, or '*' if not used):
	    highlevel ->
	    | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
	  |=|================================
	l |0| 2 | 1 | 1 | 1 | 1 | 1 | 1 | 1 |
	o -----------------------------------
	w |1| 4 | 2 | 1 | 1 | 1 | 1 | 1 | 1 |
	l -----------------------------------
	e |2| 3 | * | 2 | 1 | 1 | 1 | 1 | 1 |
	v -----------------------------------
	e |3| 3 | * | * | 2 | 1 | 1 | 1 | 1 |
	l -----------------------------------
	  |4| 3 | * | * | * | 2 | 1 | 1 | 1 |
	| -----------------------------------
	\/|5| 3 | * | * | * | * | 2 | 1 | 1 |
	  -----------------------------------
	  |6| 3 | * | * | * | * | 5 | 2 | 1 |
	  -----------------------------------
	  |7| 3 | * | * | * | * | 5 | * | 2 |
	  ===================================
	19 free cells.
*************************************************************************************/

namespace SequenceLevelsAnalyzer {

uint8_t GetLog2s(uint16_t x) {
	if(!x) {
		return 0;
	}
	uint16_t mask = 0x8000;
	uint8_t cnt = 16;
	while(1) {
		if(x & mask) {
			return cnt;
		}
		cnt--;
		mask >>= 1;
	}
	return 0; // useless command, warning suppression
}; // end get log2 short

const char SLA8_FLAWLESS_HEADER = 0x3F;

static uint32_t addbits[8] = {0xFE,0xFC,0xF8,0xF0,0xE0,0xC0,0x80,0x00}; // map of sign-extension by value's bit count. used for decoding

inline char MAKEHEADER(uint8_t high, uint8_t low) {
	return char( ((high)&0x07) | (((low)&0x07)<<3) );
};

inline uint8_t GETHIGHLEVEL(char head) {
	return head & 0x07;
};

inline uint8_t GETLOWLEVEL(char head) {
	return (head>>3) & 0x07;
};

class SLA : private NonCopyable {
	uint8_t * table;
	SLA() {
		table = new uint8_t[256];
		for(uint16_t i = 1; i < 256; ++i) {
			uint16_t j = (i >= 128) ? 255-i : i;
			table[i] = GetLog2s(j) + 1; // real bits count
		}
		table[0] = 0;
	};
	~SLA() {
		delete [] table;
		table = NULL;
	};
public:
	static inline const uint8_t* getSLATable() {
		static SLA initializer;
		return initializer.table;
	};
};

// CODE ----------------------------------------------------------------------

ECL_usize Analyze(const uint8_t* src, ECL_usize size, char* dst_header) {
	assert((src != NULL) && "NULL as source data to analyze");
	// returns size in bits : 3 + 3 + data_size
	ECL_usize bits, bbest, bact; // total bits ; bits best ; bits actual
	uint8_t highlevel, lowlevel;
	bits = 3 + 3;
	uint32_t cntrs[9] = {0}; // 0..8-bits counters
	if(! size) {
		return bits;
	}
	const uint8_t* sla_table = SLA::getSLATable();

	for(ECL_usize index = 0; index < size; ++index) {
		++cntrs[sla_table[src[index]]];
	} // statistic done
	char i;
	for(i = 8; i >= 0; --i) {
		if(cntrs[i]) {
			break;
		}
	}
	highlevel = i; // high level is known
	// transforming cntrs
	for(i = 1; i < highlevel; ++i) {
		cntrs[i] += cntrs[i-1];
	}
	// done. highlevel contains REAL BIT COUNT per data (excluding possible LDB)
	if(highlevel == 0) { // all zeroes
		*dst_header = MAKEHEADER(0, 1); // variant 4, only header
		return bits;
	} // further this variant is excluded
	lowlevel = highlevel;
	bbest = size * highlevel; // if all - same-leveled (high = low, variant 2)
	for(i = highlevel - 1; i >= 0; --i) {
		bact = cntrs[i] * (1+i); // count * (LDB + value)
		bact += (size - cntrs[i]) * (1 + highlevel); // high-leveled values
		if(bact < bbest) {
			bbest = bact;
			lowlevel = i;
		}
	}
	bits += bbest;
	if(lowlevel == 0) {
		if(highlevel < 7) { // variant 3
			*dst_header = MAKEHEADER(0, highlevel+1);
		} else { // variant 5
			*dst_header = MAKEHEADER(5, highlevel-1);
		}
	} else {
		*dst_header = MAKEHEADER(highlevel-1, lowlevel-1); // variant 1
	}
	return bits;
}; // end analyze 8-bit ----------------------------------------------------------------------------------

void Compress(const uint8_t* src, ECL_usize size, char pre_calc_header, void* dst, ECL_usize* dst_byte, uint8_t* dst_bit) {
	assert((src != NULL) && "NULL as source data to compress");
	uint8_t lowl, highl, LLm1, HLm1;
	HLm1 = GETHIGHLEVEL(pre_calc_header);
	LLm1 = GETLOWLEVEL(pre_calc_header);
	WriteBits(dst, dst_byte, dst_bit, 6, pre_calc_header);

	if(LLm1 > HLm1) { // extension
		if(LLm1 == 1) {
			return; // variant 4
		}
		if(HLm1 == 0) { // variant 3
			lowl = LLm1; // including LDB
		} else {
			if(HLm1 == 5) { // variant 5
				lowl = LLm1+2; // including LDB
			}
		}
		for(ECL_usize i = 0; i < size; ++i) {
			if(src[i] != 0) { // as low level
				WriteBits(dst, dst_byte, dst_bit, lowl, (((uint32_t)src[i]) << 1) | 0x001); // LDB = 1
			} else {
				WriteBits(dst, dst_byte, dst_bit, 1, 0); // LDB = 0
			}
		}
	} else if(HLm1 != LLm1) { // variant 1
		lowl = LLm1 + 2; // bits per low-level value + LDB
		highl = HLm1 + 2; // bits per high-level value + LDB
		const uint8_t* sla_table = SLA::getSLATable();
		const uint8_t levels[2] = {highl, lowl};
		for(ECL_usize i = 0; i < size; ++i) {
			const uint8_t is_low = uint8_t(sla_table[src[i]] - lowl) >> 7;
			WriteBits(dst, dst_byte, dst_bit, levels[is_low], (((uint32_t)src[i]) << 1) | is_low);
		}
	} else { // same level, variant 2
		highl = HLm1 + 1; // bits per any value
		for(ECL_usize i = 0; i < size; ++i) {
			WriteBits(dst, dst_byte, dst_bit, highl, src[i]);
		}
	}
}; // end compress 8-bit ----------------------------------------------------------------------------------

void Decompress(const void* src, ECL_usize* src_byte, uint8_t* src_bit, uint8_t* dst, ECL_usize size) {
	assert((src != NULL) && "NULL as source data to decompress");
	uint8_t HLm1, LLm1;
	uint8_t lowl, highl, head; // low level bits, high level bits, header
	head = ReadBits(src, src_byte, src_bit, 6);
	highl = HLm1 = GETHIGHLEVEL(head);
	lowl = LLm1 = GETLOWLEVEL(head);

	if(LLm1 > HLm1) { // extension
		if(LLm1 == 1) {
			std::fill(dst, dst + size, 0);
			return;
		}
		if(HLm1 == 0) { // variant 3
			lowl = LLm1 - 1;
		} else {
			if(HLm1 == 5) { // variant 5
				lowl = LLm1 + 1;
			}
		}
		LLm1 = lowl - 1;
		for(ECL_usize i = 0; i < size; ++i) {
			if(ReadBits(src, src_byte, src_bit, 1)) { // low-level
				const uint8_t value = ReadBits(src, src_byte, src_bit, lowl);
				dst[i] = value | (-(value >> LLm1) & addbits[LLm1]); // hardcore expanding with sign-bit
			} else {
				dst[i] = 0; // high-level, simply zero
			}
		}
	} else if(HLm1 != LLm1) { // standard coding, variant 1
		++highl;
		++lowl;
		const uint8_t to_read[2] = {highl, lowl};
		const uint8_t to_check[2] = {HLm1, LLm1};
		for(ECL_usize i = 0; i < size; ++i) {
			const uint8_t LDB = ReadBits(src, src_byte, src_bit, 1);
			const uint8_t value = ReadBits(src, src_byte, src_bit, to_read[LDB]);
			const auto nbits = to_check[LDB];
			dst[i] = value | (-(value >> nbits) & addbits[nbits]);
		}
	} else { // special coding, variant 2
		++highl;
		for(ECL_usize i = 0; i < size; ++i) {
			const uint8_t value = ReadBits(src, src_byte, src_bit, highl);
			dst[i] = value | (-(value >> HLm1) & addbits[HLm1]);
		}
	}
}; // end decompress 8-bit ----------------------------------------------------------------------------------

}; // end SequenceLevelsAnalyzer
