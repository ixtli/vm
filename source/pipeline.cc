#include "includes/pipeline.h"
#include "includes/virtualmachine.h"

InstructionPipeline::InstructionPipeline(char stages, VirtualMachine *vm) :
    _stages(stages), _vm(vm)
{
    _inst = NULL;
    _data = NULL;
    _flags = NULL;
    _wait = NULL;
}

InstructionPipeline::~InstructionPipeline()
{
    if (_inst) free(_inst);
    if (_data) free(_data);
}

// setup
bool InstructionPipeline::init()
{
    printf("Initializing %u stage instruction pipeline... ", _stages);
    
    // Error check (stages cannot = zero)
    if (!_stages || !_vm)
    {
        printf("invalid parameters.\n");
        return (true);
    }
    
    // Dynamic memory allocation
    _inst = (pipeFunc *)calloc(_stages, sizeof(pipeFunc));
    _data = (void **)calloc(_stages, sizeof(void *));
    _flags = (PipelineFlags *)calloc(_stages, sizeof(PipelineFlags));
    _wait = (char *)calloc(_stages, sizeof(char));
    
    // Error check
    if (!_inst || !_data || !_flags || !_wait)
    {
        printf("memory allocation error.\n");
        return (true);
    }
    
    _stages_in_use = 0;
    
    printf("Done.\n");
    return (false);
}

char InstructionPipeline::registerStage(pipeFunc func)
{
    // Error check
    if (!func)
    {
        printf("Invalid pipeline function.\n");
        return (true);
    }
    
    if (_stages_in_use == _stages)
    {
        return (true);
        printf("Pipeline full.\n");
    }
    
    // Register the function to be called at stage index
    _inst[_stages_in_use] = func;
    
    // Initialize the stage to "bubble" state
    _flags[_stages_in_use].bubble = 1;
    
    // Increase the count
    _stages_in_use++;
    
    // No error
    return (_stages_in_use);
}

// Pipe manipulation
bool InstructionPipeline::cycle()
{
    // Loop through the stages from end to front
    for (int i = _stages_in_use - 1; i > -1; i--)
    {
        // if this stage has dependancy on locked registers
        if (_wait[i] & _registers_in_use)
        {
            // Make sure the final stage never stalls
            if (i == _stages_in_use - 1)
            {
                fprintf(stderr, "Instruction Pipe: Final stage stalling.\n");
                return (true);
            }
            
            // forward a bubble instead of state
            _flags[i+1].clear();
            _flags[i+1].bubble = 1;
            _wait[i+1] = 0x0;
            
            // Accounting
            _bubbles++;
            
            // halt the rest of the pipe
            break;
        }
        
        // Set current stage index
        _current_stage = i;
        // Call the function with data only if it's not a bubble
        void *ret = NULL;
        if (!_flags[i].bubble)
            ret = (_vm->*_inst[i])(_data[i]);
        
        // If we're not the last stage
        if (i < _stages_in_use - 1)
        {
            // Forward pipe state to next phase
            _flags[i+1] = _flags[i];
            _wait[i+1] = _wait[i];
            
            // Forward returned datum
            _data[i+1] = ret;
            
            // Be sure to properly dispose of the used datum because it's always
            // going to be overwritten.  N.B.: _data[0] should always be NULL
            if (_data[i]) free(_data[i]);
        } else if (ret) {
            // The only error state is if the LAST stage trys to return data
            // This means the pipe was improperly constructed, so warn.
            free(ret);
            fprintf(stderr, "Instruction Pipe: Final stage passed data.\n");
            return (true);
        }
    }
    
    // Reset state of pipe stage zero
    _flags[0].clear();
    _wait[0] = 0x0;
    
    // Doing this takes one machine cycle.
    _vm->incCycleCount(1);
    
    // Reset current stage
    _current_stage = 0;
}

void InstructionPipeline::invalidate()
{
    // Squash all instruction after the current one
    for (int i = 0; i < _current_stage; i++)
    {
        _flags[i].squash = 1;
        _instructions_invalidated++;
    }
    
    _invalidations++;
}

bool InstructionPipeline::waitOnRegisters(char regs)
{
    if (regs >= kVMRegisterMax)
    {
        fprintf(stderr, "Attempt to wait on out of bounds register\n.");
        return (true);
    }
    
    // Set registers to wait on
    _wait[_current_stage] = regs;
}

bool InstructionPipeline::lockRegister(char reg)
{
    // Error check
    if (reg >= kVMRegisterMax)
    {
        fprintf(stderr, "Pipeline attempted to lock a register out of bounds.\n");
        return (false);
    }
    
    // Trap on register attempting to double lock
    if (_registers_in_use & reg)
    {
        fprintf(stderr, "Pipeline attempted to double lock register %u.\n", reg);
        return (false);
    }
    
    // Lock the register
    _registers_in_use &= reg;
    return (true);
}

void InstructionPipeline::writeBackRegister(char reg, reg_t value)
{
    // Error check
    if (reg >= kVMRegisterMax)
    {
        fprintf(stderr, "Pipeline attempted to write a register out of bounds.\n");
        return;
    }
    
    // Write the value back if pipe isn't invalid
    if (!_flags[_current_stage].squash)
    {
        reg_t *wb = _vm->selectRegister(reg);
        *wb = value;
    }
    
    // Unlock the register
    _registers_in_use ^= reg;
}

bool InstructionPipeline::testRegister(char reg)
{
    // Error check
    if (reg >= kVMRegisterMax)
        return (false);
    
    if (_registers_in_use & reg)
        return (true);
    else
        return (false);
}

