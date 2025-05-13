	push 0
	pop cx	 ; Set zero to global variables counter register
	push 1
	pop bx	 ; Set value to tmp variables counter register

	push 3	 ; Push the number to tmp through the stack
	pop [bx+2]

