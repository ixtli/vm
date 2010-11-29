#include <string.h>
#include <fstream>

#include "includes/mmu.h"
#include "includes/virtualmachine.h"
#include "includes/alu.h"
#include "includes/util.h"
#include "includes/pipeline.h"

#define BREAK_INTERRUPT     0xEF000000

MMU::MMU(VirtualMachine *vm, reg_t size, cycle_t rtime, cycle_t wtime) : 
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
    printf("Initializing MMU: %ub RAM\n", _memory_size);
    
    // Make sure we have a vm
    if (!_vm) return (true);
    
    // Allocate real memory from system and zero its
    printf("Allocating and zeroing memory... ");
    _memory = (reg_t *)calloc(_memory_size, sizeof(char));
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

reg_t MMU::loadProgramImageFile(const char *path, reg_t to, bool writeBreak)
{
    char buffer[256];
    std::ifstream myfile(path);
    
    int i = 0;
    
    if (!path)
    {
        printf("No memory image to load.\n");
        if (writeBreak)
        {
            _memory[(to >> 2)] = BREAK_INTERRUPT;
            i = 1;
        }
        return (i);
    }
    
    if (to >= _memory_size )
    {
        fprintf(stderr, "Tried to load memory image file outside of memory.\n");
        return (i);
    }
    
    printf("Loading memory image '%s'... ", path);
    
    while (myfile.good() && i+1 < _memory_size )
    {
        myfile.getline(buffer, 255);
        if (strlen(buffer) < 32)
            continue;
        // 'to' is a byte address into memory, make sure to divide by 4
        // because we're reading from an array of word-sized values
        _memory[(to >> 2) + (i++)] = _binary_to_int(buffer);
    }
    
    myfile.close();
    printf("Done.\n");
    
    if (writeBreak)
        _memory[(to >> 2) + i] = BREAK_INTERRUPT;
    else
        i--;
    
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
        myfile << std::hex << _memory[i] << std::endl;
    }
    
    myfile.close();
    printf("Done.\n");
    
    return (false);
}

cycle_t MMU::writeByte(reg_t addr, char valueToSave)
{
    if (addr >= _memory_size)
        return (0);
    
    _memory[addr] = valueToSave;
    
    return (_write_time);
}

cycle_t MMU::writeWord(reg_t addr, reg_t valueToSave)
{
    if (addr >= _memory_size)
        return (0);
    
    // Addr is a byte address, but we need to read word aligned
    // so make sure to divide by 4
    _memory[addr >> 2] = valueToSave;

    return (_write_time);
}

cycle_t MMU::writeBlock(reg_t addr, reg_t *data, reg_t size)
{
    if ((addr + size) >= _memory_size)
        return 0;
    
    // addr is a byte address, so we have to divide it by 4 to make a word addr
    // size is a byte size, so divide by four to get the amount of words
    for (int i = 0; i < (size >> 2); i++)
        _memory[(addr >> 2) + i] = data[i];
    
    return (_write_time);
}

cycle_t MMU::readWord(reg_t addr, reg_t &valueToRet)
{
    if ((addr + kRegSize) > _memory_size)
        return 0;
    
    // Addr is a byte address, but we need to read word aligned
    // so make sure to divide by 4
    valueToRet = _memory[addr >> 2];

    return (_read_time);
}

cycle_t MMU::readByte(reg_t addr, char &valueToRet)
{
    if (addr >= _memory_size)
        return 0;
    
    valueToRet = ((char *)_memory)[addr];

    return (_read_time);
}

cycle_t MMU::readRange(reg_t start, reg_t end, bool hex, char **ret)
{ 
    // Whats the max value?
    int length = 25; // 10 + 10 + \n + \0 + \t + - + ' '
    char temp[length];
    reg_t count = end - start;
    reg_t last = 0;
    *ret = (char *) malloc (sizeof(char) * count * length);
    
    for (int i = 0; start + i <= end; i++)
    {
        if (!hex)
        {
            sprintf(temp, "%#x\t- %d\n", (reg_t)start+i<<2, _memory[start+i]);
        } else {
            sprintf(temp, "%#x\t- %#x\n", (reg_t)start+i<<2, _memory[start+i]);            
        }
        strcpy(&(*ret)[last], temp);
        last += strlen(temp);
    }
}

void MMU::abort(const reg_t &location)
{
    fprintf(stderr, "MMU ABORT: Read error %#x.\n", location);
}

cycle_t MMU::singleTransfer(const STFlags &f)
{
    // The dest register is where the value comes from
    reg_t dest = _vm->selectRegister(f.rd);
    
    // Do the read/write
    if (f.b)
    {
        // Reading a byte only
        if (f.addr >= _memory_size)
        {
            // Generate a MMU abort
            abort(f.addr);
            // Fail kindly to the application
            _read_out = 0x0;
            return (kMMUAbortCycles);
        }
        // Before modifying the following, please make sure you understand
        // what is going on, as this sort of syntax is a bit tricky.
        if (f.l)
        {
            // Load the byte
            char b = ((char *)_memory)[f.addr];
            _read_out = (reg_t)b;
        } else {
            // Store the LSByte of *dest
            char b = (char) dest;
            ((char *)_memory)[f.addr] = b;
        }
    } else {
        // Reading a whole word!
        if (f.addr + 4 >= _memory_size)
        {
            abort(f.addr);
            return (kMMUAbortCycles);
        }
        if (f.l)
        {
            // Load the word
            _read_out = (unsigned int) *(((char *)_memory) + f.addr);
        } else {
            // Store the word
            // Remember that this is a word 
            *(((char *)_memory) + f.addr) = dest;
        }
    }
    
    return (kMMUReadClocks);
}
