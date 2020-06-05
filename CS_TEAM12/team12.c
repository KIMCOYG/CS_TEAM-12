/*
 * team12.c
 *
 *  Created on: 2020. 5. 12.
 *      Author: hp
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX 0x10000000
#define REG_SIZE 32
#define BUF 100000

#define zero 0
#define at 1 // assembler temporary
#define v0 2 // value for function returns and expression evaluation
#define v1 3
#define a0 4 // function arguments
#define a1 5
#define a2 6
#define a3 7
#define t0 8 // temporaries
#define t1 9
#define t2 10
#define t3 11
#define t4 12
#define t5 13
#define t6 14
#define t7 15
#define s0 16 // saved temporaries
#define s1 17
#define s2 18
#define s3 19
#define s4 20
#define s5 21
#define s6 22
#define s7 23
#define t8 24 // temporaries
#define t9 25
#define k0 26 // reserved for OS kernel
#define k1 27
#define gp 28 // global pointer
#define sp 29 // stack pointer
#define fp 30 // frame pointer
#define ra 31 // return address

//ALU
#define ADD 8
#define SUB 9
#define SLL 1
#define SRL 2
#define SRA 3
#define SL 4
#define NOSHIFT 0
#define AND 12
#define OR 13
#define XOR 14
#define NOR 15

typedef union instructionResister{
	unsigned int I;

	struct RF{
		unsigned int fn: 6; //Opcode Extension
		unsigned int sh: 5; //Shift Amount
		unsigned int rd: 5;	//Destination Register
		unsigned int rt: 5; //Source Register 2
		unsigned int rs: 5; //Source Register 1
		unsigned int op: 6; //Opcode
	} RI; //R

	struct IF{
		unsigned int operand: 16; //Immediate Operand or Address Offset
		unsigned int rt: 5; //Destination or Data
		unsigned int rs: 5; //Source or Base
		unsigned int op: 6;
	} II; //I

	struct JF{
		unsigned int ad: 26; //Memory Word Address
		unsigned int op: 6; //Opcode
	} JI;

} IR; //Instruction Register

//ALU
int ALU(int X, int Y, int C, int *Z);
int addSubtract(int X, int Y, int C);
int logicOperation(int X, int Y, int C);
int shiftOperation(int V, int Y, int C);
int checkZero(int S){ return S==0; }
int checkSetLess(int X, int Y){ return X < Y; }

//Memory
unsigned char dataMem[MAX], progMem[MAX], stackMem[MAX]; //data, program, stack
int MEM(unsigned int A, int V, int nRW, int S); //Memory Access

//Register
unsigned int R[REG_SIZE]; //Register Array
unsigned int PC = 0, HI = 0, LO = 0; //PC, instruction register
unsigned int bp = 0; //breakpoint
int accessReg(unsigned int A, unsigned int V, int nRW); //Register Access


//Command Line(l, j, g, s, m, r, x, sr, sm)
void loadMemory(char *fname); //Load
void setPC(unsigned int val){ PC = val; } //Set PC, Jump Program
void showRegister(); //r
int setSr(unsigned int A, unsigned int V){ accessReg(A, V, 1); return 0; } //sr
void setMem(unsigned int location, unsigned int value){ MEM(location, value, 1, 2); } //sm
void readMem(unsigned int start, unsigned int end);
void setBreakPoint(unsigned int val){ bp = val; } //Set BreakPoint
unsigned int getBreakPoint(){ return bp; } //Get BreakPoint
//step
unsigned int getRiOp(IR ir){return ir.RI.op;}
unsigned int getRiRs(IR ir){return ir.RI.rs;}
unsigned int getRiRt(IR ir){return ir.RI.rt;}
unsigned int getRiRd(IR ir){return ir.RI.rd;}
unsigned int getRiSh(IR ir){return ir.RI.sh;}
unsigned int getRiFn(IR ir){return ir.RI.fn;}
unsigned int getIiOp(IR ir){return ir.II.op;}
unsigned int getIiRs(IR ir){return ir.II.rs;}
unsigned int getIiRt(IR ir){return ir.II.rt;}
unsigned int getIiOper(IR ir){return ir.II.operand;}
unsigned int getJiOp(IR ir){return ir.JI.op;}
unsigned int getJiAd(IR ir){return ir.JI.ad;}
int step();



//Main
int main(){
	char *cmd = (char*)malloc(sizeof(char)*10); //command
	char *fname = (char*)malloc(sizeof(char)*20); //file name
	char *rn = (char*)malloc(sizeof(char)*10); //input number
	char *value = (char*)malloc(sizeof(char)*10); //input value
	unsigned int temp_rn, temp_v;
	unsigned int pc;
//	unsigned int bp = 0;
	int EXIT = 1, is_loaded = 0, step_result = 0;

	while(EXIT){
		printf("Enter command: ");
		fflush(stdout);
		scanf("%s", cmd);
		getchar();

		if(!strcmp(cmd, "x"))
			EXIT = 0;
		else if(!strcmp(cmd, "l")){ //load
			fflush(stdout);
			scanf("%s", fname);
			getchar();
			loadMemory(fname);
			is_loaded = 1;
		}
		else if(!is_loaded){
			printf("Binary file is not loaded\n");
			continue;
		}
		else if(!strcmp(cmd, "j")){ //jump
			fflush(stdout);
			scanf("%x", &pc);
			getchar();
			setPC(pc);
		}
		else if(!strcmp(cmd, "g")){ //go
			/*while(PC!=0&&PC!=bp)
				step();
			setPC(0x400000);*/
			while(PC!=0){
				step_result = step();
				if(step_result == 404)
					break;
			}
			if(PC==0)
				setPC(0x400000);
		}
		else if(!strcmp(cmd, "s")){ //step
			if(PC==0)
				setPC(0x400000);
			step();
		}
		else if(!strcmp(cmd, "m")){ //start~end 범위 메모리 내용 출력
			unsigned int start, end;

			fflush(stdout);
			scanf("%x %x", &start, &end);
			getchar();

//			readMem(start, end);
			for (int i = start; i <= end; i += 4)
				printf("MEM[%x]\t\t%08x\n", i, MEM(i, 0, 0, 2));
		}
		else if(!strcmp(cmd, "r")){ //현재 레지스터 내용 출력
			showRegister();
		}
		else if(!strcmp(cmd, "sr")){ //특정 레지스터 값 설정
			fflush(stdout);
			scanf("%s", rn);
			getchar();
			fflush(stdout);
			scanf("%s", value);
			getchar();

			temp_rn = atoi(rn);
			temp_v = (unsigned int)strtoul(value, NULL, 16);

			if(!temp_v)
				printf("Error value\n");
			else if((temp_rn>31) || (temp_rn<0))
				printf("Error register number\n");
			else if(!setSr(temp_rn, temp_v))
				printf("Success set register\n");
			else
				printf("Error set register\n");

		}
		else if(!strcmp(cmd, "sm")){ //메모리 특정 주소 값 설정
			fflush(stdout);
			scanf("%s", rn);
			getchar();
			fflush(stdout);
			scanf("%s", value);
			getchar();

			temp_rn = (unsigned int)strtoul(rn, NULL, 16);
			temp_v = (unsigned int)strtoul(value, NULL, 16);

			printf("%x %x\n", temp_rn, temp_v);

			setMem(temp_rn, temp_v);
		}
		else if(!strcmp(cmd, "b")){
			fflush(stdout);
			scanf("%x", &pc);
			getchar();
//			bp = pc;
			setBreakPoint(pc);
		}
		else if(!strcmp(cmd, "getb"))
			printf("Breakpoint\t\t%x\n", getBreakPoint());
		/*else if(!strcmp(cmd, "x")) //시뮬레이터 프로그램 종료
			EXIT = 0;*/
	}

	printf("프로그램 종료\n");

	free(fname);
	free(cmd);
	free(rn);
	free(value);

	return 0;
}

// ALU
int ALU(int X, int Y, int C, int *Z){
	int c32, c10;
	int ret;

	c32 = (C>>2) & 3;
	c10 = C & 3;

	if(c32==0)
		ret = shiftOperation(X, Y, c10);
	else if(c32==1)
		ret = checkSetLess(X, Y);
	else if(c32==2){
		c10 = C & 1;
		ret = addSubtract(X, Y, c10);
		*Z = checkZero(ret);
	}
	else //logic
		ret = logicOperation(X, Y, c10);

	return ret;
}

int addSubtract(int X, int Y, int C){
	int ret;

	if(C<0 || C>1){ //error
		printf("Error in add/subtract operation\n");
		exit(1);
	}
	if(C==0) //add
		ret = X + Y;
	else //subtract
		ret = X - Y;

	return ret;
}

int logicOperation(int X, int Y, int C){
	if(C<0 || C>3){
		printf("Error in logic operation\n");
		exit(1);
	}
	if(C==0) //and
		return X & Y;
	else if(C==1) //or
		return X | Y;
	else if(C==2) //xor
		return X ^ Y;
	else //nor
		return ~(X | Y);
}

int shiftOperation(int V, int Y, int C){
	int ret;
//	int x = Y & 31;

	if(C<0 || C>3){ //error
		printf("Error in shift operation\n");
		exit(1);
	}

	if(C==0) //No shift
		ret = V;
	else if(C==1) //Logical left
		ret = V << Y; //(unsigned int)x
	else if(C==2) //Logical right
		ret = (unsigned int)V >> Y; //x
	else //Arith right
		ret = V >> Y; //x

	return ret;
}

//Memory Access
int MEM(unsigned int A, int V, int nRW, int S){
	unsigned int sel, offset;
	unsigned char *pM;

	sel = A >> 20;
	offset = A & 0xFFFFF;

	if(sel==0x004){ //프로그램 메모리 접근 성공
//		printf("Program Memory Access\n");
		pM = progMem;
	}
	else if(sel==0x100){ //데이터 메모리 접근 성공
//		printf("Data Memory Access\n");
		pM = dataMem;
	}
	else if(sel==0x7FF){ //스택 메모리 접근 성공
//		printf("Stack Memory Access\n");
		pM = stackMem;
	}
	else{ //메모리 접근 실패
		printf("No memory\n");
		return 1;
	}

	pM = pM + offset; //주소값 지정

	if(S==0){ //Byte
		if(nRW==0){ //Read
			return (char)pM[0]; //1byte - char type casting
		}
		else{ //Write
//			printf("Write Value:		%d\n", V);
			pM[0] = (unsigned char)V;
			return 0;
		}
	}
	else if(S==1){ //Half word
		if(offset%2!=0){
			printf("Half word Offset Error\n");
			return 0;
		}

		if(nRW==0){ //Read
			//Big-endian 방식으로 read
			return (short)(pM[0]<<8) | (short)pM[1]; //2byte - short type casting
		}
		else if(nRW==1){ //Write
//			printf("Write Value:		%d\n", V);

			//Big-endian 방식으로 write
			//V를 4바이트 중 가장 오른쪽 1바이트로 쉬프트하여 1바이트 씩 쪼개서 데이터 메모리에 저장
			pM[0] = (unsigned char)(V>>8);
			pM[1] = (unsigned char)V;

			return 0;
		}
	}
	else if(S==2){ //word
		if(offset%4!=0){
			printf("Word Offset Error\n");
			return 0;
		}

		if(nRW==0){
			//Big-endian 방식으로 read
			return (int)(pM[0]<<24) | (int)(pM[1]<<16) | (int)(pM[2]<<8) | (int)pM[3];
		}
		else if(nRW==1){ //Write
//			printf("Write Value:		%d\n", V);
			//Big-endian 방식으로 write
			//V를 4바이트 중 가장 오른쪽 1바이트로 쉬프트하여 1바이트 씩 쪼개서 데이터 메모리에 저장
			pM[0] = (unsigned char)(V>>24);
			pM[1] = (unsigned char)(V>>16);
			pM[2] = (unsigned char)(V>>8);
			pM[3] = (unsigned char)V;

			return 0;
		}
	}
	else{ //Byte, Half Word, Word 모두 아닌 에러
		printf("Error\n");
		return 0;
	}
}

//Register Access
int accessReg(unsigned int A, unsigned int V, int nRW){
	if(A>ra){
		printf("Error, A is wrong number\n");
		return 1;
	}

	if(nRW==0)
		return R[A];
	else if(nRW==1)
		R[A] = V;

	return 0;
}

//Command Line
void loadMemory(char *fname){ //Load
	FILE *pFile = NULL;
	unsigned char Buf[BUF], subBuf[4];

	int NOI, NOD; //명령어 갯수, 데이터 갯수
	int index = 8, i = 0;
	bp = 0; //BreakPoint 0으로 초기화

//	printf("%s start\n", fname);

	if(fopen_s(&pFile, fname, "rb")){
		printf("Cannot open file\n");
	}

	while(feof(pFile)==0){
		fread(subBuf, sizeof(int), 1, pFile);

		i += 4;
		Buf[i-4] = subBuf[0];
		Buf[i-3] = subBuf[1];
		Buf[i-2] = subBuf[2];
		Buf[i-1] = subBuf[3];
	}

	NOI = (256*256*256*(int)Buf[0]) + (256*256*(int)Buf[1]) + (256*(int)Buf[2]) + (1*(int)Buf[3]); //명령어 갯수 계산
	NOD = (256*256*256*(int)Buf[4]) + (256*256*(int)Buf[5]) + (256*(int)Buf[6]) + (1*(int)Buf[7]); //데이터 갯수 계산

	printf("NOI: %d, NOD: %d\n", NOI, NOD);
	printf("%s start\n", fname);

	for(i=0;i<NOI;i++){ //명령어 저장
//		printf("%d ", index);
		int instruction = 0;
		instruction = (256*256*256*(int)Buf[index]) + (256*256*(int)Buf[index+1]) + (256*(int)Buf[index+2]) + (1*(int)Buf[index+3]); //4바이트를 16진수 값으로 변환
		MEM(0x400000 + i*4, instruction, 1, 2);
		index += 4;
	}

//	printf("\n");

	for(i=0;i<NOD;i++){ //데이터 저장
//		printf("%d ", index);
		int data = 0;
		data = (256*256*256*(int)Buf[index]) + (256*256*(int)Buf[index+1]) + (256*(int)Buf[index+2]) + (1*(int)Buf[index+3]); //4바이트를 16진수 값으로 변환
		MEM(0x10000000 + i*4, data, 1, 2);
		index += 4;
	}

	setPC(0x400000);
	accessReg(sp, 0x80000000, 1);
//	printf("load memory func exit\n");
	fclose(pFile);
}

void showRegister(){ //sr
	printf("[REGISTER]\n");
	for(int i=0;i<REG_SIZE;i++)
		printf("R%d = %x\n", i, accessReg(i, 0, 0));
	printf("PC: %x\n", PC);
}

void readMem(unsigned int start, unsigned int end) { //m
	printf("start %x,  end %x \n", start, end);
	for (int j = 0; j < 3; j++) {
		for (unsigned int i = start; i < end; i += 4) {
			if (progMem[i] != 0 && j == 0) {
				printf("i    :    %d,  progMEM : %x\n", i, progMem[i]);
				printf("i + 1:    %d,  progMEM : %x\n", i, progMem[i + 1]);
				printf("i + 2:    %d,  progMEM : %x\n", i, progMem[i + 2]);
				printf("i + 3:    %d,  progMEM : %x\n", i, progMem[i + 3]);
			}
			if (dataMem[i] != 0 && j == 1) {
				printf("i    :    %d,  dataMEM : %x\n", i, dataMem[i]);
				printf("i + 1:    %d,  dataMEM : %x\n", i, dataMem[i + 1]);
				printf("i + 2:    %d,  dataMEM : %x\n", i, dataMem[i + 2]);
				printf("i + 3:    %d,  dataMEM : %x\n", i, dataMem[i + 3]);
			}
			if (stackMem[i] != 0 && j == 2) {
				printf("i    :    %d,  stakMEM : %x\n", i, stackMem[i]);
				printf("i + 1:    %d,  stakMEM : %x\n", i, stackMem[i + 1]);
				printf("i + 2:    %d,  stakMEM : %x\n", i, stackMem[i + 2]);
				printf("i + 3:    %d,  stakMEM : %x\n", i, stackMem[i + 3]);
			}
		}
	}
}

int step(){ //step
	IR inst;
	inst.I = MEM(PC, 0, 0, 2);
	char ins[10] = "\0";
	unsigned int riOp = getRiOp(inst), riRs = getRiRs(inst), riRt = getRiRt(inst), riRd = getRiRd(inst), riSh = getRiSh(inst), riFn = getRiFn(inst);
	unsigned int iiOp = getIiOp(inst), iiRs = getIiRs(inst), iiRt = getIiRt(inst), iiOper = getIiOper(inst);
	unsigned int jiOp = getJiOp(inst), jiAd = getJiAd(inst);
	unsigned int val = 0;
	int Z = 0;


	printf("PC[%x]\t\t", PC);
	PC += 4;

	switch(riOp){
	case 1:
		strcpy(ins, "bltz"); //
		printf("%s %d %d\n", ins, iiRs, iiOper*4);
		if(ALU(accessReg(iiRs,0,0),0,SL,1)==1)
			setPC(PC+iiOper*4-4);
		break;
	case 2:
		strcpy(ins, "j");
		printf("%s 0x%08x\n", ins, jiAd*4);
		setPC(jiAd*4);
		break;
	case 3:
		strcpy(ins, "jal");
		printf("%s 0x%08x\n", ins, jiAd*4);
		accessReg(ra, PC, 1);
		setPC(jiAd*4);
		break;
	case 4: //
		strcpy(ins, "beq");
		printf("%s $%d, $%d, %d\n", ins, iiRs, iiRt, iiOper*4);
		if(ALU(accessReg(iiRs,0,0), accessReg(iiRt,0,0), SUB, &Z) == 0)
			setPC(PC+iiOper*4-4);
		break;
	case 5: //
		strcpy(ins, "bne");
		printf("%s $%d, $%d, %d\n", ins, iiRs, iiRt, iiOper*4);
		if(ALU(accessReg(iiRs,0,0), accessReg(iiRt,0,0), SUB, &Z) != 0)
				setPC(PC+iiOper*4-4);
		break;

	case 8:
		strcpy(ins, "addi");
		printf("%s $%d, $%d, %d\n", ins, iiRt, iiRs, (short)iiOper);
		val = ALU(accessReg(iiRs,0,0), iiOper, ADD, &Z);
//		printf("%d %d %d\n", iiOper, (short)iiOper, (int)iiOper);
		accessReg(iiRt, val, 1); //!!
		break;
	case 10:
		strcpy(ins, "slti");
		printf("%s $%d, $%d, %d\n", ins, iiRt, iiRs, (short)iiOper);
		val = ALU(accessReg(iiRs,0,0), iiOper, SL, &Z);
		accessReg(iiRt, val, 1); //!!
		break;
	case 12:
		strcpy(ins, "andi");
		printf("%s $%d, $%d, %d\n", ins, iiRt, iiRs, (short)iiOper);
		val = ALU(accessReg(iiRs,0,0), iiOper, AND, &Z);
//		printf("%d %d %d\n", iiOper, (short)iiOper, (int)iiOper);
		accessReg(iiRt, val, 1); //!
		break;
	case 13:
		strcpy(ins, "ori");
		printf("%s $%d, $%d, %d\n", ins, iiRt, iiRs, (short)iiOper);
		val = ALU(accessReg(iiRs,0,0), iiOper, OR, &Z);
//		printf("%d %d %d\n", iiOper, (short)iiOper, (int)iiOper);
		accessReg(iiRt, val, 1);
		break;
	case 14:
		strcpy(ins, "xori");
		printf("%s $%d, $%d, %d\n", ins, iiRt, iiRs, (short)iiOper);
		val = ALU(accessReg(iiRs,0,0), iiOper, XOR, &Z);
		accessReg(iiRt, val, 1);
		break;
	case 15:
		strcpy(ins, "lui"); //
		printf("%s $%d, %d\n", ins, iiRt, (short)iiOper);
		val = ALU(iiOper, 16, SLL, &Z);
		accessReg(iiRt, val, 1);
		break;

	case 32:
		strcpy(ins, "lb"); //바이트 단위로 데이터 로드
		printf("%s $%d, %d($%d)\n", ins, iiRt, iiOper, iiRs);
		//Sign Extend to 32 bits in rt -> (int)
		val = (int)MEM(accessReg(iiRs, 0, 0)+(short)iiOper, 0, 0, 2);
		accessReg(iiRt, val, 1);
		break;
	case 35:
		strcpy(ins, "lw"); //
		printf("%s $%d, %d($%d)\n", ins, iiRt, iiOper, iiRs);
		val = MEM(accessReg(iiRs, 0, 0)+iiOper, 0, 0, 2);
		accessReg(iiRt, val, 1);
		break;
	case 36:
		strcpy(ins, "lbu"); //
		printf("%s $%d, %d($%d)\n", ins, iiRt, iiOper, iiRs);
		//Zero Extend to 32 bits in rt
		val = (unsigned int)MEM(accessReg(iiRs, 0, 0)+iiOper, 0, 0, 2);
		accessReg(iiRt, val, 1);
		break;

	case 40:
		strcpy(ins, "sb"); //
		printf("%s $%d, %d($%d)\n", ins, iiRt, iiOper, iiRs);
		//Store just rightmost byte/halfword
		MEM(accessReg(iiRs,0,0)+iiOper, (int)accessReg(iiRt,0,0), 1, 2);
		break;
	case 43:
		strcpy(ins, "sw");
		printf("%s $%d, %d($%d)\n", ins, iiRt, iiOper, iiRs); //
		MEM(accessReg(iiRs,0,0)+iiOper, accessReg(iiRt,0,0), 1, 2);
		break;
	default:
		switch(riFn){
		case 0:
			strcpy(ins, "sll");
			printf("%s $%d, $%d, %d\n", ins, riRd, riRt, riSh);
			val = ALU(accessReg(riRt,0,0), riSh, SLL, &Z);
			accessReg(riRd, val, 1);
			break;
		case 2:
			strcpy(ins, "srl");
			printf("%s $%d, $%d, %d\n", ins, riRd, riRt, riSh);
			val = ALU(accessReg(riRt,0,0), riSh, SRL, &Z);
			accessReg(riRd, val, 1);
			break;
		case 3:
			strcpy(ins, "sra");
			printf("%s $%d, $%d, %d\n", ins, riRd, riRt, riSh);
			val = ALU(accessReg(riRt,0,0), riSh, SRA, &Z);
			accessReg(riRd, val, 1);
			break;

		case 8:
			strcpy(ins, "jr");
			printf("%s $%d\n", ins, riRs);
			setPC(accessReg(ra,0,0));
			break;
		case 12:
			strcpy(ins, "syscall");
			printf("%s %d\n", ins, accessReg(2,0,0));
			if(accessReg(2,0,0)==10)
				setPC(0);
			break;

		case 16:
			strcpy(ins, "mfhi"); //
			printf("%s %d\n", ins, riRs);
			accessReg(accessReg(riRs,0,0), HI, 1);
			break;
		case 18:
			strcpy(ins, "mflo"); //
			printf("%s %d", ins, riRs);
			accessReg(accessReg(riRs,0,0), LO, 1);
			break;

		case 24:
			strcpy(ins, "mul");
			printf("%s $%d, $%d, $%d\n", ins, riRd, riRs, riRt);
			accessReg(riRd, riRs*riRt, 1);
			break;
		case 32:
			strcpy(ins, "add");
			printf("%s $%d, $%d, $%d\n", ins, riRd, riRs, riRt);
			val = (unsigned int)ALU(accessReg(riRs,0,0), accessReg(riRt,0,0), ADD, &Z);
			accessReg(riRd, val, 1);
			break;
		case 34:
			strcpy(ins, "sub");
			printf("%s $%d, $%d, $%d\n", ins, riRd, riRs, riRt);
			val = (unsigned int)ALU(accessReg(riRs,0,0), accessReg(riRt,0,0), SUB, &Z);
			accessReg(riRd, val, 1);
			break;
		case 36:
			strcpy(ins, "and");
			printf("%s $%d, $%d, $%d\n", ins, riRd, riRs, riRt);
			val = (unsigned int)ALU(accessReg(riRs,0,0), accessReg(riRt,0,0), AND, &Z);
			accessReg(riRd, val, 1);
			break;
		case 37:
			strcpy(ins, "or");
			printf("%s $%d, $%d, $%d\n", ins, riRd, riRs, riRt);
			val = (unsigned int)ALU(accessReg(riRs,0,0), accessReg(riRt,0,0), OR, &Z);
			accessReg(riRd, val, 1);
			break;
		case 38:
			strcpy(ins, "xor");
			printf("%s $%d, $%d, $%d\n", ins, riRd, riRs, riRt);
			val = (unsigned int)ALU(accessReg(riRs,0,0), accessReg(riRt,0,0), XOR, &Z);
			accessReg(riRd, val, 1);
			break;
		case 39:
			strcpy(ins, "nor");
			printf("%s $%d, $%d, $%d\n", ins, riRd, riRs, riRt);
			val = (unsigned int)ALU(accessReg(riRs,0,0), accessReg(riRt,0,0), NOR, &Z);
			accessReg(riRd, val, 1);
			break;
		case 42:
			strcpy(ins, "slt");
			printf("%s $%d, $%d, $%d\n", ins, riRd, riRs, riRt);
			val = (unsigned int)ALU(accessReg(riRs,0,0), accessReg(riRt,0,0), SL, &Z);
			accessReg(riRd, val, 1);
			break;
		}
	}
	if(PC == bp)
		return 404;
	return 0;
}
