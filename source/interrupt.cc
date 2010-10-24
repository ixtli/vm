#include "includes/virtualmachine.h"
#include "includes/interrupt.h"

#define kBREAK_INSTRUCTION      0xDF00000F
#define kRETURN_INSTRUCTION     0xDF0000FF

reg_t jump_table[] = {      0x0, sizeof(reg_t)          };
reg_t functions[] = {       kBREAK_INSTRUCTION          ,
                            kRETURN_INSTRUCTION         ,
                            0xDF00000F                  ,
                            0xDF00000F                  };

InterruptController::InterruptController(VirtualMachine *vm) : _vm(vm)
{}

InterruptController::~InterruptController()
{
    _vm = NULL;
}

bool InterruptController::init()
{
    // Return true if instantiation failed
    if (!vm) return (true);
    
    // set up the table
    // TODO: Make this work
    
    // install the functions
    _vm->installJumpTable(NULL, 0);
    _vm->installIntFunctions(functions, sizeof(functions));
    
    return (false);
}

size_t InterruptController::swint(reg_t comment)
{
    // There are two possible interpretations of the comments field
    if (!vm->supervisor)
    {
        // If user-mode code called this then we treat the comment
        // as an offset into the interrupt table.
        
        // Check to see the address at that offset and jump to it
        *(vm->selectRegister(kPCCode)) = jump_table[comment];
        
        // We're entering supervisor mode
        vm->supervisor = true;
        return(kSWIntUserMode);
    }
    
    // otherwise we're in supervisor mode and the vm is trying to talk to
    // the host machine's hardware.  We use the 'ret' value to simulate
    // the time it takes to do one of these hardware interrupts
    size_t ret = 0;
    
    switch (comment)
    {
        case 0xF:
        // Treat this as BREAK, which should stop the FEX and allow the server
        // to query the state of the machine
        vm->fex = false;
        break;
        
        case 0xFF:
        // This is the return function, which sets PC to r15
        // and turns off supervisor mode
        *(vm->selectRegister(kPCCode)) = *(vm->selectRegister(kR15Code));
        vm->supervisor = false;
        break;
        
        default:
        break;
    }
    
    return (ret);
}