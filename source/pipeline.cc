#include <string.h>

#include "includes/pipeline.h"
#include "includes/virtualmachine.h"

InstructionPipeline::InstructionPipeline(char stages, VirtualMachine *vm) :
    _stages(stages), _vm(vm)
{
    _inst = NULL;
    _data = NULL;
    _flags = NULL;
}

InstructionPipeline::~InstructionPipeline()
{
    printf("Destroying pipeline... ");
    if (_inst) free(_inst);
    // Free all PipelineData objects
    int i = 0;
    for (; i < _stages_in_use; i++)
        if (_data[i]) delete _data[i];
    printf("(%i datum) ", i);
    if (_data) free(_data);
    printf("Done.\n");
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
    _data = (PipelineData **)calloc(_stages, sizeof(PipelineData *));
    _flags = (PipelineFlags *)calloc(_stages, sizeof(PipelineFlags));
    
    // Error check
    if (!_inst || !_data || !_flags)
    {
        printf("memory allocation error.\n");
        return (true);
    }
    
    _registers_in_use = 0x0;
    _stages_in_use = 0;
    
    printf("Done.\n");
    return (false);
}

char *InstructionPipeline::stateString()
{
    size_t line = 40;
    size_t index = 0;
    char *out = (char *)malloc(sizeof(char) * line * _stages_in_use+1);
    sprintf(out, "\tRegisters in use: %#x\n", _registers_in_use);
    index = strlen(out);
    for (int i = 0; i < _stages_in_use; i++)
    {
        sprintf(out + index, "\t%i %c ", i, (i==_current_stage?'>':'-'));
        index = strlen(out);
        
        if (_flags[i].bubble)
        {
            sprintf(out + index, "BUBBLE\t");
        } else if (_data[i]) {
            if (_data[i]->location == 0x0 && _data[i]->condition_code == 0xF)
                sprintf(out + index, "NEW\t\t");
            else
                sprintf(out + index, "%#x\t", _data[i]->instruction);
        } else {
            sprintf(out + index, "CLEAR\t");
        }
        index = strlen(out);
        
        sprintf(out + index, "(%c)", _data[i]?'+':'-');
        index = strlen(out);
        
        if (_flags[i].squash)
        {
            sprintf(out + index, "(squash)");
            index = strlen(out);
        }
        
        if (_flags[i].wait != 0x0)
        {
            sprintf(out + index, " (wait: %#x)", _flags[i].wait);
            index = strlen(out);
        }
        
        if (_flags[i].lock != 0x0) {
            sprintf(out + index, " (lock: %#x)", _flags[i].lock);
            index = strlen(out);
        }
        
        sprintf(out + index, "\n");
        index = strlen(out);
    }
    return (out);
}

void InstructionPipeline::printState()
{
    char *out = stateString();
    fprintf(stdout, "%s", out);
    free(out);
}

bool InstructionPipeline::registerStage(pipeFunc func)
{
    // Error check
    if (!func)
    {
        fprintf(stderr, "Invalid pipeline function.\n");
        return (true);
    }
    
    if (_stages_in_use == _stages)
    {
        return (true);
        fprintf(stderr, "Pipeline full.\n");
    }
    
    // Register the function to be called at stage index
    _inst[_stages_in_use] = func;
    
    // Initialize the stage to "bubble" state
    _flags[_stages_in_use].bubble = 1;
    
    // If the pipe is now greater than one stage, there needs to be a space
    // for the last stage to push the datum pointer into on each pipe cycle
    if (_stages_in_use == 1)
    {
        _data[1] = _data[0];
        _data[0] = NULL;
    } else {
        // Allocate one more PipelineData for this new stage
        _data[_stages_in_use] = new PipelineData();
        if (!_data[_stages_in_use])
        {
            fprintf(stderr, "Allocation error.\n");
            return (true);
        }
        _data[_stages_in_use]->clear();
    }
    
    // Increase the count
    _stages_in_use++;
    
    // No error
    return (false);
}

// Pipe manipulation
bool InstructionPipeline::cycle()
{
    // Reset state of pipe stage zero
    _flags[0].clear();
    
    // Loop through the stages from end to front
    for (int i = _stages_in_use - 1; i > -1; i--)
    {
        // Set current stage index
        _current_stage = i;
        
        // squashed instructions become bubbles
        if (_flags[i].squash)
        {
            unlock();
            _flags[i].clear();
            _flags[i].bubble = 1;
        }
        
        // Call the function with data only if it's not a bubble
        if (!_flags[i].bubble)
            (_vm->*_inst[i])(_data[i]);
        
        // if this stage has dependancy on locked registers
        if (_flags[i].wait & _registers_in_use)
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
            
            // Accounting
            _bubbles++;
            
            // halt the rest of the pipe
            break;
        } else {
            _flags[i].wait = 0x0;
        }
        
        // If we're not the last stage or the first
        if (i < _stages_in_use - 1)
        {
            // Forward pipe state to next phase
            _flags[i+1] = _flags[i];
            
            // Forward returned datum
            if (!_data[i+1])
            {
                _data[i+1] = _data[i];
                _data[i] = NULL;
            }
        } else {
            if (!_data[0])
            {
                // Clear the data
                _data[i]->clear();
                // Swing it around to the beginning
                _data[0] = _data[i];
                // For deallocation safety
                _data[i] = NULL;
            } else {
                // If _data[0] is filled this can only mean that
                // there is a bubble with no associated data somewhere
                // in the pipe.  Find it and reclaim it.
                for (int j = 0; j < _stages_in_use -1; j++)
                {
                    if (!_data[j])
                    {
                        _data[j] = _data[i];
                        _data[i] = NULL;
                        break;
                    }
                }
                
                // If we didn't find the stage with no data, something is wron
                if (_data[i])
                {
                    fprintf(stderr, "Pipeline memory allocation compromised.\n");
                    return (true);
                }
            }
        }
    }
    
    // Doing this takes one machine cycle.
    _vm->incCycleCount(1);
    
    // Reset current stage
    _current_stage = 0;
    
    return (false);
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

bool InstructionPipeline::waitOnRegister(char reg)
{
    if (reg >= kVMRegisterMax)
    {
        fprintf(stderr, "Attempt to wait on out of bounds register\n.");
        return (true);
    }
    
    // Set registers to wait on
    _flags[_current_stage].wait |= (1 << reg);
}

bool InstructionPipeline::lock(char reg)
{
    // Trap on register attempting to double lock
    if (_registers_in_use & (1 << reg))
    {
        fprintf(stderr, "Register double lock!\n");
        return (false);
    }
    
    // Lock the registers
    _registers_in_use |= (1 << reg);
    _flags[_current_stage].lock |= (1 << reg);
    return (true);
}

void InstructionPipeline::unlock()
{
    // Unlock the register
    _registers_in_use ^= _flags[_current_stage].lock;
    _flags[_current_stage].lock = 0x0;
}

// Debugging
bool InstructionPipeline::step()
{
    
}

void InstructionPipeline::squash()
{
    _flags[_current_stage].squash = 1;
}

bool InstructionPipeline::isSquashed()
{
    return (_flags[_current_stage].squash ? true : false);
}

reg_t InstructionPipeline::locationToExecute()
{
    
    if (_stages_in_use < 4 && _data[0])
    {
        return (_data[0]->location);
    }
    
    if (!_flags[2].bubble && !_flags[2].squash && _data[2])
    {
        return (_data[2]->location);
    }
}
