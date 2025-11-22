.text

.globl _start

_start:
    mov $0x68732f6e69622f2f, %rax
    shr $8, %rax
    push %rax
    mov %rsp, %rdi

    xor %rax, %rax
    mov $59, %al

    xor %rsi, %rsi
    xor %rdx, %rdx

    syscall
