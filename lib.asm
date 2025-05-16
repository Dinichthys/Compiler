section .data

EXIT_FUNC  equ 60
WRITE_FUNC equ 0x1
READ_FUNC  equ 0x0
STDOUT     equ 0x1
STDIN      equ 0x0
BUFFER_LEN equ 0xFF

TEN equ 0x41200000

QWORD_LEN  equ 8
DWORD_LEN  equ 4

FLOAT_SIZE equ 32
DOUBLE_EXP equ 11
FLOAT_EXP  equ 8

Buffer db 0xFF dup(0)
Zero_after_buf db 0

Float_nums dd 0x0, 0x3f800000, 0x40000000, 0x40400000, 0x40800000, 0x40A00000, 0x40C00000, 0x40E00000, 0x41000000, 0x41100000

Error_msg db 'You are trying to run library', 10
Error_msg_len db $ - Error_msg

Format_str db 'Out = %f', 10, 0

StartRam dq 0

section .text

global hlt_syscall
global in_syscall
global out_syscall
global StartRam

extern MyPrintf

start:
    mov rax, WRITE_FUNC
    mov rdi, STDOUT         ; Make parameters of syscall
    mov rsi, Error_msg
    mov rdx, Error_msg_len
    syscall

hlt_syscall:
    push rax
    push rbx
    push rcx
    push rdi
    push rsi

    mov rax, EXIT_FUNC
    mov rdi, qword [rsp+6*QWORD_LEN]
    syscall

    pop rsi
    pop rdi
    pop rcx
    pop rbx
    pop rax
    ret

in_syscall:
    push rbx
    push rcx
    push rdx
    push rdi
    push rsi

    mov rax, READ_FUNC
    mov rdi, STDIN
    mov rsi, Buffer
    mov rdx, BUFFER_LEN
    syscall

    call ReadBuffInSyscall

    pop rsi
    pop rdi
    pop rdx
    pop rcx
    pop rbx
    ret

ReadBuffInSyscall:
    mov rcx, Buffer
    xorps xmm0, xmm0
    xorps xmm1, xmm1
    mov rax, TEN
    movq xmm2, rax
    movq xmm3, rax
    xor rax, rax

.Comparison_1:
    mov al, byte [rcx]
    cmp al, '0'
    jb .StopWhile_1

    cmp rcx, Buffer + BUFFER_LEN
    jae .StopWhile_1

.While_1:
    sub al, '0'

    mulss xmm0, xmm2        ; XMM0 = XMM0 * 10
    movss xmm1, dword [rax*DWORD_LEN+Float_nums]
    addss xmm0, xmm1        ; XMM0 = XMM0 * 10 + XMM1

    inc rcx
    jmp .Comparison_1
.StopWhile_1:

    cmp al, '.'
    jne .Exit
    inc rcx

.Comparison_2:
    mov al, byte [rcx]
    cmp al, '0'
    jb .StopWhile_2

    cmp rcx, Buffer + BUFFER_LEN
    jae .StopWhile_2

.While_2:
    sub al, '0'

    movss xmm1, dword [rax*DWORD_LEN+Float_nums]
    divss xmm1, xmm2        ; XMM1 = XMM1 / 10
    addss xmm0, xmm1        ; XMM0 = XMM0 + XMM1 / 10
    mulss xmm2, xmm3

    inc rcx
    jmp .Comparison_2
.StopWhile_2:

.Exit:
    movq rax, xmm0
    ret

out_syscall:
    pop rax
    pop rdx
    push rax
    push rbx
    push rcx
    push rdi
    push rsi

    mov rdi, rdx
    shr rdx, FLOAT_SIZE - 2
    shl rdx, FLOAT_SIZE * 2 - 2

    shl rdi, FLOAT_SIZE + 2
    shr rdi, 2 + DOUBLE_EXP - FLOAT_EXP

    or rdx, rdi
    movq xmm0, rdx

    mov rdi, STDOUT
    mov rsi, Format_str

    call MyPrintf

    pop rsi
    pop rdi
    pop rcx
    pop rbx
    xor rax, rax
    ret
