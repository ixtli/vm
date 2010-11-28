#ifndef _PIPELINE_H_
#define _PIPELINE_H_

#include "global.h"

typedef struct PipelineFlags
{
    inline void clear()
    {
        squash = 0; bubble = 0; unused = 0;
    }
    
    unsigned int squash:1;
    unsigned int bubble:1;
    unsigned int unused:6;
};

class VirtualMachine;

// Type definition for 
typedef void *(VirtualMachine::*pipeFunc)(const void *data);

class InstructionPipeline
{
public:
    InstructionPipeline(char stages, VirtualMachine *vm);
    ~InstructionPipeline();
    
    // setup
    bool init();
    char registerStage(pipeFunc func);
    
    // Pipe manipulation
    bool cycle();
    bool lockRegister(char reg);
    void writeBackRegister(char reg, reg_t value);
    bool waitOnRegisters(char regs);
    bool testRegister(char reg);
    void invalidate();
    
private:
    char _stages, _stages_in_use;
    
    // Data registers
    pipeFunc *_inst;
    void **_data;
    
    // Control registers
    char _registers_in_use;
    PipelineFlags *_flags;
    char *_wait;
    
    char _current_stage;
    
    // Accounting
    size_t _bubbles, _invalidations, _instructions_invalidated;
    
    VirtualMachine *_vm;
};

#endif // _pipeline_h_
