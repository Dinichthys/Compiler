	push 0
	pop cx	 ; Set zero to global variables counter register
	push 0
	pop bx	 ; Set value to tmp variables counter register

	push 2
	push bx
	add
	pop bx	 ; Shift tmp counter
call main:	 ; Calling function
	push 2
	push bx
	sub
	pop bx	 ; Shift back tmp counter

	push ax	 ; Push return value to the stack
	pop [bx+1]	 ; Save return value to tmp var

	push 1
	push rsp
	sub
	pop rsp	 ; Shift RSP to skip arguments
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
	push 1
	add
	pop bx	 ; Make a stack frame for local variables

	push 4
	push rsp
	sub	 ; Make RBP point on the last arg
	pop rbp	 ; Move RSP to RBP

	push 0	 ; Push the number to tmp through the stack
	pop [bx+0]

	push [bx+0]	 ; Push tmp to variable through the stack
	pop [cx+0]

	push 5	 ; Push the number to tmp through the stack
	pop [bx+1]

	push [bx+1]	 ; Push tmp to variable through the stack
	pop [cx+0]

	push [cx+0]	 ; Push variable to tmp through the stack
	pop [bx+2]

	push [bx+2]	 ; Push tmp 2 to argument 0 through the stack
	push 4
	push bx
	add
	pop bx	 ; Shift tmp counter
call func_1_1:	 ; Calling function
	push 4
	push bx
	sub
	pop bx	 ; Shift back tmp counter

	push ax	 ; Push return value to the stack
	pop [bx+3]	 ; Save return value to tmp var

	push 2
	push rsp
	sub
	pop rsp	 ; Shift RSP to skip arguments
	push [bx+3]	 ; Push tmp to variable through the stack
	pop [cx+0]

	push [cx+0]	 ; Push variable to tmp through the stack
	pop [bx+4]

	push [bx+4]	 ; Push tmp 4 to argument 0 through the stack
out	 ; System call
	push 1	 ; Push the number to tmp through the stack
	pop [bx+6]


	push [bx+6]
	pop ax	 ; Save return value to the rax
	pop rbp	 ; Get RBP value back
	pop bx	 ; Get tmp register value back
	pop cx	 ; Get var register value back
ret

func_1_1:	 ; Function body
	push cx	 ; Save variables counter register value
	push bx	 ; Save tmp variables counter register value
	push rbp	 ; Save RBP

	push bx
	pop cx	 ; Move tmp var counter reg value to the var counter reg

	push bx
	push 1
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

func_1_1_label0:	 ; Label in function
	push [cx+0]	 ; Push variable to tmp through the stack
	pop [bx+0]

	push 1	 ; Push the number to tmp through the stack
	pop [bx+1]

	push [bx+1]	 ; Push second operand
	push [bx+0]	 ; Push first operand
	eq
	pop [bx+2]	 ; Move tmp var counter reg value to the var counter reg

	push [bx+2]	 ; Push the result of the condition to the stack
	push 1	 ; Push true

je func_1_1_label1:	 ; Jump on label if the condition is true
jmp func_1_1_label2:	 ; Jump on label
func_1_1_label1:	 ; Label in function
	push [cx+0]	 ; Push variable to tmp through the stack
	pop [bx+3]


	push [bx+3]
	pop ax	 ; Save return value to the rax
	pop rbp	 ; Get RBP value back
	pop bx	 ; Get tmp register value back
	pop cx	 ; Get var register value back
ret
jmp func_1_1_label3:	 ; Jump on label
func_1_1_label2:	 ; Label in function
	push [cx+0]	 ; Push variable to tmp through the stack
	pop [bx+4]

	push [cx+0]	 ; Push variable to tmp through the stack
	pop [bx+5]

	push 1	 ; Push the number to tmp through the stack
	pop [bx+6]

	push [bx+6]	 ; Push second operand
	push [bx+5]	 ; Push first operand
	sub
	pop [bx+7]	 ; Move tmp var counter reg value to the var counter reg

	push [bx+7]	 ; Push tmp 7 to argument 0 through the stack
	push 9
	push bx
	add
	pop bx	 ; Shift tmp counter
call func_1_1:	 ; Calling function
	push 9
	push bx
	sub
	pop bx	 ; Shift back tmp counter

	push ax	 ; Push return value to the stack
	pop [bx+8]	 ; Save return value to tmp var

	push 2
	push rsp
	sub
	pop rsp	 ; Shift RSP to skip arguments
	push [bx+8]	 ; Push second operand
	push [bx+4]	 ; Push first operand
	mul
	pop [bx+9]	 ; Move tmp var counter reg value to the var counter reg


	push [bx+9]
	pop ax	 ; Save return value to the rax
	pop rbp	 ; Get RBP value back
	pop bx	 ; Get tmp register value back
	pop cx	 ; Get var register value back
ret
func_1_1_label3:	 ; Label in function
