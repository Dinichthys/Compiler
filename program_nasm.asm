section .data

	StartRam dq 0

section .text

global _start

extern hlt_syscall
extern in_syscall
extern out_syscall

_start:
	mov rcx, StartRam	 ; Set label to global variables counter register
	mov rbx, StartRam
	add rbx, 0	 ; Set value to tmp variables counter register

	add rbx, 16	 ; Shift tmp counter
call main	 ; Calling function
	sub rbx, 16	 ; Shift back tmp counter

	mov qword [rbx+8], rax	 ; Save return value to tmp var

	add rsp, 0	 ; Shift RSP to skip arguments
	push qword [rbx+8]	 ; Push tmp 1 to argument 0 through the stack
call hlt_syscall	 ; System call
	mov qword [rbx+0], rax	 ; Save return value to tmp

main:	 ; Function body
	push rcx	 ; Save variables counter register value
	push rbx	 ; Save tmp variables counter register value
	push rbp	 ; Save RBP

	mov rcx, rbx	 ; Move tmp var counter reg value to the var counter reg

	add rbx, 16	 ; Make a stack frame for local variables

	mov rdx, rsp	 ; Save old RSP value
	add rsp, 24
	mov rbp, rsp	 ; Make RBP point on the last arg
	mov rsp, rdx	 ; Save old RSP value
	push 0x00000000
	pop qword [rbx+0]	 ; Push the number 0 to tmp through the stack
	push qword [rbx+0]	 ; Push tmp to variable through the stack
	pop qword [rcx+0]

	push 0x40a00000
	pop qword [rbx+8]	 ; Push the number 5 to tmp through the stack
	push qword [rbx+8]	 ; Push tmp to variable through the stack
	pop qword [rcx+0]

	push 0x41200000
	pop qword [rbx+16]	 ; Push the number 10 to tmp through the stack
	push qword [rbx+16]	 ; Push tmp to variable through the stack
	pop qword [rcx+8]

	push qword [rcx+0]	 ; Push variable to tmp through the stack
	pop qword [rbx+24]

	push qword [rbx+24]	 ; Push tmp 3 to argument 0 through the stack
call in_syscall	 ; System call
	mov qword [rbx+32], rax	 ; Save return value to tmp
	pop rdx	 ; Pop argument out of stack

	push qword [rbx+32]	 ; Push tmp to variable through the stack
	pop qword [rcx+0]

	push qword [rcx+0]	 ; Push variable to tmp through the stack
	pop qword [rbx+40]

	push qword [rbx+40]	 ; Push tmp 5 to argument 0 through the stack
call out_syscall	 ; System call
	push 0x3f800000
	pop qword [rbx+56]	 ; Push the number 1 to tmp through the stack

	push qword [rbx+56]
	pop rax	 ; Save return value to the rax
	pop rbp	 ; Get RBP value back
	pop rbx	 ; Get tmp register value back
	pop rcx	 ; Get var register value back
ret

func_1_1:	 ; Function body
	push rcx	 ; Save variables counter register value
	push rbx	 ; Save tmp variables counter register value
	push rbp	 ; Save RBP

	mov rcx, rbx	 ; Move tmp var counter reg value to the var counter reg

	add rbx, 8	 ; Make a stack frame for local variables

	mov rdx, rsp	 ; Save old RSP value
	add rsp, 24
	mov rbp, rsp	 ; Make RBP point on the last arg
	mov rsp, rdx	 ; Save old RSP value
	push rbp	 ; Save RBP for one memory access
	add rbp, 8	 ; Save value temporary to RBP
	push qword [rbp]	 ; Push argument to variable through the stack
	pop qword [rcx+0]
	pop rbp	 ; Save value to RBP

func_1_1_label0:	 ; Label in function
	push qword [rcx+0]	 ; Push variable to tmp through the stack
	pop qword [rbx+0]

	push 0x3f800000
	pop qword [rbx+8]	 ; Push the number 1 to tmp through the stack
	movq xmm0, qword [rbx+0]	 ; Push first operand
	movq xmm1, qword [rbx+8]	 ; Push second operand
	cmpeqss xmm0, xmm1	 ; Comparison
	movq rdx, xmm0	 ; Move the result of comparison to the unused reg
	shr rdx, 31	 ; Make the result 1 or 0
	mov qword [rbx+16], rdx	 ; Move the result to the tmp var

	mov rdx, qword [rbx+16]
	test rdx, rdx	 ; Compare the result of the condition
jne func_1_1_label1	 ; Jump on label if the condition is true
jmp func_1_1_label2	 ; Jump on label
func_1_1_label1:	 ; Label in function
	push qword [rcx+0]	 ; Push variable to tmp through the stack
	pop qword [rbx+24]


	push qword [rbx+24]
	pop rax	 ; Save return value to the rax
	pop rbp	 ; Get RBP value back
	pop rbx	 ; Get tmp register value back
	pop rcx	 ; Get var register value back
ret
jmp func_1_1_label3	 ; Jump on label
func_1_1_label2:	 ; Label in function
	push qword [rcx+0]	 ; Push variable to tmp through the stack
	pop qword [rbx+32]

	push qword [rcx+0]	 ; Push variable to tmp through the stack
	pop qword [rbx+40]

	push 0x3f800000
	pop qword [rbx+48]	 ; Push the number 1 to tmp through the stack
	movq xmm0, qword [rbx+40]	 ; Push first operand
	movq xmm1, qword [rbx+48]	 ; Push second operand
	subss xmm0, xmm1
	movq qword [rbx+56], xmm0	 ; Move the result to the tmp var

	push qword [rbx+56]	 ; Push tmp 7 to argument 0 through the stack
	add rbx, 72	 ; Shift tmp counter
call func_1_1	 ; Calling function
	sub rbx, 72	 ; Shift back tmp counter

	mov qword [rbx+64], rax	 ; Save return value to tmp var

	add rsp, 8	 ; Shift RSP to skip arguments
	movq xmm0, qword [rbx+32]	 ; Push first operand
	movq xmm1, qword [rbx+64]	 ; Push second operand
	mulss xmm0, xmm1
	movq qword [rbx+72], xmm0	 ; Move the result to the tmp var


	push qword [rbx+72]
	pop rax	 ; Save return value to the rax
	pop rbp	 ; Get RBP value back
	pop rbx	 ; Get tmp register value back
	pop rcx	 ; Get var register value back
ret
func_1_1_label3:	 ; Label in function
