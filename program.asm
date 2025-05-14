	push 0
	pop cx	 ; Set zero to global variables counter register
	push 1
	pop bx	 ; Set value to tmp variables counter register

	push 3	 ; Push the number to tmp through the stack
	pop [bx+2]

	push [bx+2]	 ; Push tmp to variable through the stack
	pop [cx+0]

call main:	 ; Calling function
	push ax	 ; Push return value to the stack
	pop [bx+1]	 ; Save return value to tmp var
	push [bx+1]	 ; Push tmp 1 to argument 0 through the stack
hlt	 ; System call
	pop [bx+0]	 ; Save return value to tmp

main:	 ; Function body
	push cx	 ; Save variables counter register value
	push bx	 ; Save tmp variables counter register value
	push rbp	 ; Save RBP

	push bx
	pop cx	 ; Move tmp var counter reg value to the var counter reg

	push bx
	push 2
	add
	pop bx	 ; Make a stack frame for local variables

	push 4
	push rsp
	sub	 ; Make RBP point on the last arg
	pop rbp	 ; Move RSP to RBP

	push 2	 ; Push the number to tmp through the stack
	pop [bx+3]

	push 3	 ; Push the number to tmp through the stack
	pop [bx+4]

	push [bx+4]	 ; Push second operand
	push [bx+3]	 ; Push first operand
	sub
	pop [bx+5]	 ; Move tmp var counter reg value to the var counter reg

	push [bx+5]	 ; Push tmp to variable through the stack
	pop [cx+0]

	push 2	 ; Push the number to tmp through the stack
	pop [bx+6]

	push [bx+6]	 ; Push tmp to variable through the stack
	pop [cx+0]

main_label0:	 ; Label in function
	push 1	 ; Push the number to tmp through the stack
	pop [bx+7]

	push 2	 ; Push the number to tmp through the stack
	pop [bx+8]

	push [bx+8]	 ; Push second operand
	push [bx+7]	 ; Push first operand
	more
	pop [bx+9]	 ; Move tmp var counter reg value to the var counter reg

	push [bx+9]	 ; Push the result of the condition to the stack
	push 1	 ; Push true

je main_label1:	 ; Jump on label if the condition is true
jmp main_label2:	 ; Jump on label
main_label1:	 ; Label in function
	push 2	 ; Push the number to tmp through the stack
	pop [bx+10]

	push [bx+10]	 ; Push tmp to variable through the stack
	pop [cx+0]

main_label2:	 ; Label in function
	push 10	 ; Push the number to tmp through the stack
	pop [bx+7]

	push [bx+7]	 ; Push tmp to variable through the stack
	pop [cx+1]

main_label3:	 ; Label in function
	push [cx+0]	 ; Push variable to tmp through the stack
	pop [bx+8]

	push 0	 ; Push the number to tmp through the stack
	pop [bx+9]

	push [bx+9]	 ; Push second operand
	push [bx+8]	 ; Push first operand
	less
	pop [bx+10]	 ; Move tmp var counter reg value to the var counter reg

	push [bx+10]	 ; Push the result of the condition to the stack
	push 1	 ; Push true

je main_label4:	 ; Jump on label if the condition is true
jmp main_label5:	 ; Jump on label
main_label4:	 ; Label in function
	push 50	 ; Push the number to tmp through the stack
	pop [bx+11]

	push [bx+11]	 ; Push tmp to variable through the stack
	pop [cx+1]

jmp main_label3:	 ; Jump on label
main_label5:	 ; Label in function
	push [cx+0]	 ; Push variable to tmp through the stack
	pop [bx+8]

	push 2	 ; Push the number to tmp through the stack
	pop [bx+9]

	push [bx+9]	 ; Push tmp 9 to argument 0 through the stack
	push [bx+8]	 ; Push tmp 8 to argument 1 through the stack
call func_1_2:	 ; Calling function
	push ax	 ; Push return value to the stack
	pop [bx+10]	 ; Save return value to tmp var
	push [bx+10]	 ; Push tmp to variable through the stack
	pop [cx+1]

	push 2	 ; Push the number to tmp through the stack
	pop [bx+11]

	push 2	 ; Push the number to tmp through the stack
	pop [bx+12]

	push [bx+12]	 ; Push tmp 12 to argument 0 through the stack
	push [bx+11]	 ; Push tmp 11 to argument 1 through the stack
call func_1_2:	 ; Calling function
	push ax	 ; Push return value to the stack
	pop [bx+13]	 ; Save return value to tmp var
	push [bx+13]	 ; Push tmp to variable through the stack
	pop [cx+1]

	push 1	 ; Push the number to tmp through the stack
	pop [bx+14]


	push [bx+14]
	pop ax	 ; Save return value to the rax
	pop rbp	 ; Get RBP value back
	pop bx	 ; Get tmp register value back
	pop cx	 ; Get var register value back
ret

func_1_2:	 ; Function body
	push cx	 ; Save variables counter register value
	push bx	 ; Save tmp variables counter register value
	push rbp	 ; Save RBP

	push bx
	pop cx	 ; Move tmp var counter reg value to the var counter reg

	push bx
	push 2
	add
	pop bx	 ; Make a stack frame for local variables

	push 4
	push rsp
	sub	 ; Make RBP point on the last arg
	pop rbp	 ; Move RSP to RBP

	push rbp	 ; Save RBP for one memory access
	push 1
	push rbp
	sub
	pop rbp	 ; Save value temporary to RBP
	push [rbp]	 ; Push argument to variable through the stack
	pop [cx+0]
	pop rbp	 ; Save value to RBP

	push rbp	 ; Save RBP for one memory access
	push 2
	push rbp
	sub
	pop rbp	 ; Save value temporary to RBP
	push [rbp]	 ; Push argument to variable through the stack
	pop [cx+1]
	pop rbp	 ; Save value to RBP

	push [cx+0]	 ; Push variable to tmp through the stack
	pop [bx+3]

	push [bx+3]	 ; Push tmp 3 to argument 0 through the stack
out	 ; System call
	push 2	 ; Push the number to tmp through the stack
	pop [bx+5]


	push [bx+5]
	pop ax	 ; Save return value to the rax
	pop rbp	 ; Get RBP value back
	pop bx	 ; Get tmp register value back
	pop cx	 ; Get var register value back
ret
