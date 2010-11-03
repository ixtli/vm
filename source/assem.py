#!/usr/bin/python
import sys
import re

"""
An assembler for YAAA.

Written by Chris Galardi and John Rafferty ~10/2010
"""

# opens file for i/o
infile = open(sys.argv[1] , 'r');
outfile = open(sys.argv[2], "w");

class Assembler:
    
    """
    Main class for our assembler.
    """
    
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
    
    branch = {"b" : "1010", "bl" : "1011"};
    
    # Member function definitions
    def __init__(self):
        
        """
        Initialize the class by parsing command line ops
        """
    
    
    def file_len(self, fname):
        
        """
        Determine the length of the file
        """
        
        with open(fname) as f:
                for i, l in enumerate(f):
                                    pass;
        return i + 1;
    
    def explodeOp(self, op):
        splitter = re.compile(r'\w+')
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
    
    def assemble(self):
        
        """
        Assemble the file
        """
        
        instruction_index = 0;
        
        #loop through all lines of the file one by one
        for lines in range(0,self.file_len(sys.argv[1])):
            line = infile.readline().strip();
            print line;
            
            line = self.explodeOp(line)
            
            i = 0;
            immediate = 0;
            instruction = line[0];          #the operation (add, etc..)
            dest = line[1];                 #destination reg
            src1  = line[2];                #first source register
            
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
                
                instruction_index += 1;
                
                print bin;
                outfile.write(bin + "\n");
                
                for i in self.label.keys():
                    print str(i) + " " + str(self.label[i]);
                
            elif (instruction[0] == "."):
                label_name = "";
                for i in range(1, len(instruction)):
                    label_name +=instruction[i];
                #print "branch label: " + label_name;
                self.label[label_name] = instruction_index+1;
                instruction_index += 1;
                
            elif (instruction.strip() in self.branch):
                branch_loc = "";
                i = len(instruction.strip())+1;
                while i < len(line):
                    branch_loc += line[i];
                    i += 1;
                print branch_loc;
                # begin formatting the binary string
                bin = "1110";
                if (instruction.strip() == "b"):
                    bin += self.branch[instruction.strip()];
                elif (instruction.strip() == "bl"):
                    bin += self.branch[instruction.strip()];
                
                print abs(instruction_index-self.label[branch_loc]);
                print "branch so far: " + bin;
                
            else:
                print "wrong instruction";
                

if __name__ == "__main__":
    a = Assembler()
    a.assemble()
    del a
