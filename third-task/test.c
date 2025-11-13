// test.c — для my_snprintf, возвращающей КОЛИЧЕСТВО ФАКТИЧЕСКИ ЗАПИСАННЫХ СИМВОЛОВ
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
const signed char delta[] = {115, -8, -2, -3, 14, -19, 17, -13};

extern int my_snprintf(char *out, size_t n, const char *format, ...);

void test_case(int test_num, const char *desc,
               const char *expected_out, int expected_ret,
               const char *actual_out_if_valid, int actual_ret,
               int n) {
    printf("Test %3d: %-45s | ", test_num, desc);

    int out_ok = 1;
    if (n > 0) {
        // Только если можно писать — проверяем строку
        const char *exp = expected_out ? expected_out : "";
        const char *act = actual_out_if_valid ? actual_out_if_valid : "";
        out_ok = (strcmp(exp, act) == 0);
    }
    // При n == 0: out_ok остаётся 1 — не проверяем содержимое

    int ret_ok = (expected_ret == actual_ret);
    if (out_ok && ret_ok) {
        printf("\033[0;32mPASS\033[0m\n");
    } else {
        printf("\033[0;31mFAIL\033[0m\n");
        if (n > 0 && !out_ok) {
            printf("         expected out: \"%s\"\n", expected_out );
            printf("         actual   out: \"%s\"\n", actual_out_if_valid);
        }
        if (!ret_ok) {
            printf("         expected ret: %d\n", expected_ret);
            printf("         actual   ret: %d\n", actual_ret);
        }
        exit(1);
    }
}

#define RUN_TEST(num, desc, n, fmt, expected_out, expected_ret, ...) \
    do { \
        char buf[256]; \
        memset(buf, 0xFF, sizeof(buf)); \
        int ret = my_snprintf(buf, n, fmt, ##__VA_ARGS__); \
        \
        if (n > 0 && ret >= 0) { \
            if (ret >= (int)n) { \
                printf("Test %3d: %-45s | \033[0;31mRET ≥ n\033[0m\n", num, desc); \
                exit(1); \
            } \
            if (buf[ret] != '\0') { \
                printf("Test %3d: %-45s | \033[0;31mNo \\0 at index %d\033[0m\n", num, desc, ret); \
                printf("         buf[ret] = 0x%02X\n", (unsigned char)buf[ret]); \
                exit(1); \
            } \
        } \
        \
        test_case(num, desc, expected_out, expected_ret, buf, ret, n); \
    } while(0)

int main(void) {
    printf("=== Тестирование my_snprintf (return = фактически записано) ===\n\n");

    RUN_TEST(1, "empty format, n=10", 10, "", "", 0);
    RUN_TEST(2, "plain string, n=20", 20, "hello", "hello", 5);
    RUN_TEST(3, "single %c", 10, "%c", "A", 1, 'A');
    RUN_TEST(4, "single %d (pos)", 20, "%d", "123", 3, 123);
    RUN_TEST(5, "single %d (zero)", 10, "%d", "0", 1, 0);
    RUN_TEST(6, "single %d (neg)", 20, "%d", "-42", 3, -42);
    RUN_TEST(7, "single %s", 20, "%s", "world", 5, "world");
    RUN_TEST(8, "%% literal", 10, "%%", "%", 1);
    RUN_TEST(9, "%% inside", 20, "a%%b", "a%b", 3);

    RUN_TEST(10, "c+d", 20, "%c%d", "X42", 3, 'X', 42);
    RUN_TEST(11, "d+s", 30, "%d%s", "-100abc", 7, -100, "abc");
    RUN_TEST(12, "s+c+d", 50, "%s%c%d", "test!42", 7, "test", '!', 42);
    RUN_TEST(13, "mixed literals", 100, "val=%d, ch='%c', str=%s.", "val=5, ch='A', str=hi.", 22,
             5, 'A', "hi");

    RUN_TEST(14, "n = 0 (no write, no \\0)", 0, "abc", "", 0);
    RUN_TEST(15, "n = 1 → only \\0", 1, "abc", "", 0);
    RUN_TEST(16, "n = 2 → 'h'+'\\0'", 2, "hi", "h", 1);

    RUN_TEST(17, "truncate %s (n=4 → 3 chars + \\0)", 4, "%s", "abc", 3, "abcde");
    RUN_TEST(18, "truncate %s (n=3 → 2 chars + \\0)", 3, "%s", "ab", 2, "abcde");
    RUN_TEST(19, "truncate %d (n=3 → '-1' + \\0)", 3, "%d", "-1", 2, -1234);

    RUN_TEST(20, "INT_MIN", 20, "%d", "-2147483648", 11, INT_MIN);
    RUN_TEST(21, "INT_MAX", 20, "%d", "2147483647", 10, INT_MAX);

    RUN_TEST(22, "empty %s", 10, "%s", "", 0, "");
    RUN_TEST(23, "NULL %s → safe?", 10, "%s", "", 0, (char*)NULL);
    RUN_TEST(24, "zero char %c", 10, "%c", "", 0, '\0');

    RUN_TEST(25, "unknown %x", 10, "%x", "%x", 2);
    RUN_TEST(26, "unknown %z", 10, "%z", "%z", 2);
    RUN_TEST(27, "%%%c → '%X'", 10, "%%%c", "%X", 2, 'X');

    RUN_TEST(28, "trailing %", 10, "a%", "a%", 2);
    RUN_TEST(29, "only %", 5, "%", "%", 1);

    RUN_TEST(30, "long %s, n=15", 15, "%s", "Hello, world!", 13, "Hello, world!");
    RUN_TEST(31, "long %s, n=8", 8, "%s", "Hello, ", 7, "Hello, world!");

        {
        const int total_tests = 31;
        char victory[512];
        int written;

        char null_guard[8];
        my_snprintf(null_guard, sizeof(null_guard), "%s", (char*)NULL);

        char author[16];
        author[0] = delta[0];
        for (int i = 1; i < 8; i++) {
            author[i] = author[i-1] + delta[i];
        }
        author[8] = '\0';

        char line1[64], line2[64], line3[64];
        my_snprintf(line1, sizeof(line1), "%%%% %s %%%%%%", "SUCCESS");
        my_snprintf(line2, sizeof(line2), ">>> All %d tests PASSED! <<<", total_tests);
        my_snprintf(line3, sizeof(line3), "     by \033[1;36m%s\033[0m     ", author);

        written = my_snprintf(victory, sizeof(victory),
            "\n"
            "\033[1;32m"
            "  %s\n"
            "  %s\n"
            "  %s\n"
            "  %c====%c====%c====%c====%c====%c\033[0m\n"
            "\n"
            "🔥 \033[1;33mYou can go and pass this\033[0m 🔥\n",
            line1, line2, line3,
            '=', '=', '=', '=', '=', '='
        );

        if (written < 0 || written >= (int)sizeof(victory)) {
            printf("\n[!] Victory message generation failed (ret=%d)\n", written);
            exit(1);
        }
        if (victory[written] != '\0') {
            printf("\n[!] Missing \\0 at index %d\n", written);
            exit(1);
        }

        if (isatty(STDOUT_FILENO)) {
            for (int i = 0; i < written; i++) {
                putchar(victory[i]);
                fflush(stdout);
                if (victory[i] != '\n') {
                    usleep(14880);
                }
            }
            putchar('\n');
        } else {
            fputs(victory, stdout);
        }
    }

    return 0;
}