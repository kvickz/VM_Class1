#include <stdint.h>
#include <stdio.h>
#include <string>
#include <assert.h>

//Little-endian system
//Memory divided into 3 sections
//
//0  - 13 bytes are for instructions
//14 - 15 bytes are for output
//16 - 19 bytes for two separate 2 byte inputs

typedef int8_t i8;
typedef int16_t i16;

typedef uint8_t u8; 
typedef uint16_t u16;

//Instruction Codes
const u8 k_load_word = 0x01;
const u8 k_store_word = 0x02;
const u8 k_add = 0x03;
const u8 k_sub = 0x04;
const u8 k_jmp = 0x05;
const u8 k_je = 0x06;
const u8 k_addi = 0x07;
const u8 k_halt = 0xff;

//Register Codes/Indices
const u8 k_ip = 0x00;
const u8 k_r1 = 0x01;
const u8 k_r2 = 0x02;

//Memory constants
const u8 k_output = 0x0E;
const u8 k_input1 = 0x10;
const u8 k_input2 = 0x12;

const int k_programSize = 0x14;
void execute_program(u8* pProgram)
{
	i16 registers[3] = {0};
	u8 currentInstruction = pProgram[registers[k_ip]];

	while (currentInstruction != k_halt && registers[k_ip] < k_programSize)
	{
		assert((currentInstruction >= k_load_word && currentInstruction <= k_addi || currentInstruction == k_halt));//No instruction defined for code
		switch (currentInstruction)
		{
		case k_load_word:
		{
			u8 iRegister = pProgram[registers[k_ip] + 1];
			i16* pInput = (i16*)&(pProgram[pProgram[registers[k_ip] + 2]]);

			registers[iRegister] = *pInput;

			registers[k_ip] += 2;
			break;
		}
		case k_store_word:
		{
			u8 iRegister = pProgram[registers[k_ip] + 1];
			u8 storeAddress = pProgram[registers[k_ip] + 2];

			u8* pRead = (u8*)&registers[iRegister];
			u8 value_lowerByte = *pRead++;
			u8 value_upperByte = *pRead;

			u8* pWrite = (u8*)(&pProgram[storeAddress]);
			*pWrite++ = value_lowerByte;
			*pWrite = value_upperByte;

			registers[k_ip] += 2;
			break;
		}
		case k_add:
		{
			i16 a = registers[pProgram[registers[k_ip] + 1]];
			i16 b = registers[pProgram[registers[k_ip] + 2]];
			i16 result = a + b;

			u8 result_lower = u8(result & 0x00ff);
			u8 result_higher = u8((result & 0xff00) >> (4 * 2));

			u8* pWrite = (u8*)(registers + k_r1);
			*pWrite = result_lower;
			pWrite++;
			*pWrite = result_higher;

			registers[k_ip] += 2;
			break;
		}
		case k_sub:
		{
			i16 a = registers[pProgram[registers[k_ip] + 1]];
			i16 b = registers[pProgram[registers[k_ip] + 2]];

			i16 result = a - b;
			u8 result_lower = (result & 0x00ff);
			u8 result_upper = ((result & 0xff00) >> (4 * 2));

			//Pack results in r1 in little endian ordering
			*((u8*)(&registers[k_r1])) = result_lower;
			*(((u8*)(&registers[k_r1])) + 1) = result_upper;

			registers[k_ip] += 2;
			break;
		}
		case k_jmp:
		{
			u8 iJump = pProgram[registers[k_ip] + 1];
			*(u8*)(&registers[k_ip]) = iJump - 1; //-1 because the end of the loop always iterates, but we want to end up ON iJump
			*(((u8*)(&registers[k_ip])) + 1) = 0x00;
			break;
		}
		case k_je:
		{
			i16 a = registers[k_r1];
			i16 b = registers[k_r2];

			if (a == b) //Branch
			{
				u8 iJump = pProgram[registers[k_ip] + 1];
				*(u8*)(&registers[k_ip]) = iJump - 1; //-1 because the end of the loop always iterates, but we want to end up ON iJump
				*(((u8*)(&registers[k_ip])) + 1) = 0x00;
			}
			else
			{
				registers[k_ip] += 1;
			}
			break;
		}
		case k_addi://k_addi register lowerByte upperByte
		{
			u8 iRegister = pProgram[registers[k_ip] + 1];
			u8 value_lower = (u8)pProgram[registers[k_ip] + 2];
			u8 value_upper = (u8)pProgram[registers[k_ip] + 3];

			i16 a = ((i16)value_lower) & (0x00ff);
			i16 b = (((i16)value_upper) & (0x00ff)) << (4 * 2);
			i16 inputValue = a | b;

			i16 newValue = registers[iRegister] + inputValue;

			u8* pWrite = ((u8*)(&registers[iRegister]));
			*pWrite = (u8)(newValue & (0x00ff));
			*(pWrite + 1) = (u8)((newValue & (0xff00)) >> (4 * 2));

			registers[k_ip] += 3;
			break;
		}
		case k_halt:
		default:
			return;
		}

		currentInstruction = pProgram[++registers[k_ip]];
	}
}

void print_memory(u8* pProgram)
{
	for (int i = 0; i < k_programSize; ++i)
	{
		u8 current = *(pProgram + i);
		printf("%s\t: %u\n", std::to_string(i).c_str(), current);
	}
}

int main()
{
	//Initialize memory
	/*
	u8 memory[k_programSize] = {};
	//Instructions
	memory[0x00] = 0x01;//load_word
	memory[0x01] = 0x01;//r1
	memory[0x02] = 0x10;//0x10

	memory[0x03] = 0x01;//load_word
	memory[0x04] = 0x02;//r2
	memory[0x05] = 0x12;//0x12

	memory[0x06] = 0x03;//add
	memory[0x07] = 0x01;//r1
	memory[0x08] = 0x02;//r2

	memory[0x09] = 0x02;//store_word
	memory[0x0a] = 0x01;//r1
	memory[0x0b] = 0x0E;//0x0E

	memory[0x0c] = 0xff;//Halt

	memory[0x0d] = 0x00;//Blank

	//Output 2, byte number
	memory[0x0e] = 0x00;//* 1
	memory[0x0f] = 0x00;//* 256

	//In 1, 2 byte number
	memory[0x10] = 0x00;//* 1
	memory[0x11] = 0x00;//* 256

	//In 2, 2 byte number
	memory[0x12] = 0x00;//* 1
	memory[0x13] = 0x00;//* 256
	*/

	//Program to add 2 numbers, 43 and 77 to get 120
	u8 program1_add[k_programSize] = 
	{
		//Load input 1 into register 1
		k_load_word, k_r1, k_input1,

		//Load input 2 into register 2
		k_load_word, k_r2, k_input2,

		//Perform add operation on r1 and r2
		//k_add, k_r1, k_r2,
		k_addi, k_r1, 0xff, 0xff,

		//Store instruction in output from r1
		k_store_word, k_r1, k_output,

		k_halt,
		
		0, 0, 

		0x2a, 0xff,//42
		0xcd, 0x00,//-35
	};

	u8 program2_subtract[k_programSize] =
	{
		//Load input 1 into register 1
		k_load_word, k_r1, k_input1,

		//Load input 2 into register 2
		k_load_word, k_r2, k_input2,

		//Perform add operation on r1 and r2
		k_sub, k_r1, k_r2,

		//Store instruction in output from r1
		k_store_word, k_r1, k_output,

		k_halt,
		0,

		0, 0,

		0x2b, 0,//43
		0x4d, 0 //77
	};

	
	u8 program3_loop[k_programSize] =
	{
		//in = 5
		//i = 0;
		//while(i != in)
		//	++i

		//load input1(0) into r1
		k_load_word, k_r1, k_input1,

		//load input2 into r2// This is the limit
		k_load_word, k_r2, k_input2,

		//loopStart:
		//if r1 == r2: goto jumpOut
		k_je, 0x0d,

		//iterate i
		k_addi, k_r1, 1,

// 		goto loopStart
		k_jmp, 0x06,

// 		jumpOut:
		k_halt,

		0x00, 0x00, //Output
		0x00, 0x00, //Input 1:iterator
		0x05, 0x00, //Input 2:limit
	};
	
	execute_program(program1_add);
	//execute_program(program2_subtract);
	//execute_program(program3_loop);

	return 0;
}