COD5 CPU 4x4x4
==============

The Public Domain processor
---------------------------

### The cube registers

```
          --------------------+
         / 63 / 62 / 61 / 60 /| 
        /-------------------/ |
       / 47 / 46 / 45 / 44 /| |
      /-------------------/ | |
     / 31 / 30 / 29 / 28 /| |/|
    /-------------------/ | / |
   / 15 / 14 / 13 / 12 /| |/|/|
  +-------------------+ | / / |
  |    |    |    |    | |/|/|/|
  | 15 | 14 | 13 | 12 | / / / /
  ---------------------/|/|/|/
  | 11 | 10 | 09 | 08 | / / /
  ---------------------/|/|/
  | 07 | 06 | 05 | 04 | / /      
  ---------------------/|/      
  | 03 | 02 | 01 | 00 | /       
  +-------------------+         
                                                                           |RN1|RN0|  2 Nibble registers (4bits)
                                                                           |    RB0|  1 BYTE register (8bits)
                                                                       |        RW0|  1 WORD register (16bits)
   | RD15| ... |  RD8|  RD7|  RD6|  RD5|  RD4|  RD3|  RD2|  RD1|                RD0|  16 DWORD registers(32bits)
           ...    RQ4|        RQ3|        RQ2|        RQ1|                      RQ0|  8 QWORD registers (64 bit) (unused)
 64|   60| ... |   32|   28|   24|   20|   16|   12|   08|   04| 03| 02| 01|     00|  64 bytes 

RD15 is the FLAG register and CPUID
RD14 is the data RAM bank register
RD13 is the instruction pointer 
RD7 is the link register
RD6 is the stack pointer

```

### Memory map

```
64 bit memory address (byte addressed, little endian)
     0x3FFF FFFF FFFF FFFF
                          | 4 Exabyte of memory that can be accessed by banked RAM
     0x0000 0001 0000 0000
                 FFFF FFFF
		          | 2GB banked RAM window
     0x0000 0000 8000 0000
                 7FFF FFFF
                          | 2GB unbanked RAM
     0x0000 0000 0000 0080
                      007F
		          | special functions  
			  | CAS memory locations                   
     0x0000 0000 0000 0020
                      001F
                          | 32 byte boot program		 
     0x0000 0000 0000 0000
```


### Instruction set

```
7654 32 10 			
 OP Code		Mnemonic	; Description
===========================================================================================
0000 00 00 + (n+1)bytes		EXT	; EXTension instruction
					; Next byte's 3 LSB bits are the amount of following bytes (0 to 7)
					; XXXX XNNN	EXTRA_OPCODE
					; When the minimal CPU implementation reachs an EXT instruction
					; it skips the EXTRA_OPCODE and the following bytes.
					; Example of a 4 bytes extension instruction :
					; 00000000 XXXXX010 YYYYYYYY ZZZZZZZZ
					; Example of a 2 byte extension instruction :
					; 00000000 ZZZZZ000 

     00 01		SEZ RD0		; Skip next instruction if RD0 Equal 0
     00 10		SNZ RD0		; Skip next instruction if RD0 is Not equal to 0
     00 11		SYS RD0		; SYStem call

     01 00		SBC CF		; Skip next instruction if Bit Carry flag is Clear
     01 01		SBS CF		; Skip next instruction if Bit Carry flag is Set
     01 10		CLR CF		; Clear Carry Flag
     01 11		SET CF		; Set Carry Flag

     10 WW		SEQ RD0,RDx	; Skip next instruction if RD0 EQual RD[1-4]
     11 WW		SNE RD0,RDx	; Skip next instruction if RD0 Not Equal RD[1-4]

0001 00 WW		SLE RD0,RDx	; Skip next instruction if RD0 is Less than or Equal to RD[1-4] (signed)
     01 WW		SGT RD0,RDx	; Skip next instruction if RD0 is Greater Than RD[1-4] (signed)
     10 WW		SGE RD0,RDx	; Skip next instruction if RD0 is Greater or Equal to RD[1-4] (signed)
     11 WW		SLT RD0,RDx	; Skip next instruction if RD0 is Less Than RD[1-4] (signed)

0010 00 NN		SHR RD0,n	; bit SHift Right RD0 1, 2, 4 or 8 amount
     01 NN		SHL RD0,n	; bit SHift Left RD0 1, 2, 4 or 8
     10 AA		CAS RD0,RD1,[a] ; Compare-And-Swap. 
     					; If value at address "a" equals RD0 : puts RD1 at address "a" and into RD0.
					; otherwise does nothing

     11 00		TXOR RD0,RD1,RD2 ; Ternaary logic operators (4 states: 0,1,2(x),3(z)) 
     11 01              TOR RD0,RD1,RD2  ;
     11 10		TAND RD0,RD1,RD2
     11 11		TINV RD0	

0011 XX YY		XOR RD0,RDx,RDy ; assign bitwise XOR result of RD[0-3] ^ RDy[0-3] to RD0

0100 XX YY		ADD RD0,RDx,RDy	; assign arithmetic ADDition of RD[0-3] + RD[0-3] to RD0
					; set carry flag on overflow, otherwise clear it

0101 XX YY		SUB RD0,RDx,RDy ; assign arithmetic SUBtraction of RDx[0-3] - RDy[0-3] to RD0 
					; set carry flag if RDy value is greater 
					; than RDx value (negative result)
					; otherwise clear it

0110 XX YY		AND RD0,RDx,RDy ; assign bitwise AND result of RDx[0-3] & RDy[0-3] to RD0

0111 XX YY		OR RD0,RDx,RDy 	; assign bitwise OR result of RDx[0-3] | RDy[0-3] to RD0

1000 0W WW		LD RD0,[RDx]	; LoaD into RD0 the DWORD from memory address pointed by RD[0-7]
     1W WW		ST RD0,[RDx]	; STore RD0 DWORD to memory pointed by RD[0-7]

1001 0W WW		STB RB0,[RDx]	; LoaD into RB0 the byte from memory address pointed by RD[0-7]
     1W WW		LDB RB0,[RDx]	; STore RB0 byte to memory pointed by RD[0-7]

1010 00 00		ADDI RD0,RD0,4	; ADD 4 to RD0

1010 WW WW		MOV RD0,RDx 	; MOVe RDx[1-15] value to RD0

1011 00 00		SUBI RD0,RD0,4   ; SUBtract 4 to RD0

1011 WW WW		MOV RDx,RD0	; MOVe RD0 value to RDx[1-15]

1100 ii ii		LDI RN0,imm4	; LoaD 4 bits Immediate value to RN0
1101 ii ii		LDI RN1,imm4	; LoaD 4 bits Immediate value to RN1

1110 00 00 + 1byte	LDI RB0,imm8	; LoaD 8 bits Immediate value to RB0 
1110 00 01 + 2bytes	LDI RW0,imm16	; LoaD 16 bits Immediate value to RW0
1110 00 10 + 3bytes	LDI RD0,imm24	; LoaD sign extended 24 bits Immediate value to RD0
1110 00 11 + 4bytes	LDI RD0,imm32	; LoaD 32 bits Immediate value to RD0 

1110 01 00 + 1byte	JMP rel		; JuMP to relative program address 
1110 01 01 + 2bytes	JMP rel		; JuMP to relative program address 
1110 01 10 + 3bytes	JMP rel		; JuMP to relative program address 
1110 01 11 + 4bytes	JMP rel		; JuMP to relative program address 

1110 10 00		ADDI RD6,RD6,4	; increment stack pointer
1110 10 01		SUBI RD6,RD6,4	; decrement stack pointer

1110 10 10		INC RD0		; INCrement RD0 value by 1 (don't update carry flag)
     10 11		DEC RD0		; DECrement RD0 value by 1 (don't update carry flag)
     11 00		INV RD0		; INVert RD0 bits (bitwise NOT)
     11 01		LDI RD0,-1	; LoaD Immediate -1 value to RD0
     11 10		SWP RD0,RD6	; SWaP RD0 and RD6 content
     11 11		SWP RD0,RD7	; SWaP RD0 and RD7 content

1111 0W WW		JR RDx		; JumP to program address contained in Register RD[0-7]

1111 1W WW		JAL RDx		; Jump And Link to program address contained in register RD[0-7]
					; and assign the address of next instruction to the link register (RD7)

```

### Pipeline

5 stages :

* Fetch
* Decode
* Execute
* Memory
* Writeback


See page 413 :

https://www.hep.ucl.ac.uk/pbt/wikiData/teaching/FPGA_books/David%20M.%20Harris,%20Sarah%20L.%20Harris%20-%20Digital%20Design%20and%20Computer%20Architecture,%20(2012,%20Morgan%20Kaufmann).pdf

### Data interface

```
300 bits/second using mono in audio and left audio out for data. 
The right audio signal is used to trigger the transmit signal of the 27MHz Citizen Band transceiver.
```
