-- YAAA Virtual Machine Configuration
program = "out"
memory_dump = "memory.dump"
print_instruction = false
print_branch_offset = false
program_length_trap = 0x1000
machine_cycle_trap = 30000

-- YAAA Machine Description

-- Memory size in bytes
memory_size = 3772
stack_size = 8
break_count = 10

-- Pipeline configuration
stages = 5

-- Cache configuration
-- Each element in the list is {lines, ways, line length, access time}
caches = {{2, 1, 4, 1}}
debug_cache = true

-- Breakpoints (total should be LE to break_count)
-- (NOTE: this is the line number of the LAST instruction you want to execute)
breakpoints = {}

-- Instruction timings

-- Time to read and write to main memory
read_cycles = 100
write_cycles = 100

-- ALU timings
--  If not set, defaults to 1
alu_timings = {DIV=10, MUL=5}

-- INT timings
swint_cycles = 25

-- Branch Timings
branch_cycles = 10
