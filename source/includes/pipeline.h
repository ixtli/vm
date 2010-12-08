#ifndef _PIPELINE_H_
#define _PIPELINE_H_

#include "global.h"

enum InstructionClasses {
    kDataProcessing,
    kSingleTransfer,
    kBranch,
    kReserved,
    kInterrupt,
    kFloatingPoint
};

typedef struct DPFlags
{
    unsigned int i:1, s:1, op:4, unused:2;
    char rs, rd;
    reg_t offset;
};

typedef struct STFlags
{
    unsigned int i:1, l:1, w:1, b:1, u:1, p:1, unused:2;
    char rs, rd;
    reg_t offset;
};

typedef struct FPFlags
{
    unsigned int op:4, s:3, d:3, n:3, m:3;
    reg_t value;
};

typedef struct BFlags
{
    bool link;
    signed int offset;
};

typedef struct IntFlags
{
    reg_t comment;
};

typedef struct PipelineData
{
    inline void clear()
    {
        instruction = 0x0;
        location = 0x0;
        instruction_class = kReserved;
        condition_code = 0xF; // Never
        executes = false;
    }
    
    // Metadata
    reg_t location;
    reg_t instruction;
    bool executes;
    
    // Control registers
    char condition_code;
    char instruction_class;
    union {
        STFlags st;
        DPFlags dp;
        BFlags b;
        FPFlags fp;
        IntFlags i;
    } flags;
    
    // Instructions save as many as two values
    bool record;
    reg_t output0, output1;
};

class VirtualMachine;

// Type definition for 
typedef void (VirtualMachine::*pipeFunc)(PipelineData *data);

class InstructionPipeline
{
public:
    InstructionPipeline(char stages, VirtualMachine *vm);
    ~InstructionPipeline();
    
    // setup
    bool init();
    bool registerStage(pipeFunc func);
    
    // Pipe manipulation
    bool cycle();
    bool lock(char reg);
    void unlock();
    bool waitOnRegister(char reg);
    bool isSquashed();
    void squash();
    void invalidate();
    
    // Debugging helper methods
    bool step();
    char *stateString();
    void printState();
    reg_t locationToExecute();
    
private:
    
    typedef struct PipelineFlags
    {
        inline void clear()
        {
            squash = 0; bubble = 0; unused = 0;
            lock = 0x0; wait = 0x0;
        }
        
        char squash:1, bubble:1, unused:6;
        reg_t lock, wait;
    };
    
    char _stages, _stages_in_use;
    
    // Data registers
    pipeFunc *_inst;
    PipelineData **_data;
    
    // Control registers
    reg_t _registers_in_use;
    PipelineFlags *_flags;
    
    char _current_stage;
    
    // Accounting
    size_t _bubbles, _invalidations, _instructions_invalidated;
    
    VirtualMachine *_vm;
};

#endif // _pipeline_h_
