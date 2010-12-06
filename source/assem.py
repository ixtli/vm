#!/usr/bin/python
import sys
import re

"""
An assembler for YAAA.

Written by Chris Galardi and John Rafferty ~10/2010
"""

# opens file for i/o
outfile = open(sys.argv[2], "w");

class Assembler:
    
    """
    Main class for our assembler.
    """
    
    # The asm file
    infile = []
    processedFile = []
    
    # Member variables
    label = {};
    
    shift_codes = { ">>" : "00", "<<" : "01", "ARS" : "10", "ROR" : "11",
                    "LSL": "00", "LSR": "01" }
    
    # Valid shift source registers (ones whos address can fit in four bytes)
    valid_ssr = {   "r0"  : "0000", "r1"   : "0001", "r2"   : "0010", 
                    "r3"  : "0011", "r4"   : "0100", "r5"   : "0101", 
                    "r6"  : "0110", "r7"   : "0111", "r8"   : "1000", 
                    "r9"  : "1001", "r10"  : "1010", "r11"  : "1011", 
                    "r12" : "1100", "r13"  : "1101", "r14"  : "1110",
                    "r15" : "1111" }
    
    # Valid shift value registers (ones whos address can fit in three bytes)
    valid_svr = {   "r0"  : "000", "r1"   : "001", "r2"   : "010", 
                    "r3"  : "011", "r4"   : "100", "r5"   : "101", 
                    "r6"  : "110", "r7"   : "111"}
    
    registers = {   "r0"  : "00000", "r1"   : "00001", "r2"   : "00010", 
                    "r3"  : "00011", "r4"   : "00100", "r5"   : "00101", 
                    "r6"  : "00110", "r7"   : "00111", "r8"   : "01000", 
                    "r9"  : "01001", "r10"  : "01010", "r11"  : "01011", 
                    "r12" : "01100", "r13"  : "01101", "r14"  : "01110",
                    "r15" : "01111", "pq0"  : "10000", "pq1"  : "10001", 
                    "pc"  : "10010", "psr"  : "10011", "cs"   : "10100", 
                    "ds"  : "10101", "ss"   : "10110", "fpsr" : "10111", 
                    "fpr0": "11000", "fpr1" : "11001", "fpr2" : "11010", 
                    "fpr3": "11011", "fpr4" : "11100", "fpr5" : "11101", 
                    "fpr6": "11110", "fpr7" : "11111"}
    
    condition_codes = { "eq" : "0000", "ne" : "0001", "cs" : "0010",
                        "cc" : "0011", "mi" : "0100", "pl" : "0101", 
                        "vs" : "0110", "vc" : "0111", "hi" : "1000", 
                        "ls" : "1001", "ge" : "1010", "lt" : "1011", 
                        "gt" : "1100", "le" : "1101", "al" : "1110",
                        "nv" : "1111" }
    
    # mnemonics of arithmetic and logical operations
    arithmetic_logic = {    "add" : "0000", "sub" : "0001", "mod" : "0010",
                            "mul" : "0011", "div" : "0100", "and" : "0101",
                            "orr" : "0110", "not" : "0111", "xor" : "1000",
                            "bic" : "1110" }
    
    # mnemonics for comparing and testing
    comp_test = {   "cmp" : "1001", "cmn" : "1010", "tst": "1011",
                    "teq" : "1100", "mov" : "1101", "nop" : "1111"}
    
    # mnumonics for loads and stores
    # By default, NO writeback (second bit).
    # 1st bit = 1 for load, third bit = 1 for byte transfer
    single_transfer = {    "ldw" : "100", "ldb" : "101", "stw" : "000",
                            "stb" : "001" }
    
    # Branch mnumonics
    branch = {  "b" : "1010", "bl" : "1011"};
    
    # sw/hw interrupts
    interrupt = {   "int" : "1111" };
    
    # fp ops
    floating_point = {  "fad" : "0000", "fsb" : "0001", "fml" : "0011", 
                        "fdv" : "0111", "cmf" : "1010", "cnf" : "1100" }
    
    # Member function definitions
    def __init__(self):
        
        """
        Initialize the class by reading in the file
        """
        
        f = open(sys.argv[1] , 'r');
        
        for line in f:
            l = line.strip()
            if len(l) > 0:
                # Deal with these-style comments
                if l[0] != "#":
                    self.infile.append(l)
        
    
    def explodeOp(self, op):
        splitter = re.compile(r'\w+')
        return (splitter.findall(op))
    
    def explodeLabel(self, op):
        splitter = re.compile(r'\b([A-Za-z0-9_]+):')
        return (splitter.findall(op))
    
    def explodeComment(self, op):
        splitter = re.compile(r'(#).*$\n?')
        return (splitter.findall(op))
    
    def decimal_to_binary(self, val):
        
        """
        Borrowed from some website.  We'll be sure to give it back
        """
        
        # constant = 10000000000000000000000000000000
        const = 0x80000000	
        padding = 0;
        moveon = 0; # continue through loop
        output = ""
        smalloutput = ""
        # for each bit
        for i in range(1,33):
            # if the bit is set, print 1
            if( val & const ):
                output = output + "1"
            else:
                output = output + "0"
            # shift the constant using right shift
            const = const >> 1
        # remove bit padding infront	
        for i in range(0, len(output)):
            if (output[i] == "1") or (moveon == 1):
                smalloutput += output[i];
                moveon = 1;
            else:
                continue;
        
        # pad numbers with less than 5 bits
        # still need to deal with situation with more than 5 bits
        output = "";
        if len(smalloutput) < 5:
            padding = 5-len(smalloutput);
            for i in range(0, padding):
                output += "0";
            output += smalloutput;
            return output;
        else:
            return smalloutput;
    
    def decToBin(self, val, rng):
        """
        A different decimal to binary converter that returns the first <rng>
        bits of val in the form of a string
        """
        out = ""
        for bit in range(rng):
            if (val >> bit) & 0x1 != 0:
                out = "1" + out
            else:
                out = "0" + out
        
        return out
    
    def firstPass(self):
        
        """
        This parses the file, finding labels and adding them to the table.
        """
        
        instruction_index = 0;
        for line in self.infile:
            # is it a label?
            label = self.explodeLabel(line)
            if len(label):
                # If so, record the index of the instruction that will follow
                # this line (value of instruction_index)
                self.label[label[0]] = instruction_index;
                # Remove this line from the file so we don't worry about it
                # in the next pass
                
                # Uncomment the following to display labels
                # print "Label '"+label[0]+"': " + str(instruction_index)
            else:
                # It is not a label so increase the instruction index
                self.processedFile.append(line);
                instruction_index += 1;
        
        print ""
    
    def movShift(self, line):
        """
        Use this to build operand 2 for the MOV op code.
        Should not modify line.  Returns None for error
        """
        # Shift syntax for mov should take either of the following forms
        # LSL 52 or ROR r8
        shift_code = ""
        if len(line) < 2:
            print("Invalid MOV shift syntax.")
            return None
        
        # Find the shift code
        if line[0] in self.shift_codes:
            shift_code = self.shift_codes[line[0]]
        else:
            print("Invalid MOV shift operation '"+line[0]+"'.")
            return None
        
        # Find the value to shift b
        if line[1] in self.registers:
            # Shifting by a register
            return "00" + self.registers[line[1]] + "0" + shift_code
        else:
            # Shift by a literal 6 bytes wide 
            if int(line[1]) > 127:
                print("Warning: Truncating MOV shift literal.")
            lit = self.decToBin(int(line[1]), 6)
            return lit + "1" + shift_code
    
    def calcOp2(self, line):
        """
        Returns a valid Op2 and a boolean for whether or not the user
        specified a literal or not.
        """
        
        if len(line) == 1:
            # Either an immediate or a single register for Rm
            if line[0] in self.registers:
                # Single register specified
                return self.barrelShift(line), False
            else:
                # Immediate value
                if int(line[0]) > 1023:
                    print("Warning: truncating value '"+str(int(line[0]))+"'.")
                return self.decToBin(int(line[0]), 10), True
        
        if len(line) != 3:
            print("Invalid shift syntax.")
            return None
        
        # The user wants a shift
        return self.barrelShift(line), False
        
    
    def barrelShift(self, line):
        """
        Make a valid operand 2 for all instructions except MOV.
        Returns None for error, else a string rep of a binary number
        """
        
        # If the user only specified a single register we take that to mean
        # shift the register left by zero.
        if len(line) == 1:
            if line[0] not in self.valid_ssr:
                printf("Invalid register to be shifted '"+line[0]+"'.")
                return None
            return "000" + self.valid_ssr[line[0]] + "000"
        
        # This part of the instruction should take either of the following forms
        # r0 LSL r5 or r5 ROR 7
        
        shift_code = ""
        rm = ""
        if len(line) < 3:
            print("Invalid shift syntax (too few arguments.)")
            return None
        
        # Find the source register
        if line[0] in self.valid_ssr:
            rm = self.valid_ssr[line[0]]
        else:
            print("Invalid register to be shifted '"+line[0]+"'")
            return None
        
        # Shift code should be second
        if line[1] in self.shift_codes:
            shift_code = self.shift_codes[line[1]]
        else:
            print("Invalid shift operation '"+line[1]+"'.")
            return None
        
        # Find the value to shift by
        if line[2] in self.valid_svr:
            # Shifting by a register
            return self.valid_svr[line[2]] + rm + "1" + shift_code
        else:
            # Shift by a literal 6 bytes wide 
            if int(line[2]) > 7:
                print("Warning: Truncating shift literal '"+str(int(line[2]))+"'.")
            lit = self.decToBin(int(line[2]), 3)
            return lit + rm + "0" + shift_code
        
    
    def parseDPInstruction(self, instruction, line):
        """
        Processes a Data Processing-format instruction and returns a binary
        string representing the output of the instruction following the
        condition code.
        """
        
        # Add the dataprocessing opcode
        ret = "00";
        
        # If it's a nop, then don't bother doing any work
        if instruction == "nop":
            ret += "0" + self.comp_test["nop"]+"0"+ self.decToBin(0, 20)
            return ret
        
        # allow a bang after the instruction to mean DONT set condition codes
        # otherwise default to setting them
        set_cond = "1"
        if line[0] == "!":
            set_cond = "0"
            del line[0]
        
        # mov is a special case
        if instruction == "mov":
            
            # Make sure we have enough arguments (dest, source)
            if len(line) < 2:
                print("Invalid MOV instruction format.")
                return None
            
            # Handle the first argument (destination register)
            if line[0] in self.registers:
                dest = self.registers[line[0]]
                del line[0]
                
                # Now that we know that the first argument is valid, are
                # we trying to store an immediate?
                if line[0] in self.registers:
                    # Not an immediate
                    ret += "0" + self.comp_test["mov"] + set_cond
                    
                    # Next is the source register for the mov
                    source = self.registers[line[0]]
                    del line[0]
                    
                    # If there's anything left on the line, it's treated
                    # as a shift
                    op2 = self.decToBin(0,10)
                    if len(line) > 0:
                        op2 = self.movShift(line)
                        if op2 == None:
                            return op2
                    
                    # assemble instruction
                    return ret + source + dest + op2
                
                # Just an immediate
                ret += "1" + self.comp_test["mov"] + set_cond
                # We get a combined 15 bit literal space in the mov op
                if int(line[0]) > 32767:
                    print("Warning: literal "+line[0]+" truncated to 32767.")
                
                lit = self.decToBin(int(line[0]), 15)
                # Most sig 5 bits packed into source
                source = lit[:5]
                # Last ten packed into op2
                op2 = lit[5:]
                # Finish writing the instruction
                return ret + source + dest + op2
                
            else:
                print("MOV argument one must be a register specifier.")
                return None
        
        # If it's not a mov or nop, continue
        if instruction in self.arithmetic_logic:
            # ADD r0, r1, r5 LSL r2
            # SUB r0, r2, psr
            
            if len(line) < 3:
                print("Too few arithmetic arguments.")
                return None
            
            if line[0] not in self.registers:
                print("Invalid register specifier '"+line[0]+"'.")
                return None
            dest = self.registers[line[0]]
            
            if line[1] not in self.registers:
                print("Invalid register specifier '"+line[1]+"'.")
                return None
            source = self.registers[line[1]]
            
            del line[0]
            del line[0]
            
            # All that's left is to determine the value of Op2
            op2, imm = self.calcOp2(line)
            if op2 == None:
                return None
            
            # Is op2 an immediate?
            if imm == True:
                ret += "1"
            else:
                ret += "0"
            
            # Assemble the rest of the instruction
            ret += self.arithmetic_logic[instruction] + set_cond
            return ret + source + dest + op2
        
        if instruction in self.comp_test:
            if len(line) < 2:
                print("Too few arguments for comparison operation.")
                return None
            
            # These instructions do not use the destination register
            dest = "00000"
            if line[0] not in self.registers:
                printf("Invalid register specifier '"+line[0]+"'.")
                return None
            source = self.registers[line[0]]
            del line[0]
            
            # All that's left is to determine the value of Op2
            op2, imm = self.calcOp2(line)
            if op2 == None:
                return None
            
            # Is op2 an immediate?
            if imm == True:
                ret += "1"
            else:
                ret += "0"
            
            # Assemble the rest of the instruction
            ret += self.comp_test[instruction] + set_cond
            return ret + source + dest + op2
        
    
    def parseSingleTransfer(self, instruction, line):
        # s/d base offset
        # s/d base
        
        if len(line) < 2:
            print("Too few single transfer arguments.")
            return None
        
        op = self.single_transfer[instruction]
        
        # Do pre by default
        pre = "1"
        
        # add by default
        add = "1"
        
        # define initial values for the other offsets
        immediate = "0"
        offset = "0"
        
        # A bang after the instruction means to enable writeback
        # N.b.: what this is doing is a little hacky, so make sure you get
        # it before you make changes
        if line[0] == "!":
            op[1] = "1"
            del line[0]
        
        if len(line) < 2:
            print("Too few single transfer arguments.")
            return None
        
        if line[0] not in self.registers:
            print("Invalid register specifier '"+line[0]+"'.")
            return None
        base = self.registers[line[0]]
        del line[0]
        
        if line[0] not in self.registers:
            print("Invalid register specifier '"+line[0]+"'.")
            return None
        sd_reg = self.registers[line[0]]
        del line[0]
        
        if len(line) == 0:
            # Nothing left to do
            offset = self.decToBin(0, 10)
        elif len(line) == 1:
            # Otherwise, we need to handle having an offset
            # (-)immediate
            # SHIFT
            
            # Immediate offset
            imm = int(line[0])
            if imm < 0:
                # negative, so set add to false (subtract)
                imm = abs(imm)
                add = "0"
            
            offset = self.decToBin(imm, 10)
        else:
            # If we got here, the user wants to send the rest to the barrel
            # shifter
            
            # TODO: Specify subtracting the offset from the base in a prettier
            # way than a "-" sign ...
            if line[0] == "-":
                add = "0"
            
            # Handle the offset in the same way as the DP instructions
            offset = self.barrelShfit(line)
            if offset == None:
                return None
            
        # construct the op and return
        operation = "01" + immediate + op + add + pre + base + sd_reg + offset
        return operation
        
    
    def assemble(self):
        
        """
        Assemble the file.
        """
        
        # Perform the first pass, collecting all labels
        self.firstPass();
        instruction_index = 0;
        
        # Perform the second pass, changing the instructions into ops
        for lines in self.processedFile:
            
            # Uncomment the following to print instructions with line numbers
            # print str(instruction_index) + ": " + lines
            
            line = self.explodeOp(lines)
            
            i = 0;
            immediate = 0;
            
            # Deal with condition codes
            if line[0] in self.condition_codes:
                cond_code = self.condition_codes[line[0]];
                del line[0];
            else:
                # default condition code is "always"
                cond_code = self.condition_codes["al"];
            
            # What is the instruction?
            instruction = line[0];
            del line[0];
            
            # Begin our binary instruction output
            bin = cond_code;
            
            # check if its an arithmetic/logic/compare/test instruction
            if (instruction in self.arithmetic_logic) or \
                (instruction in self.comp_test): 
                
                val = self.parseDPInstruction(instruction, line)
                # Error should cause it to skip the line
                if val == None:
                    continue
                
                bin += val
            elif instruction in self.single_transfer:
                val = self.parseSingleTransfer(instruction, line)
                if val == None:
                    continue
                
                bin += val
                #print bin
            elif instruction in self.interrupt:
                if len(line) < 1:
                    print("Must specify interrupt table offset.")
                    continue
                
                bin += self.interrupt[instruction]
                bin += self.decToBin(int(line[0]), 24)
            
            elif (instruction in self.branch):
                
                if len(line) != 1:
                    print("Invalid branch syntax.")
                    continue
                
                branch_loc = line[0];
                
                # begin formatting the binary string
                bin = cond_code + self.branch[instruction];
                
                offset = self.label[branch_loc] - instruction_index;
                
                # We have to adjust offset if we're going backwards
                # as a result of how we kepp track of label locations
                if self.label[branch_loc] < instruction_index:
                    offset += 1
                
                if (abs(offset) > 0xFFFFFF):
                    print("Warning: Branch offset greater than 24-bit max.")
                
                # print "Branch to '"+branch_loc+"': " + str(offset)
                
                bin += self.decToBin(offset, 24);
            else:
                print "Invalid operation '" + instruction + "'.";
                bin = self.condition_codes["nv"] + self.decToBin(0, 28)
            
            outfile.write(bin + "\n");
            instruction_index += 1;
        
    
    

if __name__ == "__main__":
    a = Assembler()
    a.assemble()
    del a
