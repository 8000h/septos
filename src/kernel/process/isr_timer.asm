BITS 64

global _asm_isr_timer
global _asm_isr_timer_continue

extern scheduler_context_switch

_asm_isr_timer:
    cli

    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8

    push rbp
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx
    push rax

    mov rdi, rsp
    jmp scheduler_context_switch

_asm_isr_timer_continue:
    mov cr3, rdi
    mov rsp, rsi

    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rbp

    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

    sti
    iretq