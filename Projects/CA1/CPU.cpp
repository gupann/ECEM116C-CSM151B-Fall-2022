#include "CPU.h"

instruction::instruction(bitset<32> fetch)
{
	//cout << fetch << endl;
	instr = fetch;
	//cout << instr << endl;
}

CPU::CPU()
{
	PC = 0; // set PC to 0
	rtypeCount = 0;		// to keep count of r type instructions
	totalClockCycles = 0;	// to keep count of clock cycles 
	instructionCount = 0;		// to keep count of no of instructions in a program

	for (int i = 0; i < 4096; i++) // copy instrMEM
	{
		dataMemory[i] = (0); // initialize data memory with all 0s
	}

	for (int j = 0; j < 32; j++)
	{
		registerFile[j] = (0); // initialize reg file with all 0s
	}
}

bitset<32> CPU::Fetch(bitset<8> *instmem)
{
	bitset<32> instr = ((((instmem[PC + 3].to_ulong()) << 24)) + ((instmem[PC + 2].to_ulong()) << 16) + ((instmem[PC + 1].to_ulong()) << 8) + (instmem[PC + 0].to_ulong())); // get 32 bit instruction
	PC += 4;																																								 // increment PC
	return instr;	// parses the input file and takes in 4 numbers one by one corresponding to each byte of a 4 byte instruction to give machine code
}

bool CPU::Decode(instruction *curr)
{
	//cout << "curr instr: " << curr->instr << endl;

	uint32_t currInstr;
	currInstr = (uint32_t)(curr->instr.to_ulong());

	// parse 32 bit instr and identify opcode, funcs, rs1, rs2

	uint32_t opcode = (currInstr)&0x7f;
	uint32_t func3 = (currInstr >> 12) & 0x7;
	uint32_t func7 = (currInstr >> 25) & 0x7f;
	uint32_t rs1 = (currInstr >> 15) & 0x1f;
	uint32_t rs2 = (currInstr >> 20) & 0x1f;

	// prepare input for execute stage
	decodeInstr.rs1 = registerFile[rs1];
	decodeInstr.rs2 = registerFile[rs2];
	decodeInstr.rd = (currInstr >> 7) & 0x1f;
	decodeInstr.immediate = (int32_t)curr->instr.to_ulong() >> 20; // sign extension

	// RTYPE operation
	if (opcode == RTYPE)
	{
		//cout << "R-type instr" << endl;

		if (func3 == 0x0)
		{
			if (func7 == 0x0)
			{
				decodeInstr.operation = Op::ADD;
			}
			else if (func7 == 0x20)
			{
				decodeInstr.operation = Op::SUB;
			}
			else
			{
				decodeInstr.operation = Op::ERROR;
				//cout << "##### opcode error:" << opcode << endl;
			}
		}
		else if ((func3 == 0x4) && (func7 == 0x0))
		{
			decodeInstr.operation = Op::XOR;
		}
		else if ((func3 == 0x5) && (func7 == 0x20))
		{
			decodeInstr.operation = Op::SRA;
		}
		else
		{
			decodeInstr.operation = Op::ERROR;
			//cout << "##### opcode error:" << opcode << endl;
		}

		rtypeCount++;
		instructionCount++;
	}
	//ITYPE operation
	else if (opcode == ITYPE)
	{
		//cout << "I-type instr" << endl;

		if (func3 == 0x0)
		{
			decodeInstr.operation = Op::ADDI;
		}
		else if (func3 == 0x7)
		{
			decodeInstr.operation = Op::ANDI;
		}
		else
		{
			decodeInstr.operation = Op::ERROR;
			//cout << "##### opcode error:" << opcode << endl;
		}
		instructionCount++;

	}
	// LW operation
	else if (opcode == LOADWORD && func3 == 0x2)
	{
		//cout << "LW-type instr" << endl;
		decodeInstr.operation = Op::LW;
		instructionCount++;
	}

	// SW operation
	else if (opcode == STOREWORD && func3 == 0x2)
	{
		//cout << "SW-type instr" << endl;
		
		auto imm11_5 = (int32_t)(currInstr & 0xfe000000);
		auto imm4_0 = (int32_t)((currInstr & 0xf80) << 13);
		decodeInstr.immediate = (imm11_5 + imm4_0) >> 20;	// immediates bits are dispersed in the machine code as per risc v rules so we do this
		decodeInstr.operation = Op::SW;
		instructionCount++;

	}
	else if ((opcode == SBTYPE) && (func3 == 0x4))
	{
		//cout << "SB-type instr: BLT" << endl;
		//cout << "currInstr: " << currInstr << endl;
		
		auto imm0 = (int32_t)(0);  //LSB = 0 always
		auto imm4_1 = (int32_t)((currInstr & 0xf00) >> 7);
		// 0000 0000 0000 0000 0000 1111 0000 0000
		auto imm10_5 = (int32_t)((currInstr & 0x7e000000) >> 20);
		// 0111 1110 0000 0000 0000 0000 0000 0000
		auto imm11 = (int32_t)((currInstr & 0x80) << 4);
		// 0000 0000 0000 0000 0000 0000 1000 0000
		auto imm12 = (int32_t)((currInstr & 0x80000000) >> 19);
		// 1000 0000 0000 0000 0000 0000 0000 0000
		//cout << imm12 << endl;
		decodeInstr.immediate = (int32_t)((imm12 + imm11 + imm10_5 + imm4_1 + imm0)); // immediates bits are dispersed in the machine code as per risc v rules so we do this
		//cout << "imm: " << decodeInstr.immediate << endl;
		if(imm12 > 0)
		{
			decodeInstr.immediate = (int32_t)(decodeInstr.immediate | 0xfffff000);		//an extra check to deal with negative immediates
		}
		//cout << "imm: " << decodeInstr.immediate << endl;
		
		decodeInstr.operation = Op::BLT;
		instructionCount++;
	}
	else if ((opcode == UJTYPE) && (func3 == 0x0))
	{
		//cout << "UJTYPE instr: JALR" << endl;
		decodeInstr.operation = Op::JALR;
		instructionCount++;
	}
	// ZERO op code
	else if (opcode == ZERO)
	{
		//cout << "ZERO-type instr" << endl;
		decodeInstr.operation = Op::ZE;
		//cout << "no operation:" << endl;
		instructionCount++;
	}
	else
	{
		cout << "This is not part of our instruction set" << opcode << endl;
		decodeInstr.operation = Op::ERROR;
	}
	return true;
}

void CPU::Execute()
{
	//cout << "in Execute stage" << endl;

	// prepare input for next stage
	executeInstr.operation = decodeInstr.operation;
	executeInstr.rs2 = decodeInstr.rs2;
	executeInstr.rd = decodeInstr.rd;

	switch (executeInstr.operation)
	{
	case Op::ADD:
		executeInstr.aluResult = decodeInstr.rs1 + decodeInstr.rs2;
		break;

	case Op::SUB:
		executeInstr.aluResult = decodeInstr.rs1 - decodeInstr.rs2;
		break;

	case Op::SRA:
		executeInstr.aluResult = (int32_t) decodeInstr.rs1 >> (decodeInstr.rs2 & (unsigned) 0x1f);
		break;

	case Op::XOR:
		executeInstr.aluResult = decodeInstr.rs1 ^ decodeInstr.rs2;
		break;

	case Op::ADDI:
		executeInstr.aluResult = decodeInstr.rs1 + decodeInstr.immediate;
		break;

	case Op::ANDI:
		executeInstr.aluResult = decodeInstr.rs1 & decodeInstr.immediate;
		break;
	//sw and lw do similar actions
	case Op::LW: 
	case Op::SW:
		executeInstr.aluResult = decodeInstr.rs1 + decodeInstr.immediate;
		break;

	case Op::BLT:
		if (decodeInstr.rs1 < decodeInstr.rs2)
            //PC = PC + ((int32_t)decodeInstr.immediate) - 4 ;
			PC = PC + (int32_t)(decodeInstr.immediate & ~1) - 4;
            //cout << "============BLT PC " << PC << "=========" << endl;
		break;
	
	case Op::JALR:
		executeInstr.aluResult = PC;
		//PC = decodeInstr.rs1 + decodeInstr.immediate;
		PC =  (decodeInstr.rs1 + (int32_t)(decodeInstr.immediate) & ~1);
		break;
	
	case Op::ZE:
		executeInstr.aluResult = ZERO;
		break;

	case Op::ERROR:
		cout << "##### error in execute stage #####" << endl;
		break;
	}
}

void CPU::Memory()
{
	//cout << "in memory stage" << endl;
    memInstr.rd = executeInstr.rd;
    memInstr.aluResult = executeInstr.aluResult;
    memInstr.operation = executeInstr.operation; // SW or LW

    switch(executeInstr.operation) {
        case Op::LW:
			// LW loads from memory to register
			memInstr.dataMem= ((((dataMemory[executeInstr.aluResult + 3]) << 24)) | ((dataMemory[executeInstr.aluResult + 2]) << 16) | ((dataMemory[executeInstr.aluResult + 1]) << 8) | (dataMemory[executeInstr.aluResult])); // get 32 bit instruction
            break;
        case Op::SW:
			// SW stores from register to memory
			dataMemory[executeInstr.aluResult] = (executeInstr.rs2 & 0xff);
            dataMemory[executeInstr.aluResult + 1] = ((executeInstr.rs2 & 0xff00)>> 8);
            dataMemory[executeInstr.aluResult + 2] = ((executeInstr.rs2 & 0xff0000) >> 16);
            dataMemory[executeInstr.aluResult + 3] = ((executeInstr.rs2 & 0xff000000) >> 24);
            break;
        default:
            break;
    }
}

void CPU::Writeback()
{
	//cout << "in writeback stage" << endl;
	totalClockCycles++;

	if (ZERO == memInstr.rd || Op::SW == memInstr.operation) {
    	return;
	}  

	if (Op::JALR == memInstr.operation)
	{
		 registerFile[executeInstr.rd] = executeInstr.aluResult;
	}
    // lw operation reads data from memory and stores in destination register
	
	if (Op::LW == memInstr.operation) {
        registerFile[memInstr.rd] = memInstr.dataMem; }
	
	// for all other operations, other than sw, we store thr aluResult value into the destination register 
	// e.g. add rd,rs1,rs2 --> add x11, x6,x3 | in this case aluResult contains value of x6 + x3 and rd = x11 
    else if (Op::ERROR != memInstr.operation) // for all other operations
		{ registerFile[memInstr.rd] = memInstr.aluResult; }
}

void CPU::printRegs()
{
	int a0 = registerFile[10]; //x10
	int a1 = registerFile[11]; //x11
	cout << "(" << a0 << "," << a1 << ")" << endl;
	//cout << "no. of r type instructions: " << rtypeCount << endl;
	//cout << "no. clock cycles: " << totalClockCycles << endl;
	//cout << "no of instructions: " << instructionCount << endl;
	ipc = (double) instructionCount / totalClockCycles;
	//cout << "ipc: " << ipc << endl;
}

unsigned long CPU::readPC()
{
	return PC;
}
