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
    MMU(VirtualMachine *vm, reg_t size, cycle_t rtime, cycle_t wtime);
    ~MMU();
    
    bool init();
    
    reg_t loadFile(const char *path, reg_t to, bool writeBreak);
    bool writeOut(const char *path);
    
    // Operational: must return the timing
    cycle_t singleTransfer(const STFlags *f);
    cycle_t writeWord(reg_t addr, reg_t valueToSave);
    cycle_t writeByte(reg_t addr, char valueToSave);
    cycle_t writeBlock(reg_t addr, reg_t *data, reg_t size);
    cycle_t readWord(reg_t addr, unsigned int &valueToRet);
    cycle_t readByte(reg_t addr, char &valueToRet);
    cycle_t readRange(reg_t start, reg_t end, bool hex, char **ret);

private:
    void abort(reg_t &location);
    
    VirtualMachine *_vm;
    reg_t _memory_size;
    cycle_t _read_time, _write_time;
    reg_t *_memory;
};

#endif
