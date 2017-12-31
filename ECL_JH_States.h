#ifndef ECL_JH_STATES_H_
#define ECL_JH_STATES_H_

#include "ECL_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
    ECL Jumping Header states for writing and reading.
    - lower bits go first
    - you can continue calling read/write safely even if state has invalidated, but only if *Init method succeeded
    - reading/writing on invalid buffer is safe but result is undefined
*/
typedef struct {
    uint8_t* byte;
    uint8_t* next;
    uint8_t* end;
    uint8_t n_bits;
    uint8_t is_valid; // to check after any transaction
} ECL_JH_WState;

typedef struct {
    const uint8_t* byte;
    const uint8_t* next;
    const uint8_t* end;
    uint8_t n_bits;
    uint8_t is_valid; // to check after any transaction
} ECL_JH_RState;


void ECL_JH_WInit(ECL_JH_WState* state, uint8_t* ptr, ECL_usize size, ECL_usize start); // state's constructor
void ECL_JH_RInit(ECL_JH_RState* state, const uint8_t* ptr, ECL_usize size, ECL_usize start); // -:-
void ECL_JH_Write(ECL_JH_WState* state, uint8_t value, uint8_t bits); // writes 'bits' bits of 'value' to 'state'
uint8_t ECL_JH_Read(ECL_JH_RState* state, uint8_t bits); // returns value of 'bits' size read from 'state'
void ECL_JH_WJump(ECL_JH_WState* state, ECL_usize distance); // moves pointer to next byte at 'distance' if possible
void ECL_JH_RJump(ECL_JH_RState* state, ECL_usize distance); // -:-

// E-numbers part
void ECL_JH_Write_E2(ECL_JH_WState* state, ECL_usize value); // writes 'value' to 'state' in E2 number format
ECL_usize ECL_JH_Read_E2(ECL_JH_RState* state); // reads from 'state' a value in E2 number format
uint8_t ECL_Evaluate_E2(ECL_usize number); // returns amount of bits required for coding 'number' in E2 format

void ECL_JH_Write_E4E5(ECL_JH_WState* state, ECL_usize value); // -:-
ECL_usize ECL_JH_Read_E4E5(ECL_JH_RState* state); // -:-
uint8_t ECL_Evaluate_E4E5(ECL_usize number); // -:-

// other numbers are declared via macros below

#define ECL_E_NUMBER_DECLARE_SIMPLE(num) \
    void ECL_JH_Write_E##num(ECL_JH_WState* state, ECL_usize value); \
    ECL_usize ECL_JH_Read_E##num(ECL_JH_RState* state); \
    uint8_t ECL_Evaluate_E##num(ECL_usize number);

ECL_E_NUMBER_DECLARE_SIMPLE(3)
ECL_E_NUMBER_DECLARE_SIMPLE(4)
ECL_E_NUMBER_DECLARE_SIMPLE(5)
ECL_E_NUMBER_DECLARE_SIMPLE(6)
ECL_E_NUMBER_DECLARE_SIMPLE(7)

#undef ECL_E_NUMBER_DECLARE_SIMPLE

#define ECL_E_NUMBER_DECLARE_X2(num1, num2) \
    void ECL_JH_Write_E##num1##E##num2(ECL_JH_WState* state, ECL_usize value); \
    ECL_usize ECL_JH_Read_E##num1##E##num2(ECL_JH_RState* state); \
    uint8_t ECL_Evaluate_E##num1##E##num2(ECL_usize number);

ECL_E_NUMBER_DECLARE_X2(5, 2)
ECL_E_NUMBER_DECLARE_X2(5, 3)
ECL_E_NUMBER_DECLARE_X2(5, 4)
ECL_E_NUMBER_DECLARE_X2(6, 2)
ECL_E_NUMBER_DECLARE_X2(6, 3)
ECL_E_NUMBER_DECLARE_X2(6, 4)
ECL_E_NUMBER_DECLARE_X2(7, 2)
ECL_E_NUMBER_DECLARE_X2(7, 3)
ECL_E_NUMBER_DECLARE_X2(7, 4)

#undef ECL_E_NUMBER_DECLARE_X2


#ifdef __cplusplus
}
#endif

#endif
