//
// Created by skiftare on 23.09.2025.
//

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h> //for memcpy
#include <sys/mman.h>
#include <sys/stat.h>
#include <syscall.h>
#include <unistd.h>

#define BUFFER_SIZE (10 * 1024)
#define NUM_STR_SIZE 21
const short stdout_desc = 1;
/*
Интерфейс программы - 1 аргумент, это название файлика с вычислениями

в файле не больше 10 строчек
каждая строчка не длиннее 1024
 *
 */

// Я хз что мы будем передавать в buf, но мы это будем писать в файл.

/*
 * SYNOPSIS         top
       #include <sys/syscall.h>      * Definition of SYS_* constants *
       #include <unistd.h>

       long syscall(long number, ...);
       Выдержка из man, я уверен что где-то я видел ssize_t, то я и так сижу в
 виме в виртуалке, пускай будет long
       */
long write_in_file(int file_desc, const void *buf, int count) {
  return syscall(SYS_write, file_desc, buf, count);
}

long prt(const void *buf, int count) {
  return write_in_file(stdout_desc, buf, count);
}

void exit_with_syscall(int code) { syscall(SYS_exit, code); }

enum token_type {
  end_line = 0,
  number = 1,
  operator= 2,
  error = 4,
};

// Мы передаём указатель на указатель который смотрит на буфер
// Передадим указатель, смотрящий на буфер он не сдвинется. Придётся отдельно
// трекать то сколько прошли и двигать его val_for_seek - это там значения. Если
// error на return, то в val_for_seek какой-то мусор в общем случае

enum token_type get_token(char **p, long *val_for_seek) {
  while (**p == ' ' || **p == '\t')
    (*p)++;
  char curr = **p;
  if (**p == '\0' || **p == '\n') {
    return end_line;
  }
  bool flag_for_minus = false;
  if (curr == '-') {
    if (*(*p + 1) >= '0' && *(*p + 1) <= '9') {
      // Это отрицательно число
      flag_for_minus = true;
      (*p)++;
    }
  }
  curr = **p;

  if (curr >= '0' && curr <= '9') {
    long val = 0;
    while (**p >= '0' && **p <= '9') {
      val = val * 10 + (**p - '0');
      (*p)++;
    }
    *val_for_seek = val * (flag_for_minus ? -1 : 1);
    return number;
  }
  if (curr == '+' || curr == '-' || curr == '*' || curr == '/') {
    *val_for_seek = curr;
    (*p)++;
    return operator;
  }
  // Сюда не должны дойти
  return error;
}

int num_to_str(long n, char *buf) {
  char temp_buf[NUM_STR_SIZE];
  int i = 0;
  long is_negative = n < 0;

  if (n == 0) {
    buf[0] = '0';
    buf[1] = '\0';
    return 1;
  }

  if (is_negative) {
    n = -n;
  }

  while (n > 0) {
    temp_buf[i++] = (n % 10) + '0';
    n /= 10;
  }

  if (is_negative) {
    temp_buf[i++] = '-';
  }

  int j = 0;
  while (i > 0) {
    buf[j++] = temp_buf[--i];
  }
  buf[j] = '\0';
  return j;
}

long evaluate(char **p, bool *error_flag) {
  if (*error_flag) {
    return 0;
  }
  long token_value;
  int token_type = get_token(p, &token_value);

  switch (token_type) {
  case number:
    return token_value;

  case operator: {
    long op1 = evaluate(p, error_flag);
    if (*error_flag) {
      return op1;
    }
    long op2 = evaluate(p, error_flag);
    if (*error_flag) {
      return op2;
    }
    char operator=(char) token_value;

    switch (operator) {
    case '+':
      return op1 + op2;
    case '-':
      return op1 - op2;
    case '*':
      return op1 * op2;
    case '/':
      if (op2 == 0) {
        *error_flag = true;
        return 1;
      }
      return op1 / op2;
    }
  }

  case end_line:
    *error_flag = true;
    return -1;
  default:
    *error_flag = true;
    return 2;
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    prt("./main file", 8 + 3);
    prt("\n", 1);
    exit_with_syscall(2);
  }

  const char *filaname = argv[1]; // Не поменяем
  long file_desc = syscall(SYS_open, filaname, O_RDONLY); // логично что RDonly
  if (file_desc == -1) {
    prt("err open", 8);
    prt("\n", 1);
    exit_with_syscall(3);
  }
  char buffer[BUFFER_SIZE];
  // long bytes_read = syscall(SYS_read, file_desc, buffer, sizeof(buffer) - 1);
  /*
   *
   * Ну раз решили писать нормально, то мы можем прикрутить обработку на размер
   *файла, потом маппить его в память С помощью нейронки я узнал о fstat, в
   *целом почему бы и нет
   *
   *On success, zero is returned. On error, -1 is returned, and errno is set to
   *indicate the error.
   * */
  struct stat st;
  long fstat_val = syscall(SYS_fstat, file_desc, &st);
  if (fstat_val == -1) {
    prt("fstat err\n", 10);
    exit_with_syscall(4);
  }
  long real_file_size = st.st_size;
  if (real_file_size > BUFFER_SIZE - 1) {
    prt("wrong size of file\n", 19);
    syscall(SYS_close, file_desc);
    exit_with_syscall(5);
  }
  //	https://manpages.debian.org/unstable/manpages-dev/mmap.2.en.html

  void *mapped = (void *)syscall(SYS_mmap, 0, real_file_size, PROT_READ,
                                 MAP_PRIVATE, file_desc, 0);
  /*
  RETURN VALUE
  On success, mmap() returns a pointer to the mapped area. On error, the value
  MAP_FAILED (that is, (void *) -1) is returned, and errno is set to indicate
  the error.

  On success, munmap() returns 0. On failure, it returns -1, and errno is set to
  indicate the error (probably to EINVAL).
  */
  if (mapped == MAP_FAILED) {
    prt("mmap failed\n", 12);
    syscall(SYS_close, file_desc);
    exit_with_syscall(6);
  }

  memcpy(buffer, mapped, real_file_size);
  buffer[real_file_size] = '\0';
  // Размаппливаем всё обратно, т.к. данные уже в буфере.
  syscall(SYS_munmap, mapped, real_file_size);
  syscall(SYS_close, file_desc);

  char *p = buffer;
  bool flag = false;
  while ((p - buffer) < real_file_size) {
    long result = evaluate(&p, &flag);
    if (flag) {
      if (result == -1) {
        // + 3
        prt("a lot ops", 9);
        prt("\n", 1);
      }
      if (result == 1) {
        // 2/0
        prt("div to 0", 8);
        prt("\n", 1);
      }
      if (result == 2) {
        prt("parse err", 9);
        prt("\n", 1);
      }
      flag = false;
    } else {
      char result_str[NUM_STR_SIZE];
      int len = num_to_str(result, result_str);
      prt(result_str, len);
      prt("\n", 1);
    }
    while (*p != '\n' && *p != '\0') {
      p++;
    }
    if (*p == '\n') {
      p++;
    }
  }
  exit_with_syscall(0);
}
