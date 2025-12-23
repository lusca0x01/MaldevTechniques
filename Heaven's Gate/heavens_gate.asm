section .data
made_in_heaven_msg db 0xD, 0xA, '!!!Made in Heaven!!!', 0xD, 0xA
made_in_heaven_msg_size equ $ - made_in_heaven_msg

prompt_msg db 'Enter the key: ', 0
prompt_msg_size equ $ - prompt_msg

success_msg db 0xD, 0xA, 'Correct! You found the key!', 0xD, 0xA
success_msg_size equ $ - success_msg

fail_msg db 0xD, 0xA, 'Wrong key. Try again!', 0xD, 0xA
fail_msg_size equ $ - fail_msg

section .bss
bytes_written  resq 1
bytes_read     resd 1
stdout_handle  resd 1
stdin_handle   resd 1
key_buffer    resb 64
key_result    resd 1

section .text
global _main
extern _GetStdHandle@4
extern _WriteConsoleA@20
extern _ReadConsoleA@20
extern _ExitProcess@4


[BITS 32]
_main:
    push -11
    call _GetStdHandle@4
    mov [stdout_handle], eax

    push -10
    call _GetStdHandle@4
    mov [stdin_handle], eax

    push prompt_msg_size
    push prompt_msg
    call print_string_32
    add esp, 8

    push 0
    push bytes_read
    push 63
    push key_buffer
    push dword [stdin_handle]
    call _ReadConsoleA@20

    call made_in_heaven

    mov eax, [key_result]
    test eax, eax
    jz .fail

.success:
    push success_msg_size
    push success_msg
    call print_string_32
    add esp, 8
    jmp .exit

.fail:
    push fail_msg_size
    push fail_msg
    call print_string_32
    add esp, 8

.exit:
    push 0
    call _ExitProcess@4

print_string_32:
    push ebp
    mov ebp, esp

    push 0
    push bytes_written
    push dword [ebp+12]
    push dword [ebp+8]
    push dword [stdout_handle]
    call _WriteConsoleA@20

    pop ebp
    ret


made_in_heaven:
    push ebp
    mov ebp, esp

    push made_in_heaven_msg_size
    push made_in_heaven_msg
    call print_string_32
    add esp, 8

    push ebx
    mov ebx, esp
    and esp, 0xFFFFFFF8

    push 0x33
    push heavens_gate
    retf

back_from_64:
    mov esp, ebx
    pop ebx
    mov esp, ebp
    pop ebp
    ret

[BITS 64]
heavens_gate:
    lea rsi, [rel key_buffer]
    xor rax, rax
    
    mov rbx, [rsi]
    mov rcx, 0x47246E3376613348
    cmp rbx, rcx
    jne .fail_64
    
    mov rbx, [rsi + 8]
    not rbx
    mov rcx, 0x85CCCE8AADCC8BCB
    cmp rbx, rcx
    jne .fail_64
    
    movzx rbx, word [rsi + 16]
    xor rbx, 0xDEAD
    cmp rbx, 0xD4A0
    jne .fail_64
    
    mov rax, [gs:0x60]
    movzx rbx, byte [rax + 0x2]
    test rbx, rbx
    jnz .fail_64
    
    mov rax, 1
    jmp .done_64

.fail_64:
    xor rax, rax

.done_64:
    lea rbx, [rel key_result]
    mov [rbx], eax

    push 0x23
    lea rcx, [rel back_from_64]
    push rcx
    db 0x48
    retf