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
    
    # Member variables
    label = {};
    
    #the mnemonics of all arithmetic and logic instructions
    arithmetic_logic = {    "add" : "0000", "sub" : "0001", "mod" : "0010",
                            "mul" : "0011", "div" : "0100", "and" : "0101",
                            "orr" : "0110", "not" : "0111" ,"xor" : "1000"}
    
    #mnemonics for comparing and testing
    comp_test = {   "cmp" : "1001", "cmn" : "1010", "tst": "1011",
                    "teq" : "1100", "mov" : "1101", "bic" : "1110",
                    "nop" : "1111"}
    
    branch = {  "b" : "1010", "bl" : "1011"};
    
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
    
    def firstPass(self):
        
        """
        This parses the file, finding labels and adding them to the table.
        """
        
        previous_was_label = 0;
        instruction_index = 0;
        for line in self.infile:
            label = self.explodeLabel(line)
            if len(label):
                # It's a label
                if previous_was_label == 0:
                    self.label[label[0]] = instruction_index + 1;
                    previous_was_label == 1;
                    self.infile.remove(line);
                else:
                    # If we don't do this, this function needs to be recurive
                    # and I don't see a reason to have multiple names
                    # for one line
                    self.infile.remove(line);
                    print "Only one label per location, please."
            else:
                previous_was_label == 0
                instruction_index += 1;
            
    
    def assemble(self):
        
        """
        Assemble the file.
        """
        
        # Perform the first pass, collecting all labels
        self.firstPass();
        
        instruction_index = 0;
        
        # Perform the second pass, changing the instructions into ops
        for lines in self.infile:
            print lines;
            
            line = self.explodeOp(lines)
            
            i = 0;
            immediate = 0;
            instruction = line[0];          #the operation (add, etc..)
            dest = line[1];
            
            if len(line) < 3:               #destination reg
                src1 = "00"
            else:
                src1 = line[2];            #first source register
            
            if len(line) > 3:
                src2 = line[3];             #second source register, Op2
            else:
                src2 = "00"
            
            cond_code = "1110";             #condition code
            bin = cond_code;
            
            # check if its an arithmetic/logic/compare/test instruction
            if (instruction in self.arithmetic_logic) or \
                (instruction in self.comp_test): 
                i += 1;
                
                # mov is a special case
                if instruction == "mov":
                    src2 = src1
                    src1 = "00"
                
                # check if there is an r (register), or if its immediate
                if (src2[0] == "r"): 
                    immediate = 0;
                else:
                    immediate = 1;
                
                bindest = dest[1:];
                
                if src1[0] == 'r':
                    binsrc1 = src1[1:];
                else:
                    binsrc1 = src1;
                
                if src2[0] == 'r':
                    binsrc2 = src2[1:];
                else:
                    binsrc2 = src2;
                
                bin += "00";
                # I flag
                bin += str(immediate);
                
                if instruction in self.arithmetic_logic:
                        # opcode
                        bin += self.arithmetic_logic[instruction];
                else:
                        # opcode
                        bin += self.comp_test[instruction];
                
                # s flag
                bin += "1";
                bin += self.decimal_to_binary(int(binsrc1)); # Rs
                bin += self.decimal_to_binary(int(bindest)); # Rd
                # padding if its not being shifted
                bin += "00000";
                
                if immediate == 0:
                    # Op2
                    bin += self.decimal_to_binary(int(binsrc2));
                else:
                    bin += self.decimal_to_binary(int(src2));
                
                print bin;
                outfile.write(bin + "\n");
                
            elif (instruction in self.branch):
                branch_loc = dest;
                # begin formatting the binary string
                bin = "1110";
                if (instruction == "b"):
                    bin += self.branch[instruction];
                elif (instruction == "bl"):
                    bin += self.branch[instruction];
                offset = abs(instruction_index - self.label[branch_loc]);
                
                # format the offset as 2's comp. and add it to the instruction
                binoffset = self.decimal_to_binary(offset);
                actual_offset = int(binoffset, 2)-(1<<24);
                bin += self.decimal_to_binary(abs(actual_offset));
                
                print bin;
                outfile.write(bin + "\n");
                
            else:
                print "wrong instruction";
                
            instruction_index += 1;

if __name__ == "__main__":
    a = Assembler()
    a.assemble()
    del a
