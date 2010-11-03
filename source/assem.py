import sys

#==========FUNCTIONS

#determine the length of the file
def file_len(fname):
    with open(fname) as f:
            for i, l in enumerate(f):
                                pass;
    return i + 1;

#"borrowed" from website...
# we'll be sure to give it back some day
def decimal_to_binary(int):
    ## constant = 10000000000000000000000000000000
    const = 0x80000000	
    padding = 0;
    moveon = 0; #continue through loop
    output = ""
    smalloutput = ""
    ## for each bit
    for i in range(1,33):
        ## if the bit is set, print 1
        if( int & const ):
            output = output + "1"
        else:
            output = output + "0"
        ## shift the constant using right shift
        const = const >> 1
    ##remove bit padding infront	
    for i in range(0, len(output)):
        if (output[i] == "1") | (moveon == 1):
            smalloutput += output[i];
            moveon = 1;
        else:
            continue;
    
    #pad numbers with less than 5 bits; still need to deal with situation with more than 5 bits
    output = "";
    if len(smalloutput) < 5:
        padding = 5-len(smalloutput);
        for i in range(0, padding):
            output += "0";
        output += smalloutput;
        return output;
    else:
        return smalloutput;
    
#==========DICTIONARYS

label = {};

#the mnemonics of all arithmetic and logic instructions
arithmetic_logic = {"ADD" : "0000", "SUB" : "0001", "MOD" : "0010", "MUL" : "0011",
"DIV" : "0100", "AND" : "0101", "ORR" : "0110", "NOT" : "0111" ,"XOR" :
"1000"}

#mnemonics for comparing and testing
comp_test = {"CMP" : "1001", "CMN" : "1010", "TST": "1011", "TEQ" : "1100", "MOV" :
"1101", "BIC" : "1110", "NOP" : "1111"}

#open file for reading
infile = open(sys.argv[1] , 'r');
outfile = open(sys.argv[2], "w");

#==========MAIN		

instruction_index = 0;

#loop through all lines of the file one by one
for lines in range(0,file_len(sys.argv[1])):
    line = infile.readline().strip();
   
    print line;
    
    i = 0;
    immediate = 0;
    instruction = ""; #the operation (ADD, SUB, AND, OR, MOV, etc)
    dest = ""; #destination reg
    src1  = ""; #first source register
    src2 = ""; #second source register, Op2
    cond_code = "1110"; #condition code
    bin = cond_code;
    
    #get the operation mnemonic (ADD, SUB, MOD, AND, etc)
    while (line[i] != " ") & (i < len(line)-1) & (line[i] != "\n"):
        instruction += (line[i]);
        i += 1;
    instruction += line[i];
    
    #check if its an arithmetic/logic/compare/test instruction
    if (instruction.strip() in arithmetic_logic) | (instruction.strip() in comp_test): 
        #bin += arithmetic_logic[instruction.strip()];
        #print bin;
        i += 1;
        
        #get the destination register
        while ((i < len(line)-1) & (line[i] != ",") & (line[i] != " ")):
            dest += (line[i]);
            i += 1;
        
        #get the source register, its it arighmetic or logic we grab it
        if instruction.strip() in arithmetic_logic:
            i += 2; #go past the comma and space
            while ((i < len(line)-1) & (line[i] != ",") & (line[i] != " ")):
                src1 += (line[i]);
                i += 1;
        #we use 0's if the instruction is MOV, the register is unused
        else:
                src1 = "00"; #needs 2 places because src[0] is assumed to be R usually

        #get the second source register
        i += 2; # go past the comma and space
        while ((line[i] != ",") & (line[i] != " ") & (i < len(line)-1)):
            src2 += (line[i]);
            i += 1;
        src2 += (line[i]);
      
        #check if there is an R, which stands for register or if it
        if (src2[0] == "R"): 
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

        if instruction.strip() in arithmetic_logic:
                bin += arithmetic_logic[instruction.strip()]; #opcode
        else:
                bin += comp_test[instruction.strip()]; #opcode

        bin += "1"; #s flag
        bin += decimal_to_binary(int(bindest)); #Rd
        bin += decimal_to_binary(int(binsrc1)); #Rs
        bin += "00000"; #padding if its not being shifted
        
        if immediate == 0:
            bin += decimal_to_binary(int(binsrc2)); #Op2
        else:
            bin += decimal_to_binary(int(src2));
        
        instruction_index += 1;
        
        print bin;
        outfile.write(bin + "\n");

    elif (instruction[0] == "."):
        label_name = "";
        for i in range(1, len(instruction)):
            label_name +=instruction[i];
        print "branch label: " + label_name;
        label[label_name] = instruction_index+1;
        
    else:
        print "wrong instruction";

