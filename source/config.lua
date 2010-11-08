-- YAAA Virtual Machine Configuration
program = "out"
memory_dump = "memory.dump"

-- YAAA Machine Description

-- Memory size in bytes
memory_size = 1024
stack_size = 32

-- Instruction timings

-- Memory
read_cycles = 100
write_cycles = 100

-- ALU timings
--  If not set, defaults to 1
alu_timings = {DIV=10, MUL=5}