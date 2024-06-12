#include "x86.h"
#include "device.h"

extern TSS tss;
extern ProcessTable pcb[MAX_PCB_NUM];
extern int current;

extern int displayRow;
extern int displayCol;

void GProtectFaultHandle(struct StackFrame *sf);

void syscallHandle(struct StackFrame *sf);

void syscallWrite(struct StackFrame *sf);
void syscallPrint(struct StackFrame *sf);

void timerHandle(struct StackFrame *sf);
void syscallFork(struct StackFrame *sf);
void syscallSleep(struct StackFrame *sf);
void syscallExit(struct StackFrame *sf);

void irqHandle(struct StackFrame *sf)
{ // pointer sf = esp
	/* Reassign segment register */
	asm volatile("movw %%ax, %%ds" ::"a"(KSEL(SEG_KDATA)));
	/*XXX Save esp to stackTop */
	uint32_t tmpStackTop = pcb[current].stackTop;
	pcb[current].prevStackTop = pcb[current].stackTop;
	pcb[current].stackTop = (uint32_t)sf;

	switch (sf->irq)
	{
	case -1:
		break;
	case 0xd:
		GProtectFaultHandle(sf);
		break;
	case 0x20:
		timerHandle(sf);
		break;
	case 0x80:
		syscallHandle(sf);
		break;
	default:
		assert(0);
	}
	/*XXX Recover stackTop */
	pcb[current].stackTop = tmpStackTop;
}

void GProtectFaultHandle(struct StackFrame *sf)
{
	assert(0);
	return;
}

void timerHandle(struct StackFrame *sf)
{
	// sleep time dec
	for(int i = 1;i < MAX_PCB_NUM; i++){
		if(pcb[i].state == STATE_BLOCKED){
			pcb[i].sleepTime -= 1;
			if(pcb[i].sleepTime == 0 )
				pcb[i].state = STATE_RUNNABLE;
		}
	}

	//current time dec
	pcb[current].timeCount++;
	if (pcb[current].timeCount == MAX_TIME_COUNT || !current){	//特殊考虑IDLE:总是最低优先级进入IDLE，总是尝试离开IDLE
		//修改本进程信息
		pcb[current].state = STATE_RUNNABLE;
		pcb[current].timeCount = 0;
		//寻找下一个进程
		int next = 0;
		for (int offset = 1; offset <= MAX_TIME_COUNT; offset++){	//轮转
			if (!(current + offset) % MAX_TIME_COUNT)
				continue;	//最低优先级进入IDLE
			if (pcb[(current + offset) % MAX_TIME_COUNT].state == STATE_RUNNABLE){
				//找到了可以执行的进程
				next = (current + offset) % MAX_TIME_COUNT;
				break;
			}
		}
		//修改pcb
		current = next;
		pcb[current].state = STATE_RUNNING;
		//切换进程
		uint32_t tmpStackTop = pcb[current].stackTop;
       	pcb[current].stackTop = pcb[current].prevStackTop;	//如果之前的中断被中断了，中断现场在stackTop而用户现场在prevStackTop
        tss.esp0 = (uint32_t)&(pcb[current].stackTop);
        asm volatile("movl %0, %%esp"::"m"(tmpStackTop)); // switch kernel stack
        asm volatile("popl %gs");
        asm volatile("popl %fs");
        asm volatile("popl %es");
        asm volatile("popl %ds");
        asm volatile("popal");
        asm volatile("addl $8, %esp");
        asm volatile("iret");
	}
}

void syscallHandle(struct StackFrame *sf)
{
	switch (sf->eax)
	{ // syscall number
	case 0:
		syscallWrite(sf);
		break; // for SYS_WRITE
	/*TODO Add Fork,Sleep... */
	case 1:
		syscallFork(sf);
		break;
	case 2:
		syscallSleep(sf);
		break;
	case 3:
		syscallExit(sf);
		break;
	default:
		break;
	}
}

void syscallWrite(struct StackFrame *sf)
{
	switch (sf->ecx)
	{ // file descriptor
	case 0:
		syscallPrint(sf);
		break; // for STD_OUT
	default:
		break;
	}
}

void syscallPrint(struct StackFrame *sf)
{
	int sel = sf->ds; // segment selector for user data, need further modification
	char *str = (char *)sf->edx;
	int size = sf->ebx;
	int i = 0;
	int pos = 0;
	char character = 0;
	uint16_t data = 0;
	asm volatile("movw %0, %%es" ::"m"(sel));
	for (i = 0; i < size; i++)
	{
		asm volatile("movb %%es:(%1), %0" : "=r"(character) : "r"(str + i));
		if (character == '\n')
		{
			displayRow++;
			displayCol = 0;
			if (displayRow == 25)
			{
				displayRow = 24;
				displayCol = 0;
				scrollScreen();
			}
		}
		else
		{
			data = character | (0x0c << 8);
			pos = (80 * displayRow + displayCol) * 2;
			asm volatile("movw %0, (%1)" ::"r"(data), "r"(pos + 0xb8000));
			displayCol++;
			if (displayCol == 80)
			{
				displayRow++;
				displayCol = 0;
				if (displayRow == 25)
				{
					displayRow = 24;
					displayCol = 0;
					scrollScreen();
				}
			}
		}
		// asm volatile("int $0x20"); //XXX Testing irqTimer during syscall
		// asm volatile("int $0x20":::"memory"); //XXX Testing irqTimer during syscall
	}

	updateCursor(displayRow, displayCol);
	// take care of return value
	return;
}

// TODO syscallFork ...
void memcpy(void* dest, void* src, size_t size){
	for(uint32_t j = 0; j < size; j++){
		*(uint8_t*)(dest + j)=*(uint8_t*)(src + j);
	}
}
void syscallFork(struct StackFrame *sf){
	//找空闲pcb
	for (int i = 1; i < MAX_PCB_NUM; i++){
		if (pcb[i].state == STATE_DEAD){
			//找到了，开始拷贝
			memcpy((void*)((i + 1) * 0x100000), (void*)((current + 1) * 0x100000), 0x100000);	
			memcpy(&pcb[i], &pcb[current], sizeof(ProcessTable));
			//开始调整pcb
			pcb[i].stackTop = (uint32_t)&(pcb[i].regs);
			pcb[i].prevStackTop = (uint32_t)&(pcb[i].stackTop);
			pcb[i].state = STATE_RUNNABLE;
			pcb[i].timeCount = 0;
			pcb[i].sleepTime = 0;
			pcb[i].pid = i;
			
			//esp不变
			//eflags不变 
			//eip不变

			pcb[i].regs.cs = USEL(2 * i + 1);
			pcb[i].regs.ss = USEL(2 * i + 2);
			pcb[i].regs.ds = USEL(2 * i + 2);
			pcb[i].regs.es = USEL(2 * i + 2);
			pcb[i].regs.fs = USEL(2 * i + 2);
			pcb[i].regs.gs = USEL(2 * i + 2);

			pcb[i].regs.eax = 0;	//子进程返回0
			pcb[current].regs.eax = i;	//主进程返回i
			return;
		}
	}
	//没有空闲pcb,返回-1
	pcb[current].regs.eax=-1;	
}

void syscallSleep(struct StackFrame *sf){
	//设置好pcb然后复用timerHandle
	pcb[current].sleepTime=sf->ecx;
	pcb[current].state=STATE_BLOCKED;
	asm volatile("int $0x20");
}	

void syscallExit(struct StackFrame *sf){
	pcb[current].state = STATE_DEAD;
	asm volatile("int $0x20");
}

