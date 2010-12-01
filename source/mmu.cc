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
    // Make sure we have a vm
    if (!_vm) return (true);
    
    printf("Initializing MMU: %ub RAM\n", _memory_size);
    
    // Allocate real memory from system and zero its contents
    printf("Allocating and zeroing memory... ");
    _memory = (char *)calloc(_memory_size, sizeof(char));
    
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
    if (to >= _memory_size )
    {
        fprintf(stderr, "Tried to load memory image file outside of memory.\n");
        return (0);
    }
    
    // Get the starting address of where to write to.
    reg_t *temp = (reg_t *) &(_memory[to]);
    
    // Index used so that we know where the file load ended
    int i = 0;
    
    if (!path)
    {
        printf("No memory image to load.\n");
        if (writeBreak)
        {
            *temp = BREAK_INTERRUPT;
            i = 1;
        }
        return (i);
    }
    
    // Lines should be 32 readable characters long + \n
    char buffer[40];
    std::ifstream myfile(path);
    fprintf(stdout, "Loading memory image '%s'... ", path);
    
    while (myfile.good() && (i * kRegSize) < _memory_size - 1 )
    {
        myfile.getline(buffer, 40);
        
        if (strlen(buffer) != 32)
        {
            fprintf(stderr, "Warning: Line %i badly formatted. Skipping.\n", i);
            continue;
        }
        
        temp[i++] = _binary_to_int(buffer);
    }
    
    if ((i * kRegSize) == _memory_size && writeBreak)
        fprintf(stderr, "Warning: INT 0 written past memory boundry.");
    
    myfile.close();
    printf("Done.\n");
    
    if (writeBreak)
        temp[i] = BREAK_INTERRUPT;
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
    
    reg_t *temp = (reg_t *) &(_memory[addr]);
    *temp = valueToSave;
    
    return (_write_time);
}

cycle_t MMU::writeBlock(reg_t addr, reg_t *data, reg_t size)
{
    if ((addr + size) >= _memory_size)
        return 0;
    
    // Get initial location in memory to write to
    reg_t *temp = (reg_t *) &(_memory[addr]);
    
    // Since we're writing words, and everything wants to use sizeof() to get
    // the size measurement, we'll have to divide by 4
    for (int i = 0; i < (size >> 2); i++)
        temp[i] = data[i];
    
    return (_write_time);
}

cycle_t MMU::readWord(reg_t addr, reg_t &valueToRet)
{
    if ((addr + kRegSize) > _memory_size)
        return 0;
    
    reg_t *temp = (reg_t *) &(_memory[addr]);
    valueToRet = *temp;
    
    return (_read_time);
}

cycle_t MMU::readByte(reg_t addr, char &valueToRet)
{
    if (addr >= _memory_size)
        return 0;
    
    valueToRet = _memory[addr];

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
    
    reg_t *mem = (reg_t *) &(_memory[start]);
    
    for (int i = 0; start + i <= end; i++)
    {
        if (!hex)
        {
            sprintf(temp, "%#x\t- %d\n", (reg_t)start+i<<2, mem[i]);
        } else {
            sprintf(temp, "%#x\t- %#x\n", (reg_t)start+i<<2, mem[i]);            
        }
        strcpy(&(*ret)[last], temp);
        last += strlen(temp);
    }
}

void MMU::abort(const reg_t &location)
{
    fprintf(stderr, "MMU ABORT: Read error %#x.\n", location);
}

cycle_t MMU::singleTransfer(const STFlags &f, reg_t addr)
{
    // How long the operation took
    cycle_t timing = kMMUAbortCycles;
    
    // The dest register is where the value comes from
    reg_t dest = _vm->selectRegister(f.rd);
    
    // Do the read/write
    if (f.b)
    {
        // Reading a byte only
        if (addr >= _memory_size)
        {
            // Generate a MMU abort
            abort(addr);
            // Fail kindly to the application
            _read_out = 0x0;
            return (timing);
        }
        
        // Do the operation
        if (f.l)
        {
            // Load the byte
            char temp;
            timing = readByte(addr, temp);
            // Have to cast the result back
            _read_out = (reg_t) temp;
        } else {
            // Store the LSByte of dest
            timing = writeByte(addr, (char) dest);
        }
    } else {
        // Reading or writing a whole word!
        if (addr + 4 >= _memory_size)
        {
            abort(addr);
            return (timing);
        }
        
        if (f.l)
            // Load the word
            timing = readWord(addr, _read_out);
        else
            // Store the word
            timing = writeWord(addr, dest);
        
    }
    
    return (timing);
}
