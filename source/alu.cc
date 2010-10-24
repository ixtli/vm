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

void ALU::shiftOffset(reg_t &offset, bool immediate)
{
    // Perform the somewhat complex operation of barrel shifting the offset
    // return the carry out of the shift operation
    bool carry_bit = false;
    
    // Simplest case is when the offset is treated like an immediate
    if (immediate)
    {
        // Lower 8 bits of op2 is an immediate value
        reg_t imm = (offset & kShiftImmediateMask);
        // Upper two bits is a rotate value between 0 and 4
        reg_t ror = (offset & kShiftRotateMask) >> 8;
        
        // If the following is one op, then checking for zero would
        // just be more code for no added efficiency
        
        // Apparently most compilers will recognize the following idiom
        // and compile it into a single ROL operation 
        imm = (imm >> ror) | (imm << (kRegSize - ror));
        
        // Store the value
        offset = imm;
        
        // If we're rotating then there is no carry
    } else {
        // We need to do a more complex operation
        reg_t operation = (offset & kShiftOpMask) >> 5;
        reg_t *value = vm->selectRegister(offset & kShiftRmMask);
        reg_t *shift = vm->selectRegister((offset & kShiftRsMask) >> 7);
        reg_t mask;
        
        // Shifting zero is a special case where you dont touch the C bit
        if (*shift == 0)
        {
            offset = *value;
            return;
        }
    
        switch (operation)
        {
            case kShiftLSL:
            offset = (*value << *shift);
            // Least significant discarded bit becomes the carry-out
            mask = kOne << (kRegSize - *shift - 2);
            if (*value & mask)
                carry_bit = true;
            break;
        
            case kShiftASR:
            offset = (*value >> *shift);
            // Sign extend by filling in vacant bits with original sign bit
            // Get a mask of the now-empty bits
            mask = kWordMask << (kRegSize - *shift);
            
            // If there was a sign bit, make all empty bits 1
            // otherwise leave it as zeros
            if (*value & kSignBitMask)
                offset |= mask;
            
            // Then use the MSB of the discarded portion as the carry
            mask = kOne << (*shift - 1);
            if (*value & mask)
                carry_bit = true;
            break;
        
            case kShiftLSR:
            // Same thing as LSL, but check the MSB of the discarded portion
            offset = (*value >> *shift);
            mask = kOne << (*shift - 1);
            if (*value & mask)
                carry_bit = true;
            break;
            
            case kShiftROR:
            default:
            // Do a normal rotate right
            offset = (*value >> *shift) | (*value >> (kRegSize - *shift));
            // However, selection for the carry bit is the same as in LSR
            mask = kOne << (*shift - 1);
            if (*value & mask)
                carry_bit = true;
            break;
        }
    }
    
    if (carry_bit)
        vm->_psr = C_SET;
    else
        vm->_psr = C_CLEAR;
    
    return;
}

size_t ALU::dataProcessing(bool I, bool S, char op, char s, char d, reg_t &op2)
{
    size_t cycles = 0;
    bool shift_carry = false;
    bool arithmetic = false;
    reg_t *dest = vm->selectRegister(d);
    reg_t *source = vm->selectRegister(s);
    
    switch (op)
    {
        case kADD:
        arithmetic = true;
        shiftOffset(op2, I);
        *dest = *source + op2;
        cycles += kADDCycles;
        break;
        
        case kAND:
        arithmetic = false;
        shiftOffset(op2, I);
        *dest = *source & op2;
        cycles += kANDCycles;
        break;
        
        case kNOP:
        default:
        return (kNOPCycles);
    }
    
    // We're done if we're not setting status bits
    // N.B.: Ignore the S bit if dest == PC, so that we dont get status bit
    //       confusion when you branch or something.
    if (!s || d == kPCCode) return (cycles);
    
    // Reset status bits that are effected
    vm->_psr &= ~NVCZ_MASK;
    
    // Set status bits
    // There are two cases, logical and arithmetic
    if (arithmetic)
    {
        // Set V if there was an overflow into the 31st bit
        // My understanding of this is that if 31st bit was NOT
        // set in *source, but is in *dest, set V
        if (*dest & kMSBMask)
        {
            if (!(*source & kMSBMask))
                vm->_psr |= kPSRVBit;
            
            // Also, set the negative bit
            vm->_psr |= kPSRNBit;
        } else {
            
            // Z flag set if dest is zero
            if (*dest == 0x0) {
                vm->_psr |= kPSRZBit;
            } else {
                // As long as we're not zero, we might be super large
                // The C flag is the carry out of bit 31
                if (*dest < *source)
                {
                    vm->_psr |= kPSRCBit;
                }
            }
        }
        
    } else {
        // We're a logical operation
        // C flag is set to the carry out of the shifter, so do nothing.
        
        // Z flag is set if result is all zeros
        if (*dest == 0x0)
            vm->_psr |= kPSRZBit;
        
        // N flag is set to the logical value of bit 31 of the result
        if (*dest & kMSBMask)
            vm->_psr |= kPSRNBit;
        
        // (V Flag is uneffected by logical operations)
    }
    return (cycles);
}


