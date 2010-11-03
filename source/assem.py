#!/usr/bin/python
import sys

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
    arithmetic_logic = {"add" : "0000", "sub" : "0001", "mod" : "0010", "mul" : "0011",
    "div" : "0100", "and" : "0101", "orr" : "0110", "not" : "0111" ,"xor" :
    "1000"}
    
    #mnemonics for comparing and testing
    comp_test = {"cmp" : "1001", "cmn" : "1010", "tst": "1011", "teq" : "1100", "mov" :
    "1101", "bic" : "1110", "nop" : "1111"}
    
    branch = {"brh" : "1010", "brl" : "1011"};
    
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
            
            i = 0;
            immediate = 0;
            instruction = "";           #the operation (ADD, SUB, AND, ...)
            dest = "";                  #destination reg
            src1  = "";                 #first source register
            src2 = "";                  #second source register, Op2
            cond_code = "1110";         #condition code
            bin = cond_code;
            
            # get the operation mnemonic (ADD, SUB, MOD, AND, etc)
            while (line[i] != " ") and (i < len(line)-1) and (line[i] != "\n"):
                instruction += (line[i]);
                i += 1;
            instruction += line[i];
            
            # check if its an arithmetic/logic/compare/test instruction
            if (instruction.strip() in self.arithmetic_logic) or \
                (instruction.strip() in self.comp_test): 
                # bin += arithmetic_logic[instruction.strip()];
                # print bin;
                i += 1;
                
                # get the destination register
                while ( (i < len(line)-1) and \
                        (line[i] != ",") and \
                        (line[i] != " ")):
                    dest += (line[i]);
                    i += 1;
                    
                # get the source register, if it arighmetic or logic we grab it
                if instruction.strip() in self.arithmetic_logic:
                    # go past the comma and space
                    i += 2;
                    while ( (i < len(line)-1) and (line[i] != ",") and \
                            (line[i] != " ") ):
                        src1 += (line[i]);
                        i += 1;
                # we use 0's if the instruction is MOV, the register is unused
                else:
                    # needs 2 places because src[0] is assumed to be R usually
                    src1 = "00"; 
                # get the second source register
                # go past the comma and space
                i += 2;
                while ( (line[i] != ",") and (line[i] != " ") and \
                        (i < len(line)-1)):
                    src2 += (line[i]);
                    i += 1;
                src2 += (line[i]);
                
                # check if there is an r (register), or if its immediate
                if (src2[0] == "r"): 
                    immediate = 0;
                else:
                    immediate = 1;
                
                bindest = "";
                binsrc1 = "";
                binsrc2 = "";
                i = 1;
                while i < len(dest):
                    bindest += dest[i]; 
                    i += 1;
                i = 1;
                while i < len(src1):
                    binsrc1 += src1[i];
                    i += 1;
                i = 1;
                while i < len(src2):
                    binsrc2 += src2[i];
                    i += 1;
                    
                bin += "00";
                bin += str(immediate); #I flag
                
                if instruction.strip() in self.arithmetic_logic:
                        # opcode
                        bin += self.arithmetic_logic[instruction.strip()];
                else:
                        # opcode
                        bin += self.comp_test[instruction.strip()];
                
                # s flag
                bin += "1";
                bin += self.decimal_to_binary(int(binsrc1)); # Rs
                bin += self.decimal_to_binary(int(bindest)); # Rd
                # padding if its not being shifted
                bin += "00000";
                
                if immediate == 0:
                    bin += self.decimal_to_binary(int(binsrc2)); # Op2
                else:
                    bin += self.decimal_to_binary(int(src2));
                
                instruction_index += 1;
                
                print bin;
                outfile.write(bin + "\n");
                
                
                for i in self.label.keys():
                    print str(i) + " " + str(label[i]);
                
            elif (instruction[0] == "."):
                label_name = "";
                for i in range(1, len(instruction)):
                    label_name +=instruction[i];
                #print "branch label: " + label_name;
                self.label[label_name] = instruction_index+1;
                instruction_index += 1;
                
            elif (instruction.strip() in branch):
                print "jump";
                
            else:
                print "wrong instruction";
                

if __name__ == "__main__":
    a = Assembler()
    a.assemble()
    del a
