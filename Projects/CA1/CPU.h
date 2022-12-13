#include <iostream>
#include <bitset>
#include <stdio.h>
#include <stdlib.h>
#include <string>
using namespace std;

// Opcode types
#define ZERO 0x0 // Five in a row indicates the program has completed
#define RTYPE 0x33
#define ITYPE 0x13
#define LOADWORD 0x3
#define STOREWORD 0x23
#define SBTYPE 0x63
#define UJTYPE 0x67

enum class Op
{
	ERROR, // for all other instructions
	ZE,	   
	ADD,
	SUB,
	ADDI,
	XOR,
	ANDI,
	SRA,
	LW,
	SW,
	BLT,
	JALR
};
class instruction
{
public:
	bitset<32> instr;			   // instruction
	instruction(bitset<32> fetch); // constructor
};

class CPU
{
private:
	int dataMemory[4096];		  // data memory byte addressable in little endian fashion;
	unsigned long PC;		  // pc
	int32_t registerFile[32]; // Registers x0-x31
	int rtypeCount;
	int totalClockCycles;
	int instructionCount;
	double ipc;

	struct RISC_V_Decode
	{
		int32_t rs1 = 0;
		int32_t rs2 = 0;
		uint32_t rd = 0;
		int32_t immediate = 0;
		Op operation = Op::ZE;
	} decodeInstr;

	struct RISC_V_EXECUTE
	{
		int32_t aluResult = 0;
		int32_t rs2 = 0;
		uint32_t rd = 0;
		Op operation = Op::ZE;
	} executeInstr;

	struct RISC_V_MEMORY
	{
		uint32_t rd = 0;
		int32_t aluResult = 0;
		int32_t dataMem = 0;
		Op operation = Op::ZE;
	} memInstr;

public:
	CPU();
	unsigned long readPC();
	bitset<32> Fetch(bitset<8> *instmem);
	bool Decode(instruction *instr);
	void Execute();
	void Memory();
	void Writeback();
	void printRegs();
};
