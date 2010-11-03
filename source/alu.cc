#include "includes/virtualmachine.h"
#include "includes/alu.h"

// Macros for checking the PSR
#define N_SET     (vm->_psr & kPSRNBit)
#define N_CLEAR  !(vm->_psr & kPSRNBit)
#define V_SET     (vm->_psr & kPSRVBit)
#define V_CLEAR  !(vm->_psr & kPSRVBit)
#define C_SET     (vm->_psr & kPSRCBit)
#define C_CLEAR  !(vm->_psr & kPSRCBit)
#define Z_SET     (vm->_psr & kPSRZBit)
#define Z_CLEAR  !(vm->_psr & kPSRZBit)

ALU::ALU(VirtualMachine *vm) : _vm(vm)
{}

ALU::~ALU()
{
    _vm = NULL;
}

bool ALU::init()
{
    // Return true if instantiation failed
    if (!vm) return (true);
    
    _carry_out = false;
    return (false);
}

bool ALU::shift(reg_t &offset, reg_t val, reg_t shift, reg_t op)
{
    reg_t mask;
    
    switch (op)
    {
        case kShiftLSL:
        offset = (val << shift);
        // Least significant discarded bit becomes the carry-out
        mask = kOne << (kRegSize - shift - 2);
        if (val & mask)
            return (true);
        return (false);
    
        case kShiftASR:
        offset = (val >> shift);
        // Sign extend by filling in vacant bits with original sign bit
        // Get a mask of the now-empty bits
        mask = kWordMask << (kRegSize - shift);
        
        // If there was a sign bit, make all empty bits 1
        // otherwise leave it as zeros
        if (val & kSignBitMask)
            offset |= mask;
        
        // Then use the MSB of the discarded portion as the carry
        mask = kOne << (shift - 1);
        if (val & mask)
            return (true);
        return (false);
    
        case kShiftLSR:
        // Same thing as LSL, but check the MSB of the discarded portion
        offset = (val >> shift);
        mask = kOne << (shift - 1);
        if (val & mask)
            return (true);
        return (false);
        
        case kShiftROR:
        default:
        // Do a normal rotate right
        offset = (val >> shift) | (val >> (kRegSize - shift));
        // However, selection for the carry bit is the same as in LSR
        mask = kOne << (shift - 1);
        if (val & mask)
            return (true);
        return (false);
    }
}

void ALU::shiftOffset(reg_t &offset)
{
    // Perform the somewhat complex operation of barrel shifting the offset
    // return the carry out of the shift operation
    bool carry_bit = false;
    
    reg_t operation = (offset & kShiftOpMask) >> 5;
    reg_t *value = vm->selectRegister(offset & kShiftRmMask);
    reg_t shift;
    
    if (offset & kShiftTypeMask)
        // We're shifting by the value in a register
        shift = *(vm->selectRegister((offset & kShiftRsMask) >> 7));
    else
        // We're only shifting by an immediate amount
        shift = (offset & kShiftRsMask) >> 7;
    
    // Shifting zero is a special case where you dont touch the C bit
    if (shift == 0)
    {
        offset = *value;
        return;
    }
    
    if (ALU::shift(offset, *value, shift, operation))
        vm->_psr = C_SET;
    else
        vm->_psr = C_CLEAR;
    
    return;
}

cycle_t ALU::dataProcessing(bool I, bool S, char op, char s, char d, reg_t &op2)
{
    cycle_t cycles = 0;
    bool shift_carry = false;
    bool arithmetic = true;
    bool commit = true;
    reg_t dest;
    reg_t *source = vm->selectRegister(s);
    
    switch (op)
    {
        case kADD:
        if (!I) shiftOffset(op2);
        dest = *source + op2;
        cycles += kADDCycles;
        break;
        
        case kSUB:
        arithmetic = true;
        if (!I) shiftOffset(op2);
        dest = *source - op2;
        cycles += kSUBCycles;
        break;
        
        case kMUL:
        arithmetic = true;
        if (!I) shiftOffset(op2);
        dest = ((*source) * op2); //store lowest 32 bits on Rd
        //dest = ((*source) * op2) & 4294967295; //store lowest 32 bits on Rd
        //pq[0] = (*source * op2) >> 32; //shift 32 bits right, to give highest bits
        cycles += kMULCycles;
        break;
        
        case kMOD:
        if (!I) shiftOffset(op2);
        dest = *source % op2;
        cycles += kMODCycles;
        break;
        
        case kDIV:
        if (!I) shiftOffset(op2);
        dest = *source / op2;
        cycles += kDIVCycles;
        break;
        
        case kMOV:
        arithmetic = false;
        if (!I) shiftOffset(op2);
        dest = op2;
        cycles += kMOVCycles;
        break;
        
        case kAND:
        arithmetic = false;
        if (!I) shiftOffset(op2);
        dest = *source & op2;
        cycles += kANDCycles;
        break;
        
        case kORR:
        arithmetic = false;
        if (!I) shiftOffset(op2);
        dest = *source | op2;
        cycles += kORRCycles;
        break;
        
        case kXOR:
        arithmetic = false;
        if (!I) shiftOffset(op2);
        dest = *source ^ op2;
        cycles += kXORCycles;
        break;
        
        case kNOT:
        arithmetic = false;
        if (!I) shiftOffset(op2);
        dest = ~(*source);
        cycles += kNOTCycles;
        break;
        
        case kCMP:
        arithmetic = false;
        commit = false;
        if (!I) shiftOffset(op2);
        dest = *source - op2;
        cycles += kCMPCycles;
        break;
        
        case kCMN:
        arithmetic = false;
        commit = false;
        if (!I) shiftOffset(op2);
        dest = *source + op2;
        cycles += kCMNCycles;
        break;
        
        case kTST:
        arithmetic = false;
        commit = false;
        if (!I) shiftOffset(op2);
        dest = *source & op2;
        cycles += kTSTCycles;
        break;
        
        case kTEQ:
        arithmetic = false;
        commit = false;
        if (!I) shiftOffset(op2);
        dest = *source ^ op2;
        cycles += kTEQCycles;
        break;
        
        case kBIC:
        arithmetic = false;
        if (!I) shiftOffset(op2);
        dest = *source & ~op2;
        cycles += kBICCycles;
        break;
        
        case kNOP:
        default:
        return (kNOPCycles);
    }
    
    // We're done if we're not setting status bits
    // N.B.: Ignore the S bit if dest == PC, so that we dont get status bit
    //       confusion when you branch or something.
    
    // Reset status bits that are effected
    vm->_psr &= ~NVCZ_MASK;
    
    // Set status bits
    // There are two cases, logical and arithmetic
    if ( !(s || d == kPCCode))
    {
        if (arithmetic)
        {
            // Set V if there was an overflow into the 31st bit
            // My understanding of this is that if 31st bit was NOT
            // set in *source, but is in *dest, set V
            if (dest & kMSBMask)
            {
                if (!(*source & kMSBMask))
                    V_SET;
                else
                    V_CLEAR;
            
                // Also, set the negative bit
                N_SET;
            } else {
            
                // Z flag set if dest is zero
                if (dest == 0x0) {
                    Z_SET;
                } else {
                    // As long as we're not zero, we might be super large
                    if (dest < *source)
                        C_SET;
                    else
                        C_CLEAR;
                
                    Z_CLEAR;
                }
            }
        
        } else {
            // We're a logical operation
            // C flag is set to the carry out of the shifter, so do nothing.
        
            // Z flag is set if result is all zeros
            if (dest == 0x0)
                Z_SET;
            else
                Z_CLEAR;
        
            // N flag is set to the logical value of bit 31 of the result
            if (dest & kMSBMask)
                N_SET;
            else
                N_CLEAR;
        
            // (V Flag is uneffected by logical operations)
        }
    }
    
    if (commit)
        *(vm->selectRegister(d)) = dest;
    
    return (cycles);
}


