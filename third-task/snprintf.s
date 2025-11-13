.text

# size_t my_strlen(const char *s);
.globl my_strlen
my_strlen:
    push %rbp
    mov %rsp, %rbp
    mov %rdi, %rax
    xor %rcx, %rcx

strlen_loop:
    cmpb $0, (%rax, %rcx, 1)
    je strlen_done
    inc %rcx
    jmp strlen_loop

strlen_done:
    mov %rcx, %rax          # длина → return
    pop %rbp
    ret


# int my_snprintf(char *out, size_t n, const char *fmt, ...);
.globl my_snprintf
my_snprintf:
    push %rbp
    mov %rsp, %rbp
    sub $128, %rsp          # локальный буфер для emit_int (цифры)

    mov %rdi, -8(%rbp)      # out
    mov %rsi, -16(%rbp)     # n
    mov %rdx, -24(%rbp)     # fmt
    mov %rcx, -32(%rbp)     # arg0  (1st vararg)
    mov %r8,  -40(%rbp)     # arg1  (2nd vararg)
    mov %r9,  -48(%rbp)     # arg2  (3rd vararg)

    movq $0, -56(%rbp)      # written — сколько реально записано (до n-1)
    movq $0, -64(%rbp)      # fmt_i — индекс в fmt
    movq $0, -72(%rbp)      # arg_i — счётчик обработанных аргументов

fmt_loop:
    #Загрузка текущего символа fmt[fmt_i]
    mov -24(%rbp), %rdi     # fmt
    mov -64(%rbp), %rcx     # fmt_i
    movzb (%rdi, %rcx, 1), %eax   # %al = fmt[fmt_i]
    test %al, %al
    jz fmt_end              # '\0' → завершение

    cmpb $'%', %al
    jne write_normal

    #% → проверка спецсимвола
    incq -64(%rbp)          # продвигаемся за '%'
    mov -24(%rbp), %rdi
    mov -64(%rbp), %rcx
    movzbl (%rdi, %rcx, 1), %eax   # следующий символ

    test %al, %al
    jz literal_percent      # "%\0" → печатаем '%', done

    cmpb $'c', %al
    je do_c
    cmpb $'s', %al
    je do_s
    cmpb $'d', %al
    je do_d
    cmpb $'%', %al
    je do_percent
    #Если неизевстный спецификатор, выводим как есть

unknown_spec:
    push %rax
    movb $'%', %al
    call emit_char
    pop %rax
    call emit_char
    incq -64(%rbp)          # пропускаем оба символа: '%' и 'X'
    jmp fmt_loop


literal_percent:
    movb $'%', %al
    call emit_char
    # fmt_i уже указывает на \0 → дальше fmt_loop сам выйдет
    jmp fmt_loop

do_c:
    call get_arg            # в %rax — int (char расширен до 64 бит)
    cmpb $0, %al            # НУЛЬ-символ — НЕ записываем
    je fmt_next
    call emit_char
    jmp fmt_next

do_s:
    call get_arg            # в %rax — const char *
    call emit_string        # внутри: если %rax == NULL → пропуск
    jmp fmt_next

do_d:
    call get_arg            # в %rax — int32 (передан как sign-extended)
    movsxd %eax, %rax       # sign-extend eax → rax (если нужно)
    call emit_int
    jmp fmt_next

do_percent:
    movb $'%', %al
    call emit_char
    jmp fmt_next

write_normal:
    call emit_char
    jmp fmt_next

fmt_next:
    incq -64(%rbp)          # продвигаем fmt_i
    jmp fmt_loop

fmt_end:
    #запись '\0', если есть место
    mov -56(%rbp), %rax     # written
    mov -16(%rbp), %rbx     # n

    cmpq $0, %rbx
    je no_null
    cmp %rbx, %rax
    jge no_null             # written >= n → нет места под '\0'
    mov -8(%rbp), %rdi      # out
    movb $0, (%rdi, %rax, 1)

no_null:
    mov %rbp, %rsp
    pop %rbp
    ret



# get_arg() → %rax
# Возвращает очередной vararg; инкрементирует arg_i.
# arg0, arg1, arg2 → регистры, остальное — стек.
get_arg:
    mov -72(%rbp), %rcx     # arg_i
    cmp $3, %rcx
    jl .use_regs

    # Стековые аргументы: 1st at [rbp+16], 2nd at +24, ...
    lea 16(%rbp), %rax
    sub $3, %rcx            # коррекция смещения: arg_i = 3 → [rbp+16]
    movq (%rax, %rcx, 8), %rax
    jmp .done_r

.use_regs:
    cmp $0, %rcx
    je .a0
    cmp $1, %rcx
    je .a1
    # else: rcx == 2
    mov -48(%rbp), %rax     # r9
    jmp .done_r
.a0:
    mov -32(%rbp), %rax     # rcx
    jmp .done_r
.a1:
    mov -40(%rbp), %rax     # r8
.done_r:
    incq -72(%rbp)          # arg_i++
    ret


# emit_char() — запись одного символа %al в out, если есть место.
# Учитывает: n ≥ 1 и written < n-1.
emit_char:
    mov -16(%rbp), %rdx     # n
    cmp $1, %rdx
    jl .no_space            # n ≤ 0 → nowhere to write

    mov -56(%rbp), %rcx     # written
    dec %rdx                # rdx = n-1
    cmp %rdx, %rcx          # written ≥ n-1?
    jge .no_space

    # Записываем байт
    mov -8(%rbp), %rdi      # out
    mov %al, (%rdi, %rcx, 1)
    incq -56(%rbp)          # written++
    ret

.no_space:
    ret


# emit_string() — вывод строки из %rax (возможно NULL).
# Если NULL — ничего не делаем
# Иначе — посимвольно через emit_char.
emit_string:
    push %rbx

    test %rax, %rax
    jz .done_string         # NULL → skip

    mov %rax, %rbx          # src
.loop_string:
    movb (%rbx), %al
    test %al, %al
    jz .done_string
    call emit_char
    inc %rbx
    jmp .loop_string

.done_string:
    pop %rbx
    ret


# emit_int() — вывод знакового целого из %rax.
# Использует локальный буфер [rbp-128 .. rbp-109] для цифр.
emit_int:
    push %rbx
    push %rcx
    push %rdx
    mov %rax, %rbx

    test %rbx, %rbx
    jnz .not_zero
    movb $'0', %al
    call emit_char
    jmp .end

.not_zero:
    cmp $0, %rbx
    jge .positive
    neg %rbx
    movb $'-', %al
    call emit_char

.positive:
    lea -128(%rbp), %rdi   # буферный старт (128 байт под стек — хватит)
    mov %rdi, %rsi
    add $20, %rsi          # 20 байт вперёд — будем писать цифры в обратном порядке
    xor %r11, %r11         # digit counter
    mov $10, %rcx          # делитель

.dgt_loop:
    mov %rbx, %rax
    xor %rdx, %rdx
    div %rcx               # rax /= 10, rdx = остаток
    add $'0', %dl
    dec %rsi
    mov %dl, (%rsi)
    inc %r11
    mov %rax, %rbx
    test %rbx, %rbx
    jnz .dgt_loop

.out_loop:
    test %r11, %r11
    jz .end
    movzb (%rsi), %eax     # byte → zero-extend в %eax (для emit_char)
    call emit_char
    inc %rsi
    dec %r11
    jmp .out_loop

.end:
    pop %rdx
    pop %rcx
    pop %rbx
    ret