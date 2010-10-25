#ifndef _MMU_H_
#define _MMU_H_

#include "global.h"

enum SingleTransferMasks {
    kSTIFlagMask        = 0x02000000,
    kSTLFlagMask        = 0x01000000,
    kSTWFlagMask        = 0x00800000,
    kSTBFlagMask        = 0x00400000,
    kSTUFlagMask        = 0x00200000,
    kSTPFlagMask        = 0x00100000,
    kSTSourceMask       = 0x000F8000,
    kSTDestMask         = 0x00007C00,
    kSTOffsetMask       = 0x000003FF
};

enum SingleTransferOffsetMasks {
    kSTORegisterMask    = 0x0000001F,
    kSTOShiftValMask    = 0x000003E0
};

struct STFlags
{
    bool I, L, W, B, U, P;
    reg_t rs, rd, offset;
};

class VirtualMachine;

class MMU
{
public:
    MMU(VirtualMachine *vm, size_t size, size_t rtime, size_t wtime);
    ~MMU();
    
    bool init();
    
    size_t loadFile(const char *path, size_t to, bool writeBreak);
    bool writeOut(const char *path);
    
    // Operational: must return the timing
    size_t singleTransfer(const STFlags *f);
    size_t write(size_t addr, reg_t valueToSave);
    size_t writeBlock(size_t addr, reg_t *data, size_t size);
    size_t readWord(size_t addr, unsigned int &valueToRet);
    size_t readByte(size_t addr, char &valueToRet);
    size_t readRange(size_t start, size_t end, bool hex, char **ret);

private:
    reg_t shiftOffset(reg_t shift, reg_t *source);
    void abort(reg_t &location);
    
    VirtualMachine *_vm;
    size_t _memory_size, _read_time, _write_time;
    reg_t *_memory;
};

#endif
