# TODO: This is lab1.2
/* Protected Mode Hello World */
.code16

.global start
start:
	movw %cs, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss
	# TODO:关闭中断
	cli

	# 启动A20总线
	inb $0x92, %al 
	orb $0x02, %al
	outb %al, $0x92

	# 加载GDTR
	data32 addr32 lgdt gdtDesc # loading gdtr, data32, addr32

	# TODO：设置CR0的PE位（第0位）为1
	movl %cr0, %eax
	orl $0x1, %eax
	movl %eax, %cr0

	# 长跳转切换至保护模式
	data32 ljmp $0x08, $start32 # reload code segment selector and ljmp to start32, data32

.code32
start32:
	movw $0x10, %ax # setting data segment selector
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %ss
	movw $0x18, %ax # setting graphics data segment selector
	movw %ax, %gs
	
	movl $0x8000, %eax # setting esp
	movl %eax, %esp
	# TODO:输出Hello World
	#设置段选择子
	movw $0x18, %ax
	movw %ax, %gs
	#设置字体属性
	movb $0x0c, %ah
	#设置变址
	movl $0x0, %edi
	#开始填入显存
	movb $72, %al
	movw %ax, %gs:(%edi)
	addl $2, %edi
	movb $69, %al
	movw %ax, %gs:(%edi)
	addl $2, %edi
	movb $76, %al
	movw %ax, %gs:(%edi)
	addl $2, %edi
	movb $76, %al
	movw %ax, %gs:(%edi)
	addl $2, %edi
	movb $79, %al
	movw %ax, %gs:(%edi)
	addl $2, %edi
	movb $44, %al
	movw %ax, %gs:(%edi)
	addl $2, %edi
	movb $32, %al
	movw %ax, %gs:(%edi)
	addl $2, %edi
	movb $87, %al
	movw %ax, %gs:(%edi)
	addl $2, %edi
	movb $79, %al
	movw %ax, %gs:(%edi)
	addl $2, %edi
	movb $82, %al
	movw %ax, %gs:(%edi)
	addl $2, %edi
	movb $76, %al
	movw %ax, %gs:(%edi)
	addl $2, %edi
	movb $68, %al
	movw %ax, %gs:(%edi)
	addl $2, %edi
	movb $33, %al
	movw %ax, %gs:(%edi)
	addl $2, %edi
	movb $10, %al
	movw %ax, %gs:(%edi)
	addl $2, %edi
	movb $0, %al
	movw %ax, %gs:(%edi)
	addl $2, %edi



	
	


loop32:
	jmp loop32

message:
	.string "Hello, World!\n\0"



.p2align 2
gdt: # 8 bytes for each table entry, at least 1 entry
	# .word limit[15:0],base[15:0]
	# .byte base[23:16],(0x90|(type)),(0xc0|(limit[19:16])),base[31:24]
	# GDT第一个表项为空
	.word 0,0
	.byte 0,0,0,0

	# TODO：code segment entry
	.word 0xffff,0
	.byte 0,0x9a,0xcf,0

	# TODO：data segment entry
	.word 0xffff,0
	.byte 0,0x92,0xcf,0

	# TODO：graphics segment entry
	.word 0xffff,0x8000
	.byte 0x0b,0x92,0xcf,0

gdtDesc: 
	.word (gdtDesc - gdt -1) 
	.long gdt 

