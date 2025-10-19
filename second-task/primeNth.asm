.globl _start

.text

_start:
    movq $11, %r8      # n
    movq $2, %r9      # candidate = 2 - это число, которое адо проверить в текущей итерации
    jmp .pre_loop
.pre_loop:
    movq $2, %r11       # делитель
    movb $0, %r10b      # флаг: 0 <-> prime
.loop:
    #r11 * r11 > r9 → prime
    movq %r11, %rax
    mulq %r11	# rax = r11^2
    cmpq %r9, %rax
    jg .is_prime

    # r9 % r11 == 0?
    movq %r9, %rax
    movq $0, %rdx

    divq %r11
    cmpq $0, %rdx
    je .not_prime

    incq %r11
    jmp .loop

.is_prime:
    movb $1, %r10b
    jmp .verdict	#оно нужно иначе мы потом в not rpime упадём

.not_prime:
    movb $0, %r10b

.verdict:
    movzbq %r10b, %rax	# Это расширение байта до 8 байт чтобы типы были одинаковые
    subq %rax, %r8	# r8 -= (r10b ? 1 : 0)
    cmpq $0, %r8
    je .done	# r8 == 0

    incq %r9
    jmp .pre_loop
.done:
    movq $60, %rax	# sys_exit
    movq %r9, %rdi 
    syscall
