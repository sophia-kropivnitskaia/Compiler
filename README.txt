Homework 4: Recursive Descent Parser and Code Generator 
Sophia Kropivnitskaia and Harshika Jindal

Description: Implementing a Recursive Descent Parser and Code Generator for the extended
PL/0 language. Its procedures are support via procedure-declaration and the call statement. 
If statements are include an else part, ensuring complete code generation paths in
both branches.Mod operator is recognized in the grammar and translated to 2 0 11.
Output of elf.txt can be directly used in PM/0 Virtual Machine. 

Compilation Instructions:
gcc -o hw4compiler hw4compiler.c
./hw4compiler input.txt

Usage:
The required argument is the input file, where the source program will be read. 

Example:
Input File:
var x, y;
begin
x:= y * 2;
end.

Output File:

No errors, program is syntactically correct.
               
Line 	 OP L  M
0 	JMP 0 13
1 	INC 0  5
2 	LOD 0  4
3 	LIT 0  2
4 	OPR 0  3
5 	STO 0  3
6 	SYS 0  3

Symbol Table:
Kind | Name | Value | Level | Address | Mark
---------------------------------------------------
2 | x | 0 | 0 | 3 | 1
2 | y | 0 | 0 | 4 | 1

elf.txt file output: 

7 0 13
6 0 5
3 0 4
1 0 2
2 0 3
4 0 3
9 0 3

    