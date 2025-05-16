section .data

WRITE_FUNC      equ 0x1
PRINT_NUMBER    equ 0x1
STDOUT          equ 0x1

BUFFER_LEN equ 0xFF

NUMBER_ARGS_IN_REGS equ 6

END_SYMBOL                 equ 0x0
ARG_SYMBOL                 equ '%'
COLOR_SYMBOL               equ '#'
COLOR_JUMP_TABLE_FIRST_SYM equ 'b'
ESC_SYM                    equ 27               ; \e

BLACK_ESC_SEQUENCES        db '[30m'
RED_ESC_SEQUENCES          db '[31m'
GREEN_ESC_SEQUENCES        db '[32m'
YELLOW_ESC_SEQUENCES       db '[33m'
WHITE_ESC_SEQUENCES        db '[37m'

; ESC | COLOR
;-------------
; 30	Black
; 31	Red
; 32	Green
; 33	Yellow
; 37	White

%macro CASE_COLOR_PRINTED 1
.Color%1:
    cmp rcx, BUFFER_LEN - 5                       ; 5 = \e[30m
    jae .PrintBuffColor%1

.ContinueColor%1:
    mov byte [Buffer + rcx], ESC_SYM
    inc rcx
    mov eax, dword [%1_ESC_SEQUENCES]
    mov dword [Buffer + rcx], eax
    add rcx, 4
    jmp .Conditional

.PrintBuffColor%1:
    call PrintBuffer
    jmp .ContinueColor%1

%endmacro

HTML_START db '<html>', 0x0, 0x0
HTML_START_LEN equ 6

HTML_END db '</html>', 0x0
HTML_END_LEN equ 7

BLACK_HTML  db '<font color="#000000">', 0x0, 0x0
GREEN_HTML  db '<font color="#10FF10">', 0x0, 0x0
RED_HTML    db '<font color="#FF1010">', 0x0, 0x0
YELLOW_HTML db '<font color="#CCCC10">', 0x0, 0x0
WHITE_HTML  db '<font color="#E0E0E0">', 0x0, 0x0

HTML_FONT_COLOR_LEN equ 22

END_COLOR_HTML db '</font>', 0x0, 0x0
END_COLOR_HTML_LEN equ 7

%macro HTML_CASE_COLOR_PRINTED 1
.HTMLColor%1:
    cmp rcx, BUFFER_LEN - HTML_FONT_COLOR_LEN - END_COLOR_HTML_LEN
    jae .PrintBuffHTMLColor%1

.ContinueHTMLColor%1:
    mov rax, qword [END_COLOR_HTML]
    mov qword [Buffer + rcx], rax
    add rcx, END_COLOR_HTML_LEN

    mov rax, qword [%1_HTML]
    mov qword [Buffer + rcx], rax
    add rcx, 8

    mov rax, qword [%1_HTML + 8]
    mov qword [Buffer + rcx], rax
    add rcx, 8

    mov rax, qword [%1_HTML + 16]
    mov qword [Buffer + rcx], rax
    add rcx, 6                                          ; HTML_FONT_COLOR_LEN = 22 = 8 + 8 + 6
    jmp .Conditional

.PrintBuffHTMLColor%1:
    call PrintBuffer
    jmp .ContinueHTMLColor%1

%endmacro

JUMP_TABLE_FIRST_SYM equ 'b'
%define JUMP_TABLE_LEN_FROM_F_TO_N  'n'-'f' - 1
%define JUMP_TABLE_LEN_FROM_N_TO_O  'o'-'n' - 1
%define JUMP_TABLE_LEN_FROM_O_TO_S  's'-'o' - 1
%define JUMP_TABLE_LEN_FROM_S_TO_X  'x'-'s' - 1

DONE_RESULT       equ 0x0
INVALID_SPECIFIER equ 0x1
SYSCALL_ERROR     equ 0x2
INVALID_COLOR     equ 0x3

ADDRESS_LEN_POW_2 equ 0x3
STACK_ELEM_SIZE   equ 0x8
QWORD_LEN         equ 0x3

SIGN_MASK dq 0xA000000000000000                            ; First bit is 1, other are 0
MINUS     equ '-'

REGISTER_SIZE equ 64
INT_SIZE      equ 32
FLOAT_SIZE    equ 32
DOUBLE_SIZE   equ 64

%define CURRENT_ARGUMENT        qword [r8]
%define INCREASE_ARGUMENT_INDEX add r8, STACK_ELEM_SIZE
%define ARGUMENT_INDEX          r8

%define CURRENT_FLOAT_ARGUMENT         [r9]
%define INCREASE_FLOAT_ARGUMENT_INDEX  add r9, STACK_ELEM_SIZE
%define FLOAT_ARGUMENT_INDEX           r9

%macro START_INDEXING_FLOAT_ARGUMENTS 0
    mov r9, (NUMBER_ARGS_IN_REGS + 2) * STACK_ELEM_SIZE
    add r9, rbp
%endmacro

FLAG_START_NUMBER equ 0x1
FLAG_END_NUMBER   equ 0x0

FIRST_DEGREE  equ 0x1
THIRD_DEGREE  equ 0x3
FOURTH_DEGREE equ 0x4

TEN  equ 0xA
FIVE equ 0x5

EAX_PATTERN equ 0x00000000FFFFFFFF

MANTISSA_LEN equ 52
MANTISSA_ONE dq 0x0010000000000000

EXPONENT equ 0b10000000000

FLAG_DEF_RCX equ 0
FLAG_NEG_RCX equ 1

FLAG_PLUS  equ 0
FLAG_MINUS equ 1

FLAG_DEF_VAL  equ 0
FLAG_SPEC_VAL equ 1                                                             ; Flags of spec or default value of float number

%define LOC_VAR_NUM_PRINTED qword [rbp - STACK_ELEM_SIZE]
%define LOC_VAR_FLOAT_GOT   qword [rbp - STACK_ELEM_SIZE * 2]
%define INCREASE_FLOAT_COUNTER add qword [rbp - STACK_ELEM_SIZE * 2], 8         ; 8 = SizeOf (label)
%define LOC_FILE_OUT        qword [rbp - STACK_ELEM_SIZE * 3]

%macro ARG_F_GOT_FLOAT_FROM_REG 1
    INCREASE_FLOAT_COUNTER
    movq rax, %1
    call PrintArgF
    jmp .Conditional
%endmacro

%macro GOT_ARGUMENT_TO_RAX 0
    mov rax, CURRENT_ARGUMENT
    INCREASE_ARGUMENT_INDEX
%endmacro

INF db 'inf'
NAN db 'nan'

NAN_EXPONENTA dq 0x7FF0000000000000

%macro SYNCHRONIZATION_ARG_INDEXES 0
    mov rax, ARGUMENT_INDEX
    sub rax, (NUMBER_ARGS_IN_REGS + 2) * STACK_ELEM_SIZE
    cmp rax, rbp
    je .SynchronizationFromAvgToFloat
    ja .SynchronizationFromFloatToAvg
%endmacro

Alphabet:
    db '0123456789ABCDEF'

Buffer:
    db BUFFER_LEN dup (0)

SaveZoneAfterBuffer:
    db 0xFF dup (0)
;--------------------------------------------


;--------------------------------------------
section .text

global MyPrintf

;--------------------------------------------
; Function pushes all parameters to stack
; and save pointer on string in rdi
;
; Entry: RDI, RSI, RDX, RCX, R8, R9, STACK
; Exit:  Stdout
; Dest:  RAX
;--------------------------------------------

MyPrintf:
    pop rax                         ; Save returning address

    push r9                         ; Push parameters
    push r8
    push rcx
    push rdx
    push rsi
    push rdi

    push rax

    push rbp
    mov rbp, rsp                    ; Make the stack frame
;// TODO
    sub rsp, 3 * STACK_ELEM_SIZE
    mov LOC_VAR_NUM_PRINTED, 0x0    ; Local variable with number of printed symbols
    mov LOC_VAR_FLOAT_GOT, 0x0
    mov LOC_FILE_OUT, STDOUT

    mov r8, rbp
    add r8, STACK_ELEM_SIZE * 2     ; R8 - pointer of the argument
    mov rsi, qword [r8]
    mov LOC_FILE_OUT, rsi
    add r8, STACK_ELEM_SIZE
    mov rsi, qword [r8]             ; Move pointer of string to RSI
    add r8, STACK_ELEM_SIZE

    jmp MyPrintfReal                ; Start real Printf

ExitFunction:
    add rsp, STACK_ELEM_SIZE * 3
    mov rsp, rbp

    pop rbp
    pop rdi

    add rsp, STACK_ELEM_SIZE * 6
    push rdi

    ret

;--------------------------------------------

;--------------------------------------------
; Function print the string with parameters
; pointed by R8
;
; Entry: RSI, R8
; Exit:  Stdout, RAX
; Dest:  RCX, RDX, RSI, RDI, R8, R9, R11
;--------------------------------------------

MyPrintfReal:
    xor rcx, rcx

    cmp LOC_FILE_OUT, STDOUT
    jne .StartHtml

.ReturnStartHtml:
    START_INDEXING_FLOAT_ARGUMENTS

;---------------------------------

.Conditional:
    cmp byte [rsi], END_SYMBOL
    je .Done                                    ; Check if there is the end of the string

.While:
    cmp byte [rsi], ARG_SYMBOL
    je .PrintArgument                           ; Check if an argument is needed

    cmp byte [rsi], COLOR_SYMBOL
    je .PrintColor
.PrintChar:
    cmp rcx, BUFFER_LEN
    je .BufferEnd

.Continue:
    mov al, byte [rsi]
    mov byte [Buffer + rcx], al
    inc rcx

    inc rsi                                     ; RSI is the pointer of the next symbol of the string
    jmp .Conditional

;---------------------------------

.Done:
    jmp .LastPrintBuffer

.MovDoneResult:
    mov rax, DONE_RESULT

.StopPrint:
    jmp ExitFunction

;---------------------------------

.StartHtml:
    mov rax, qword [HTML_START]
    mov qword [Buffer + rcx], rax
    add rcx, HTML_START_LEN

    mov rax, qword [BLACK_HTML]
    mov qword [Buffer + rcx], rax
    add rcx, 8

    mov rax, qword [BLACK_HTML + 8]
    mov qword [Buffer + rcx], rax
    add rcx, 8

    mov rax, qword [BLACK_HTML + 16]
    mov qword [Buffer + rcx], rax                       ; Buffer = "<html> <font color="#101010">
    add rcx, 6                                          ; HTML_FONT_COLOR_LEN = 22 = 8 + 8 + 6

    jmp .ReturnStartHtml

;---------------------------------

.LastPrintBuffer:
    cmp LOC_FILE_OUT, STDOUT
    jne .PrintLastColorFile

    cmp rcx, BUFFER_LEN - 5                       ; 5 = \e[30m
    jae .PreLastPrintBuff

.ContinueLastPrintBuff:
    mov byte [Buffer + rcx], ESC_SYM
    inc rcx
    mov eax, dword [BLACK_ESC_SEQUENCES]
    mov dword [Buffer + rcx], eax
    add rcx, 4
    push rsi
    call PrintBuffer
    pop rsi
    jmp .MovDoneResult

.PreLastPrintBuff:
    call PrintBuffer
    jmp .ContinueLastPrintBuff

;-----------------------

.PrintLastColorFile:
    cmp rcx, BUFFER_LEN - HTML_END_LEN - HTML_FONT_COLOR_LEN
    jae .PreLastPrintBuffFile

.ContinueLastPrintBuffFile:
    mov rax, qword [END_COLOR_HTML]
    mov qword [Buffer + rcx], rax
    add rcx, END_COLOR_HTML_LEN

    mov rax, qword [HTML_END]
    mov qword [Buffer + rcx], rax
    add rcx, HTML_END_LEN
    call PrintBuffer
    jmp .MovDoneResult

.PreLastPrintBuffFile:
    call PrintBuffer
    jmp .ContinueLastPrintBuffFile

;---------------------------------

.BufferEnd:
    call PrintBuffer
    jmp .Continue

;---------------------------------

.PrintArgument:
    inc rsi

    cmp byte [rsi], ARG_SYMBOL
    je .PrintChar
    xor rax, rax
    mov al, byte [rsi]
    inc rsi
    sub rax, JUMP_TABLE_FIRST_SYM               ; RAX is the number of string in JumpTable

    shl rax, QWORD_LEN

    add rax, .JumpTable
    mov rax, [rax]
    jmp rax

;---------------------------------

.PrintColor:
    cmp LOC_FILE_OUT, STDOUT
    jne .FilePrintColor

    inc rsi
    xor rax, rax
    mov al, byte [rsi]
    inc rsi
    sub rax, COLOR_JUMP_TABLE_FIRST_SYM

    shl rax, QWORD_LEN

    add rax, .ColorJumpTable
    mov rax, [rax]
    jmp rax

.FilePrintColor:
    inc rsi
    xor rax, rax
    mov al, byte [rsi]
    inc rsi
    sub rax, COLOR_JUMP_TABLE_FIRST_SYM

    shl rax, QWORD_LEN

    add rax, .ColorJumpTableHTML
    mov rax, [rax]
    jmp rax

;---------------------

    CASE_COLOR_PRINTED BLACK

;---------------------

    CASE_COLOR_PRINTED GREEN

;---------------------

    CASE_COLOR_PRINTED RED

;---------------------

    CASE_COLOR_PRINTED WHITE

;---------------------

    CASE_COLOR_PRINTED YELLOW

;---------------------

.ColorJumpTable:
    dq .ColorBLACK
    dq 'g'-'b'-1 dup (.InvalidColor)
    dq .ColorGREEN
    dq 'r'-'g'-1 dup (.InvalidColor)
    dq .ColorRED
    dq 'w'-'r'-1 dup (.InvalidColor)
    dq .ColorWHITE
    dq 'y'-'w'-1 dup (.InvalidColor)
    dq .ColorYELLOW

;---------------------

    HTML_CASE_COLOR_PRINTED BLACK

;---------------------

    HTML_CASE_COLOR_PRINTED GREEN

;---------------------

    HTML_CASE_COLOR_PRINTED RED

;---------------------

    HTML_CASE_COLOR_PRINTED WHITE

;---------------------

    HTML_CASE_COLOR_PRINTED YELLOW

;---------------------

.ColorJumpTableHTML:
    dq .HTMLColorBLACK
    dq 'g'-'b'-1 dup (.InvalidColor)
    dq .HTMLColorGREEN
    dq 'r'-'g'-1 dup (.InvalidColor)
    dq .HTMLColorRED
    dq 'w'-'r'-1 dup (.InvalidColor)
    dq .HTMLColorWHITE
    dq 'y'-'w'-1 dup (.InvalidColor)
    dq .HTMLColorYELLOW

.InvalidColor:
    mov rax, INVALID_COLOR
    jmp .StopPrint

;---------------------------------

.InvalidArgument:

    mov rax, INVALID_SPECIFIER
    jmp .StopPrint

;-----------------

.ArgB:

    GOT_ARGUMENT_TO_RAX

    push rsi
    mov rsi, FIRST_DEGREE                   ; RSI - the number of power of 2 in counting system
    call ValToStrPowTwo
    pop rsi

    SYNCHRONIZATION_ARG_INDEXES

    jmp .Conditional

;-----------------

.ArgC:

    GOT_ARGUMENT_TO_RAX

    call PrintArgC

    SYNCHRONIZATION_ARG_INDEXES

    jmp .Conditional

;-----------------

.ArgD:

    GOT_ARGUMENT_TO_RAX

    call PrintArgD

    SYNCHRONIZATION_ARG_INDEXES

    jmp .Conditional

;-----------------

.ArgF:

    mov rax, LOC_VAR_FLOAT_GOT
    add rax, .JumpTableFloat
    mov rax, [rax]
    jmp rax

;--------

.FirstFloat:
    ARG_F_GOT_FLOAT_FROM_REG xmm0

;--------

.SecondFloat:
    ARG_F_GOT_FLOAT_FROM_REG xmm1

;--------

.ThirdFloat:
    ARG_F_GOT_FLOAT_FROM_REG xmm2

;--------

.FourthFloat:
    ARG_F_GOT_FLOAT_FROM_REG xmm3

;--------

.FifthFloat:
    ARG_F_GOT_FLOAT_FROM_REG xmm4

;--------

.SixthFloat:
    ARG_F_GOT_FLOAT_FROM_REG xmm5

;--------

.SeventhFloat:
    ARG_F_GOT_FLOAT_FROM_REG xmm6

;--------

.EighthFloat:
    ARG_F_GOT_FLOAT_FROM_REG xmm7

;--------

.FloatInStack:

    movsd xmm4, CURRENT_FLOAT_ARGUMENT
    movq rax, xmm4                              ; XMM4 - could be changed according to calling convention
    INCREASE_FLOAT_ARGUMENT_INDEX

    call PrintArgF

    mov rax, ARGUMENT_INDEX
    sub rax, (NUMBER_ARGS_IN_REGS + 2) * STACK_ELEM_SIZE
    cmp rax, rbp

    jae .SynchronizationFromAvgToFloat
    jmp .Conditional

;--------

.JumpTableFloat:
    dq .FirstFloat
    dq .SecondFloat
    dq .ThirdFloat
    dq .FourthFloat
    dq .FifthFloat
    dq .SixthFloat
    dq .SeventhFloat
    dq .EighthFloat
    dq .FloatInStack

;-----------------

.ArgN:

    GOT_ARGUMENT_TO_RAX

    mov rdx, LOC_VAR_NUM_PRINTED
    add rdx, rcx
    mov qword [rax], rdx

    SYNCHRONIZATION_ARG_INDEXES

    jmp .Conditional

;-----------------

.ArgO:

    GOT_ARGUMENT_TO_RAX

    call ValToStrOct

    SYNCHRONIZATION_ARG_INDEXES

    jmp .Conditional

;-----------------

.ArgS:

    push rsi
    mov rsi, CURRENT_ARGUMENT
    INCREASE_ARGUMENT_INDEX
    call PrintArgString
    pop rsi

    SYNCHRONIZATION_ARG_INDEXES

    jmp .Conditional

;-----------------

.ArgX:

    GOT_ARGUMENT_TO_RAX

    push rsi
    mov rsi, FOURTH_DEGREE                  ; RSI - the number of power of 2 in counting system
    call ValToStrPowTwo
    pop rsi

    SYNCHRONIZATION_ARG_INDEXES

    jmp .Conditional

;---------------------------------

;// TODO Подставить вместо макросов то, во что они раскрываются

.JumpTable:
    dq .ArgB
    dq .ArgC
    dq .ArgD
    dq .InvalidArgument                                     ; %e
    dq .ArgF
    dq 'n'-'f'-1 dup (.InvalidArgument)
    dq .ArgN
    dq 'o'-'n'-1 dup (.InvalidArgument)
    dq .ArgO
    dq 's'-'o'-1 dup (.InvalidArgument)
    dq .ArgS
    dq 'x'-'s'-1 dup (.InvalidArgument)
    dq .ArgX

;---------------------------------

.SynchronizationFromAvgToFloat:
    mov ARGUMENT_INDEX, FLOAT_ARGUMENT_INDEX
    jmp .Conditional

;---------------------------------

;---------------------------------

.SynchronizationFromFloatToAvg:
    mov FLOAT_ARGUMENT_INDEX,  ARGUMENT_INDEX
    jmp .Conditional

;---------------------------------

;---------------------------------
; Prints buffer to stdout and
; mov null to rcx
;
; Entry:  Buffer, RCX
; Exit:   Stdout, RCX
; Destrs: RAX, RDI, RSI, RDX, R11
;---------------------------------

PrintBuffer:
    push rsi

    mov rax, WRITE_FUNC
    mov rdi, LOC_FILE_OUT         ; Make parameters of syscall
    mov rsi, Buffer
    mov rdx, rcx
    add LOC_VAR_NUM_PRINTED, rcx
    syscall

    pop rsi

    cmp rax, rdx
    jne .Error

    xor rcx, rcx

    ret

.Error:
    mov rax, SYSCALL_ERROR
    jmp ExitFunction

;---------------------------------


;---------------------------------
; It moves argument char to buffer
;
; Entry:  RAX, RCX
; Exit:   Stdout, RCX, Buffer
; Destrs: RAX, RBX, RCX, RDX
;---------------------------------

PrintArgC:

    cmp rcx, BUFFER_LEN
    je .BufferEnd

.Continue:
    mov byte [Buffer + rcx], al
    inc rcx
    ret

.BufferEnd:
    call PrintBuffer
    jmp .Continue

;---------------------------------


;---------------------------------
; It translates RAX values
; to the string
;
; Entry:  RAX
; Exit:   Buffer
; Destrs: RAX, RDX
;---------------------------------

PrintArgD:

    push rax
    push rbx
    push rdi
    push rsi

    push rcx
    mov rcx, REGISTER_SIZE
    shr rcx, 1                               ; CL = REGISTER_SIZE / 4
    shl rax, cl
    shr rax, cl                              ; Prepare DX and AX for dividing
    pop rcx

    mov rbx, INT_SIZE
    call CheckSign
    mov ebx, TEN

    xor rdx, rdx

    xor rsi, rsi                             ; RSI - Counter of digits in the number

    mov rdi, FLAG_START_NUMBER

.Conditional_1:
    test rax, rax
    je .StopWhile_1

.While_1:
    div ebx
    push rdx
    xor rdx, rdx
    inc rsi
    jmp .Conditional_1

.StopWhile_1:

.Conditional_2:
    test rsi, rsi
    je .StopWhile_2

.While_2:
    pop rax
    call DigitToStr
    dec rsi
    jmp .Conditional_2

.StopWhile_2:

    pop rsi
    pop rdi
    pop rbx
    pop rax

    ret

;---------------------------------


;---------------------------------
; It translates RAX values
; to the string
;
; Entry:  RAX
; Exit:   Buffer
; Destrs: RAX
;---------------------------------

PrintArgF:

    call CheckSpecialFloatMeaning
    cmp r11, FLAG_SPEC_VAL
    je .Skip

    push rax
    push rbx
    push rdx

    mov rbx, DOUBLE_SIZE
    call CheckSign
    push rcx

    cmp rdx, FLAG_MINUS
    je .NegativeNum

    mov rdx, rax
    shr rdx, MANTISSA_LEN
    mov rbx, EXPONENT
    inc rdx
    sub rdx, rbx                             ; DX - Exponent

.ContinueNegativeNum:
    mov rbx, rax
    shl rbx, DOUBLE_SIZE - MANTISSA_LEN
    shr rbx, DOUBLE_SIZE - MANTISSA_LEN
    add rbx, qword [MANTISSA_ONE]                    ; RBX - Mantissa

    tzcnt rcx, rbx                                   ; RCX = Number of trailing zeroes in RBX
    shr rbx, cl

    push rbx
    mov rbx, rcx
    mov rcx, MANTISSA_LEN
    sub rcx, rbx
    sub rcx, rdx
    pop rbx

                                                ; Printed number = RBX * 2 ^ (-RCX)
                                                ; Printed number = RBX * 5 ^ (RCX) * 10 ^ (-RCX)
    cmp rcx, [SIGN_MASK]
    ja .NegRCX
    je .ZeroRCX
    cmp rcx, 0x0
    je .ZeroRCX

    push rax
    push rdx

    lzcnt rdx, rbx                          ; RDX = Number of leading zeroes in RBX
    push rcx                                ; Push old RCX
    shl rcx, 1                              ; RCX = old RCX * 2
    pop rax                                 ; RAX = old RCX
    add rcx, rax                            ; RCX = old RCX * 3
.ConditionalRoundResult:
    cmp rdx, rcx                            ; What is greater?
                                            ; Number of leading zeroes in RBX or 3 * RCX
    jae .StopRounding

    sub rcx, rdx
    shr rcx, 2                              ; RCX - RDX = (old RCX - new RCX) / 3 + (new RDX - old RDX)
    inc rcx
    shr rbx, cl
    sub rax, rcx

.StopRounding:
    mov rcx, rax

    push rcx
.For:
    imul rbx, FIVE                          ; 5 = 10 / 2
    loop .For
    pop rcx

    pop rdx
    pop rax

    mov rax, rbx
    mov rbx, rcx
    pop rcx
    call PrintNumber

.Done:
    pop rdx
    pop rbx
    pop rax

.Skip:
    ret

.NegRCX:
    neg rcx
    shl rbx, cl
    mov rax, rbx
    pop rcx
    call PrintArgD
    jmp .Done

.ZeroRCX:
    mov rax, rbx
    pop rcx
    call PrintArgD
    jmp .Done

.NegativeNum:
    neg rax                                  ; In function of checking sign RAX was negative
    shl rax, 1
    shr rax, 1
    mov rdx, rax
    shr rdx, MANTISSA_LEN
    mov rbx, EXPONENT
    inc rdx
    sub rdx, rbx                             ; DX - Exponent
    jmp .ContinueNegativeNum

;---------------------------------


;---------------------------------
; It checks RAX is INF or NAN
; If it is NAN or INF this func will
; put a flag to R11
;
; Entry:  RAX
; Exit:   Buffer
; Destrs: RAX, R11
;---------------------------------

CheckSpecialFloatMeaning:

    push rax

    mov r11, FLAG_DEF_VAL
    and rax, qword [NAN_EXPONENTA]
    cmp rax, qword [NAN_EXPONENTA]
    je .FullExp

.Done:
    pop rax
    ret

.FullExp:
    pop rax
    push rax
    shl rax, REGISTER_SIZE - MANTISSA_LEN
    cmp rax, 0x0
    je .Infinite

    pop rax
    push rax
    shr rax, REGISTER_SIZE - 1
    cmp rax, 1
    je .MinusNAN

    cmp rcx, BUFFER_LEN - 3
    jae .BufferEndPlusNAN

.ContinuePlusNAN:
    mov ax, word [NAN]
    mov word [Buffer + rcx], ax                 ; [Buffer + RCX] = 'na'
    mov al, byte [NAN + 2]
    mov byte [Buffer + 2 + rcx], al             ; [Buffer + RCX] = 'nan'
    add rcx, 3

    mov r11, FLAG_SPEC_VAL
    jmp .Done

.BufferEndPlusNAN:
    call PrintBuffer
    jmp .ContinuePlusNAN

.MinusNAN:
    cmp rcx, BUFFER_LEN - 4
    jae .BufferEndMinusNAN

.ContinueMinusNAN:
    mov byte [Buffer + rcx], MINUS              ; [Buffer + RCX] = '-'
    mov ax, word [NAN]
    mov word [Buffer + 1 + rcx], ax             ; [Buffer + RCX] = '-na'
    mov al, byte [NAN + 2]
    mov byte [Buffer + 3 + rcx], al             ; [Buffer + RCX] = '-nan'
    add rcx, 4

    mov r11, FLAG_SPEC_VAL
    jmp .Done

.BufferEndMinusNAN:
    call PrintBuffer
    jmp .ContinueMinusNAN

.Infinite:
    pop rax
    push rax
    shr rax, REGISTER_SIZE - 1
    cmp rax, 1
    je .MinusINF

    cmp rcx, BUFFER_LEN - 3
    jae .BufferEndPlusINF

.ContinuePlusINF:
    mov ax, word [INF]
    mov word [Buffer + rcx], ax                 ; [Buffer + RCX] = 'in'
    mov al, byte [INF + 2]
    mov byte [Buffer + 2 + rcx], al             ; [Buffer + RCX] = 'inf'
    add rcx, 3

    mov r11, FLAG_SPEC_VAL
    jmp .Done

.BufferEndPlusINF:
    call PrintBuffer
    jmp .ContinuePlusINF

.MinusINF:
    cmp rcx, BUFFER_LEN - 4
    jae .BufferEndMinusINF

.ContinueMinusINF:
    mov byte [Buffer + rcx], MINUS              ; [Buffer + RCX] = '-'
    mov ax, word [INF]
    mov word [Buffer + 1 + rcx], ax             ; [Buffer + RCX] = '-in'
    mov al, byte [INF + 2]
    mov byte [Buffer + 3 + rcx], al             ; [Buffer + RCX] = '-inf'
    add rcx, 4

    mov r11, FLAG_SPEC_VAL
    jmp .Done

.BufferEndMinusINF:
    call PrintBuffer
    jmp .ContinueMinusINF

;---------------------------------


;---------------------------------
; It prints number RAX * 10 ^ (-RBX)
;
; Entry:  RAX, RBX, RCX
; Exit:   Buffer
; Destrs: RAX
;---------------------------------

PrintNumber:

    push rax
    push rdx
    push rbx
    push rdi
    push rsi

    mov rdi, rbx

    mov rbx, TEN

    xor rdx, rdx

    xor rsi, rsi                             ; RSI - Counter of digits in the number


.Conditional_1:
    test rax, rax
    je .StopWhile_1

.While_1:
    div rbx
    push rdx
    xor rdx, rdx
    inc rsi
    jmp .Conditional_1

.StopWhile_1:

    mov rbx, rdi
    cmp rbx, rsi
    jae .ZeroStarted                            ; RBX more than length of RAX so number will looks like '0.etc'
    jmp .RSImoreRBX

.ContinueFunc:
    mov rdi, FLAG_END_NUMBER

.Conditional_2:
    test rsi, rsi
    je .StopWhile_2

.While_2:
    pop rax
    call DigitToStr
    dec rsi
    jmp .Conditional_2

.StopWhile_2:

    pop rsi
    pop rdi
    pop rbx
    pop rdx
    pop rax

    ret

.ZeroStarted:
; It prints '0.' and then prints zeroes until RBX = RSI. Then it will print the hole number

    cmp rcx, BUFFER_LEN - 1
    jae .BufferEndZeroStarted

.ContinueZeroStarted:
    mov word [Buffer + rcx], '0.'               ; The number started with '0.'
    add rcx, 2

.Conditional_ZeroStarted:
    cmp rsi, rbx
    je .StopWhile_ZeroStarted

.While_ZeroStarted:
    xor rax, rax
    call DigitToStr
    dec rbx
    jmp .Conditional_ZeroStarted

.StopWhile_ZeroStarted:

    jmp .ContinueFunc

.BufferEndZeroStarted:
    push rsi
    call PrintBuffer
    pop rsi
    jmp .ContinueZeroStarted

.RSImoreRBX:
; Print numbers of RAX until RBX = RSI. Then put '.' and print other numbers

.Conditional_RBX:
    cmp rsi, rbx
    je .StopWhile_RBX

.While_RBX:
    pop rax
    call DigitToStr
    dec rsi
    jmp .Conditional_RBX

.StopWhile_RBX:

    cmp rcx, BUFFER_LEN
    je .BufferEndRSImoreRBX

.ContinueRSImoreRBX:
    mov byte [Buffer + rcx], '.'
    inc rcx
    jmp .ContinueFunc

.BufferEndRSImoreRBX:
    push rsi
    call PrintBuffer
    pop rsi
    jmp .ContinueRSImoreRBX

;---------------------------------


;---------------------------------
; It moves sign to the buffer
;
; Entry:  RAX, RBX
; Exit:   Buffer
; Destrs: RBX, RDX
;---------------------------------

CheckSign:

    push rax

    push rcx
    mov rcx, rbx
    sub rcx, 1
    shr rax, cl
    pop rcx

    cmp rax, 1
    je .Minus
    pop rax
    mov rdx, FLAG_PLUS

.Done:
    ret

.Minus:
    cmp rcx, BUFFER_LEN
    je .BufferEnd

.Continue:
    pop rax
    neg rax
    mov byte [Buffer + rcx],  MINUS
    inc rcx
    mov rdx, FLAG_MINUS
    jmp .Done

.BufferEnd:
    push rcx
    call PrintBuffer
    pop rcx
    jmp .Continue

;---------------------------------


;---------------------------------
; It translates RAX values
; to the string with value in a form of
; number system of 2 to the power RSI
;
; Entry:  RAX, RSI
; Exit:   Buffer
; Destrs: RAX
;---------------------------------

;// TODO Сделать параметр - маску для вывода одной цифры

ValToStrPowTwo:

    push rdi
    push rbx
    push rdx
    push rax

    xor rdx, rdx
    mov rbx, REGISTER_SIZE
    sub rbx, rsi
    mov rdi, FLAG_START_NUMBER

.Conditional:
    cmp rdx, REGISTER_SIZE
    jae .StopWhile

.While:
    push rcx
    mov rcx, rdx
    shl rax, cl
    mov rcx, rbx
    add rcx, rdx
    shr rax, cl
    pop rcx
    call DigitToStr
    pop rax
    push rax
    sub rbx, rsi
    add rdx, rsi
    jmp .Conditional

.StopWhile:

    pop rax
    pop rdx
    pop rbx
    pop rdi
    ret

;---------------------------------


;---------------------------------
; It translates RAX values
; to the string with value in a form of
; octal number system
;
; Entry:  RAX
; Exit:   Buffer
; Destrs: RAX, RDI
;---------------------------------

ValToStrOct:

    push rdi
    push rbx
    push rdx
    push rax

    xor rdx, rdx
    mov rbx, REGISTER_SIZE
    sub rbx, 1                                  ; 64 mod 3 = 1, where 64 bit - size of register, 3 - power of 2
    mov rdi, FLAG_START_NUMBER

.Conditional:
    cmp rdx, REGISTER_SIZE
    jae .StopWhile

.While:
    push rcx
    mov rcx, rdx
    shl rax, cl
    mov rcx, rbx
    add rcx, rdx
    shr rax, cl
    pop rcx
    call DigitToStr
    pop rax
    push rax
    sub rbx, THIRD_DEGREE
    test rdx, rdx
    je .FirstDigit
    add rdx, THIRD_DEGREE
    jmp .Conditional

.StopWhile:
    pop rax
    pop rdx
    pop rbx
    pop rdi
    ret

.FirstDigit:
    add rdx, 1                                  ; 64 mod 3 = 1, where 64 bit - size of register, 3 - power of 2
    jmp .Conditional

;---------------------------------


;---------------------------------
; It translates number from AL to
; stack
;
; Entry:  RAX, RDI
; Exit:   Buffer
; Destrs: RAX, RDI
;---------------------------------

DigitToStr:

    cmp rcx, BUFFER_LEN
    je .BufferEnd

.Continue:
    cmp rdi, FLAG_START_NUMBER
    je .SkipZeroes

.DontSkip:
    mov al, byte [Alphabet + rax]
    mov byte [Buffer + rcx], al
    inc rcx

.Skip:
    ret

.SkipZeroes:
    test rax, rax
    je .Skip
    mov rdi, FLAG_END_NUMBER
    jmp .DontSkip

.BufferEnd:
    push rax
    push rbx
    push rdx
    push rsi
    call PrintBuffer
    pop rsi
    pop rdx
    pop rbx
    pop rax
    jmp .Continue

;---------------------------------


;---------------------------------
; It prints string pointed by RSI
;
; Entry:  RSI
; Exit:   Stdout
; Destrs: RAX, RDX, RDI
;---------------------------------

;// TODO Если длина строки меньше длины буфера, то закидываем строку в буфер, иначе выводим отдельным сисколом

PrintArgString:

    push rcx
    xor rcx, rcx
;     push rsi
;
;     call PrintBuffer
;
;     pop rsi

.Conditional:
    cmp byte [rsi], END_SYMBOL
    je .ExitWhile

.While:
    inc rsi
    inc rcx
    jmp .Conditional

.ExitWhile:

    cmp rcx, BUFFER_LEN
    jbe .CopyToBuffer

    mov r10, rcx
    pop rcx
    push rsi
    call PrintBuffer
    mov rcx, r10
    pop rsi

    mov rax, WRITE_FUNC
    mov rdi, LOC_FILE_OUT         ; Make parameters of syscall
    sub rsi, rcx
    mov rdx, rcx
    add LOC_VAR_NUM_PRINTED, rcx
    syscall

    cmp rax, rdx
    jne .Error

    xor rcx, rcx
.Stop:
    ret

.Error:
    mov rax, SYSCALL_ERROR
    jmp ExitFunction

.CopyToBuffer:
    pop rdi
    mov rdx, BUFFER_LEN
    sub rdx, rdi
    sub rsi, rcx
    cmp rdx, rcx
    jb .PrintBuffer

    mov rcx, rdi

.CondCopyToBuff:
    cmp byte [rsi], END_SYMBOL
    je .Stop

.WhileCopyToBuff:
    mov al, byte [rsi]
    mov byte [Buffer + rcx], al
    inc rsi
    inc rcx
    jmp .CondCopyToBuff


.PrintBuffer:
    push rsi
    call PrintBuffer
    pop rsi
    jmp .CondCopyToBuff
;---------------------------------

;--------------------------------------------

