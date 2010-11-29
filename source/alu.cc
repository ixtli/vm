#include "includes/virtualmachine.h"
#include "includes/alu.h"
#include "includes/pipeline.h"

// Macros for checking the PSR
#define N_SET     (_vm->_psr & kPSRNBit)
#define N_CLEAR  !(_vm->_psr & kPSRNBit)
#define V_SET     (_vm->_psr & kPSRVBit)
#define V_CLEAR  !(_vm->_psr & kPSRVBit)
#define C_SET     (_vm->_psr & kPSRCBit)
#define C_CLEAR  !(_vm->_psr & kPSRCBit)
#define Z_SET     (_vm->_psr & kPSRZBit)
#define Z_CLEAR  !(_vm->_psr & kPSRZBit)

// Macros for SETTING the PSR
#define SET_N     (_vm->_psr |= kPSRNBit)
#define CLEAR_N   (_vm->_psr &= ~kPSRNBit)
#define SET_V     (_vm->_psr |= kPSRVBit)
#define CLEAR_V   (_vm->_psr &= ~kPSRVBit)
#define SET_C     (_vm->_psr |= kPSRCBit)
#define CLEAR_C   (_vm->_psr &= ~kPSRCBit)
#define SET_Z     (_vm->_psr |= kPSRZBit)
#define CLEAR_Z   (_vm->_psr &= ~kPSRZBit)

ALU::ALU(VirtualMachine *vm) : _vm(vm)
{}

ALU::~ALU()
{
    _vm = NULL;
}

bool ALU::init(ALUTimings &timing)
{
    // Return true if instantiation failed
    if (!_vm) return (true);
    
    // Copy the supplied timing struct
    _timing.copy(timing);
    
    // Init the carry_out
    _carry_out = false;
    _result = false;
    _output = 0x0;
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

void ALU::shiftOffset(reg_t &offset, reg_t &val)
{
    // Perform the somewhat complex operation of barrel shifting the offset
    // But use the special format for the MOV operation
    reg_t operation = (offset & kShiftOp);
    reg_t shift;
    
    if (offset & kShiftType)
        // Shifting a value in a register
        shift = _vm->selectRegister((offset & kMOVShiftRs) >> 3);
    else
        // Shifting an immediate
        shift = (offset & kMOVLiteral) >> 3;
    
    // Shifting zero is a special case where you dont touch the C bit
    if (shift == 0)
    {
        offset = val;
        return;
    }
    
    if (ALU::shift(offset, val, shift, operation))
        _vm->_psr = C_SET;
    else
        _vm->_psr = C_CLEAR;
}

void ALU::shiftOffset(reg_t &offset)
{
    // Perform the somewhat complex operation of barrel shifting the offset
    reg_t operation = (offset & kShiftOp);
    reg_t shift, value;
    
    // This is the standard case, get embedded val
    value = _vm->selectRegister((offset & kShiftRmMask) >> 3);
    
    if (offset & kShiftType)
        // We're shifting by the value in a register
        shift = _vm->selectRegister((offset & kShiftRsMask) >> 7);
    else
        // We're only shifting by an immediate amount
        shift = (offset & kShiftRsMask) >> 7;
    
    // Shifting zero is a special case where you dont touch the C bit
    if (shift == 0)
    {
        offset = value;
        return;
    }
    
    if (ALU::shift(offset, value, shift, operation))
        _vm->_psr = C_SET;
    else
        _vm->_psr = C_CLEAR;
}

cycle_t ALU::dataProcessing(DPFlags &instruction)
{
    bool arithmetic = true;
    bool commit = true;
    bool alu_carry = false;
    reg_t dest;
    reg_t source = _vm->selectRegister(instruction.rs);
    
    switch (instruction.op)
    {
        case kADD:
        if (!instruction.i) shiftOffset(instruction.offset);
        dest = source + instruction.offset;
        // check if there would have been a carry out (a+b<a)
        if (dest < source) alu_carry = true;
        break;
        
        case kSUB:
        if (!instruction.i) shiftOffset(instruction.offset);
        dest = source - instruction.offset;
        // check if there would have been a carry out (a-b>a)
        if (dest > source) alu_carry = true;
        break;
        
        case kMUL:
        if (!instruction.i) shiftOffset(instruction.offset);
        dest = ((source) * instruction.offset); //store lowest 32 bits on Rd
        // TODO: Make this use the pq registers
        break;
        
        case kMOD:
        if (!instruction.i) shiftOffset(instruction.offset);
        dest = source % instruction.offset;
        break;
        
        case kDIV:
        if (!instruction.i) shiftOffset(instruction.offset);
        dest = source / instruction.offset;
        break;
        
        case kMOV:
        arithmetic = false;
        // This is a special case
        if (!instruction.i)
        {
            // so use the special shift format
            shiftOffset(instruction.offset, source);
            dest = instruction.offset;
        } else {
            // Make a giant literal out of the five bits where source would be
            // and the 10 bits of instruction.offset
            dest = (instruction.rs << 10) | instruction.offset;
        }
        break;
        
        case kAND:
        arithmetic = false;
        if (!instruction.i) shiftOffset(instruction.offset);
        dest = source & instruction.offset;
        break;
        
        case kORR:
        arithmetic = false;
        if (!instruction.i) shiftOffset(instruction.offset);
        dest = source | instruction.offset;
        break;
        
        case kXOR:
        arithmetic = false;
        if (!instruction.i) shiftOffset(instruction.offset);
        dest = source ^ instruction.offset;
        break;
        
        case kNOT:
        arithmetic = false;
        if (!instruction.i) shiftOffset(instruction.offset);
        dest = ~(source);
        break;
        
        case kCMP:
        commit = false;
        if (!instruction.i) shiftOffset(instruction.offset);
        dest = source - instruction.offset;
        // check if there would have been a carry out (a-b>a)
        if (dest > source) alu_carry = true;
        break;
        
        case kCMN:
        commit = false;
        if (!instruction.i) shiftOffset(instruction.offset);
        dest = source + instruction.offset;
        // check if there would have been a carry out (a+b<a)
        if (dest < source) alu_carry = true;
        break;
        
        case kTST:
        arithmetic = false;
        commit = false;
        if (!instruction.i) shiftOffset(instruction.offset);
        dest = source & instruction.offset;
        break;
        
        case kTEQ:
        arithmetic = false;
        commit = false;
        if (!instruction.i) shiftOffset(instruction.offset);
        dest = source ^ instruction.offset;
        break;
        
        case kBIC:
        arithmetic = false;
        if (!instruction.i) shiftOffset(instruction.offset);
        dest = source & ~instruction.offset;
        break;
        
        case kNOP:
        default:
        return (_timing.op[kNOP]);
    }
    
    // We're done if we're not setting status bits
    // N.B.: Ignore the S bit if dest == PC, so that we dont get status bit
    //       confusion when you branch or something.
    if (instruction.s && instruction.rd != kPCCode)
    {
        // Set status bits
        // There are two cases, logical and arithmetic
        if (arithmetic)
        {
            // the V flag in the CPSR will be set if an overflow occurs
            // into bit 31 of the result
            if (dest & kMSBMask != source & kMSBMask)
                SET_V;
            else
                CLEAR_V;
            
            // the N flag will be set to the value of bit 31 of the result
            if (dest & kMSBMask)
                SET_N;
            else
                CLEAR_N;
            
            // the Z flag will be set if and only if the result was zero
            if (dest == 0x0)
                SET_Z;
            else
                CLEAR_Z;
                
            // the C flag will be set to the carry out of bit 31 of the ALU
            // NOTE: the following detection may not work correctly on
            // 32bit machines
            if (alu_carry)
                SET_C;
            else
                CLEAR_C;
        
        } else {
            // We're a logical operation
            // C flag is set to the carry out of the shifter, so do nothing.
            
            // Z flag is set if result is all zeros
            if (dest == 0x0)
                SET_Z;
            else
                CLEAR_Z;
            
            // N flag is set to the logical value of bit 31 of the result
            if (dest & kMSBMask)
                SET_N;
            else
                CLEAR_N;
            
            // (V Flag is uneffected by logical operations)
        }
    }
    
    // Set the readable state of the ALU
    _result = commit;
    _output = dest;
    
    // Return timing
    return (_timing.op[instruction.op]);
}

cycle_t ALU::singleTransfer(STFlags &f)
{
    // The base register is where the address comes from
    reg_t base = _vm->selectRegister(f.rs);
    // The offset modifies the base register
    reg_t offset = f.offset;
    
    reg_t computed_source = base;
    
    // Preserve carry when doing single transfer
    bool c = C_SET;
    
    // Immediate means that the offset is composed of
    if (f.i)
        // Use the ALU barrel shifter here
        shiftOffset(offset);
    
    if (c) SET_C;
    
    if (f.p)
    {
        // Pre indexing -- Modify source with offset before transfer
        if (f.u)
            // Add offset
            computed_source += offset;
        else
            computed_source -= offset;
    }
    
    _aux_out = computed_source;
    
    if (!f.p)
    {
        // Post indexing -- Modify source with offset after transfer
        if (f.u)
            // Add offset
            computed_source += offset;
        else
            // Subtract
            computed_source -= offset;
        
        // Since we are post indexing, the W bit is redundant because
        // if you wanted to save the source value you could just set
        // the offset to #0
        base = computed_source;
    } else {
        // Honor the write-back bit only if it's set
        if (f.w)
            base = computed_source;
    }
    
    _output = base;
    _result = true;
    
    return (0);
}

