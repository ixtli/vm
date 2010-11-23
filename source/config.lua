-- YAAA Virtual Machine Configuration
program = "out"
memory_dump = "memory.dump"
print_instruction = true
print_branch_offset = true
program_length_trap = 0x50
machine_cycle_trap = 7000

-- YAAA Machine Description

-- Memory size in bytes
memory_size = 1024
stack_size = 8
break_count = 10

-- Breakpoints (total should be LE to break_count)
breakpoints = {5}

-- Instruction timings

-- Memory
read_cycles = 100
write_cycles = 100

-- ALU timings
--  If not set, defaults to 1
alu_timings = {DIV=10, MUL=5}

-- INT timings
swint_cycles = 25

-- Branch Timings
branch_cycles = 10
