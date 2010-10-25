#include <string.h>
#include <fstream>
#include <iostream>

#include "includes/mmu.h"
#include "includes/virtualmachine.h"

#define BREAK_INTERRUPT     0xEF000000

MMU::MMU(VirtualMachine *vm, size_t size, size_t rtime, size_t wtime) : 
    _vm(vm), _memory_size(size), _read_time(rtime), _write_time(wtime)
{}

MMU::~MMU()
{
    printf("Destroying MMU... ");
    if (_memory)
        free(_memory);
    printf("Done.\n");
}

bool MMU::init()
{
    printf("Initializing MMU: %lib RAM\n", _memory_size);
    
    // Make sure we have a vm
    if (!_vm) return (true);
    
    // Allocate real memory from system and zero its
    printf("Allocating and zeroing memory... ");
    _memory = (unsigned int *)calloc(_memory_size, sizeof(char));
    // Test to make sure memory was allocated
    if (!_memory)
    {
        printf("Error.\n");
        return (true);
    } else {
        printf("Done.\n");
    }
    
    return (false);
}

size_t MMU::loadFile(const char *path, size_t to, bool writeBreak)
{
    char buffer[256];
    std::ifstream myfile(path);
    
    int i = 0;
    
    if (to >= _memory_size )
    {
        fprintf(stderr, "Tried to load memory image file outside of memory.\n");
        return (i);
    }
    
    printf("Loading memory image '%s'... ", path);
    
    while (myfile.good() && i+1 < _memory_size )
    {
        myfile.getline(buffer, 255);
        // 'to' is a byte address into memory, make sure to divide by 4
        // because we're reading from an array of word-sized values
        _memory[(to >> 2) + (i++)] = (reg_t) atoi(buffer);
    }
    
    // adjust i for the cycle that never happened
    i--;
    
    myfile.close();
    printf("Done.\n");
    
    if (writeBreak)
        _memory[(to >> 2) + i] = BREAK_INTERRUPT;
    
    return (i * kRegSize);
}

bool MMU::writeOut(const char *path)
{
    std::ofstream myfile(path, std::ifstream::out);
    
    if (!myfile.is_open())
        return (true);
    
    printf("Dumping memory image '%s'... ", path);
    for (int i = 0; i < _memory_size; i++)
    {
        myfile << _memory[i] << std::endl;
    }
    
    myfile.close();
    printf("Done.\n");
    
    return (false);
}

size_t MMU::write(size_t addr, reg_t valueToSave)
{
    if (addr >= _memory_size)
        return 0;
    
    // Addr is a byte address, but we need to read word aligned
    // so make sure to divide by 4
    _memory[addr >> 2] = valueToSave;

    return (_write_time);
}

size_t MMU::writeBlock(size_t addr, reg_t *data, size_t size)
{
    if ((addr + size * kRegSize) >= _memory_size)
        return 0;
    
    // Addr is a byte address, but we need to read word aligned
    // so make sure to divide by 4
    for (int i = 0; i < size; i++)
        _memory[i+(addr >> 2)] = data[i];
    
    return (_write_time);
}

size_t MMU::readWord(size_t addr, reg_t &valueToRet)
{
    if ((addr + kRegSize) > _memory_size)
        return 0;
    
    // Addr is a byte address, but we need to read word aligned
    // so make sure to divide by 4
    valueToRet = _memory[addr >> 2];

    return (_read_time);
}

size_t MMU::readByte(size_t addr, char &valueToRet)
{
    if (addr >= _memory_size)
        return 0;
    
    valueToRet = ((char *)_memory)[addr];

    return (_read_time);
}

size_t MMU::readRange(size_t start, size_t end, bool hex, char **ret)
{ 
    // Whats the max value?
    int length = 25;
    size_t words = end - start;
    size_t max_size = words + (words*length) + 1;
    
    //char range[size];
    *ret = (char*)malloc(sizeof(char) * max_size);
    char *val = *ret;
    
    char single[length+2];
    size_t range_index = 0;
    
    for(int i = 0; start+i <= end; i++)
    {
        if (!hex)
            sprintf(single, "%#x\t- %d\n",  (reg_t)start+i << 2, _memory[start+i]);
        else
            sprintf(single, "%#x\t- %#x\n",  (reg_t)start+i << 2, _memory[start+i]);
        
        strncpy(&val[range_index], single, strlen(single));
        range_index += strlen(single);
    }
    val[range_index] = '\0';
    
    return (_read_time * words);
}

reg_t MMU::shiftOffset(reg_t shift, reg_t *source)
{
    
}

void MMU::abort(reg_t &location)
{
    fprintf(stderr, "MMU ABORT: Read error %#x.\n", location);
}

size_t MMU::singleTransfer(const STFlags *f)
{
    // The base register is where the address comes from
    reg_t *base = vm->selectRegister(f->rs);
    // The dest register is where the value comes from
    reg_t *dest = vm->selectRegister(f->rd);
    // The offset modifies the base register
    reg_t offset = f->offset;
    
    reg_t computed_source = *base;
    
    // Immediate means that the offset is composed of
    if (f->I == true)
    {
        // An offset register
        reg_t *offset_reg = vm->selectRegister(offset & kSTORegisterMask);
        // And a 5-bit value to shift that register by
        offset = shiftOffset((offset & kSTOShiftValMask) >> 5, offset_reg);
        // (Rather odd here that Immediate == True means it's not immediate ...)
    }
    
    if (f->P)
    {
        // Pre indexing -- Modify source with offset before transfer
        if (f->U)
            // Add offset
            computed_source += offset;
        else
            computed_source -= offset;
    }
    
    // Do the read/write
    if (f->B)
    {
        // Reading a byte only
        if (computed_source >= _memory_size)
        {
            // Generate a MMU abort
            abort(computed_source);
            // Fail kindly to the application
            *dest = 0x0;
            return (kMMUAbortCycles);
        }
        // Before modifying the following, please make sure you understand
        // what is going on, as this sort of syntax is a bit tricky.
        if (f->L)
        {
            // Load the byte
            char b = ((char *)_memory)[computed_source];
            *dest = (reg_t)b;
        } else {
            // Store the LSByte of *dest
            char b = (char) *dest;
            ((char *)_memory)[computed_source] = b;
        }
    } else {
        // Reading a whole word!
        if (computed_source + 1 >= _memory_size)
        {
            abort(computed_source);
            return (kMMUAbortCycles);
        }
        if (f->L)
            // Load the word
            *dest = _memory[computed_source];
        else
            // Store the word
            _memory[computed_source] = *dest;
    }
    
    if (!f->P)
    {
        // Post indexing -- Modify source with offset after transfer
        if (f->U)
            // Add offset
            computed_source += offset;
        else
            // Subtract
            computed_source -= offset;
        
        // Since we are post indexing, the W bit is redundant because
        // if you wanted to save the source value you could just set
        // the offset to #0
        *base = computed_source;
    } else {
        // Honor the write-back bit only if it's set
        if (f->W)
            *base = computed_source;
    }
    
    return (kMMUReadClocks);
}
