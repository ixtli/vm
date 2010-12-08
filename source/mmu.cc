#include <string.h>
#include <fstream>

#include "includes/mmu.h"
#include "includes/virtualmachine.h"
#include "includes/alu.h"
#include "includes/util.h"
#include "includes/pipeline.h"
#include "includes/cache.h"

#define BREAK_INTERRUPT     0xEF000000

MMU::MMU(VirtualMachine *vm, reg_t size, cycle_t rtime, cycle_t wtime) : 
    _vm(vm), _memory_size(size), _read_time(rtime), _write_time(wtime)
{
    _cache = NULL;
}

MMU::~MMU()
{
    printf("Destroying MMU... ");
    if (_memory)
        free(_memory);
    if (_cache)
        delete [] _cache;
    printf("Done.\n");
}

bool MMU::init(char caches, CacheDescription *desc)
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
    
    // End if no caches to allocate
    _caches = caches;
    if (!_caches) return (false);
    
    // If we want caches, we need to have them described
    if (!desc) return (true);
    
    printf("Initializing %u caches:\n", caches);
    _cache = new MemoryCache[_caches];
    if (!_cache)
    {
        fprintf(stderr, "Could not allocate cache array.\n");
        return (true);
    }
    
    for (int i = 0; i < _caches; i++ )
    {
        if (_cache[i].init(this, desc[i], i))
            return (true);
    }
    
    // Initialize cache accounting
    _evictions = 0;
    _misses = 0;
    
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

cycle_t MMU::cache(reg_t addr, bool write, bool word)
{
    cycle_t ret = 0;
    
    // is it in any of the caches?
    
    // if it is, get the total cost of reading from the caches we've read to
    // so far.  also dirty the line if we're writing
    
    // if not, load it in to all of the caches
    
    // if any evictions occurred keep track of the time taken to write back
    
    return (ret);
}

cycle_t MMU::writeByte(reg_t addr, char valueToSave)
{
    if (addr >= _memory_size)
        return (0);
    
    _memory[addr] = valueToSave;
    
    // The amount of time this takes is simulated by our caches
    return (cache(addr, true, false));
}

cycle_t MMU::writeWord(reg_t addr, reg_t valueToSave)
{
    if (addr >= _memory_size)
        return (0);
    
    reg_t *temp = (reg_t *) &(_memory[addr]);
    *temp = valueToSave;
    
    // The amount of time this takes is simulated by our caches
    return (cache(addr, true));
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
    
    // Simulate a cache
    cycle_t ret = 0;
    for (int i = 0; i < size; i++)
        ret += cache(addr + i, true, false);
    
    // The amount of time this takes is simulated by our caches
    return (ret);
}

cycle_t MMU::readWord(reg_t addr, reg_t &valueToRet)
{
    if ((addr + kRegSize) > _memory_size)
        return 0;
    
    reg_t *temp = (reg_t *) &(_memory[addr]);
    valueToRet = *temp;
    
    // The amount of time this takes is simulated by our caches
    return (cache(addr));
}

cycle_t MMU::readByte(reg_t addr, char &valueToRet)
{
    if (addr >= _memory_size)
        return 0;
    
    valueToRet = _memory[addr];
    
    // The amount of time this takes is simulated by our caches
    return (cache(addr, false, false));
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
    
    // This function isn't used by anything but status query functions
    // so don't mess the caches up with it.
    return (0);
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
