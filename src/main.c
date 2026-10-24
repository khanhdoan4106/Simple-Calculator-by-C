/*
 * main.c - Giao dien dong lenh (REPL) cho may tinh.
 *
 * Cach dung:
 *   ./calc                 che do tuong tac
 *   ./calc "2 + 3 * 4"     tinh mot bieu thuc roi thoat (ma thoat 1 neu loi)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "calc.h"
#include "history.h"

#define LINE_CAP (CALC_MAX_INPUT + 2)

static char history_path[1024];

static void print_help(void) {
    printf("\n"
           "  Nhap bieu thuc roi bam Enter, vi du:\n"
           "    2 + 3 * 4          -> 14\n"
           "    (1 + 2)^3 / 9      -> 3\n"
           "    sqrt(16) + 5!      -> 124\n"
           "    sin(pi / 2)        -> 1\n"
           "    r = 5              (gan bien)\n"
           "    pi * r^2           -> 78.5398...\n"
           "    ans * 2            (ans = ket qua truoc)\n"
           "\n"
           "  Toan tu : + - * / %% ^ !   (^ ket hop phai, -2^2 = -4)\n"
           "  Ham     : sqrt sin cos tan ln log exp abs   (goc tinh bang radian)\n"
           "  Hang so : pi e ans\n"
           "\n"
           "  Lenh    : help     - hien huong dan nay\n"
           "            history  - xem lich su phep tinh\n"
           "            clear    - xoa lich su\n"
           "            vars     - xem cac bien da gan\n"
           "            quit     - thoat (hoac exit, q, Ctrl+D)\n\n");
}

static void print_vars(const CalcEnv *env) {
    int i;
    printf("  ans = %.12g\n", env->ans);
    if (env->var_count == 0) printf("  (Chua gan bien nao)\n");
    for (i = 0; i < env->var_count; i++)
        printf("  %s = %.12g\n", env->vars[i].name, env->vars[i].value);
}

/* In dau ^ ngay duoi vi tri loi de nguoi dung thay ngay cho sai. */
static void print_error(const char *expr, const CalcResult *r, int show_caret) {
    if (show_caret && r->pos >= 0) {
        int i;
        printf("  %s\n  ", expr);
        for (i = 0; i < r->pos; i++) putchar(expr[i] == '\t' ? '\t' : ' ');
        printf("^\n");
    }
    printf("  [!] Loi: %s\n", calc_error_message(r->error));
}

/* Bo khoang trang dau/cuoi, tra ve con tro vao vung giua. */
static char *trim(char *s) {
    char *end;
    while (*s == ' ' || *s == '\t') s++;
    end = s + strlen(s);
    while (end > s && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\n' || end[-1] == '\r')) end--;
    *end = '\0';
    return s;
}

static void setup_history_path(void) {
    const char *home = getenv("HOME");
#ifdef _WIN32
    if (!home) home = getenv("USERPROFILE");
#endif
    if (home && strlen(home) < sizeof history_path - 20)
        sprintf(history_path, "%s/.calc_history", home);
    else
        strcpy(history_path, "calc_history.txt");
}

/* Che do mot lenh: ghep cac tham so thanh mot bieu thuc. */
static int run_once(int argc, char **argv) {
    char expr[LINE_CAP] = "";
    CalcEnv env;
    CalcResult r;
    size_t used = 0;
    int i;

    for (i = 1; i < argc; i++) {
        size_t len = strlen(argv[i]);
        if (used + len + 1 >= sizeof expr) {
            printf("  [!] Loi: %s\n", calc_error_message(CALC_ERR_TOO_LONG));
            return 1;
        }
        if (i > 1) expr[used++] = ' ';
        memcpy(expr + used, argv[i], len + 1);
        used += len;
    }

    calc_env_init(&env);
    r = calc_eval(&env, expr);
    if (r.error != CALC_OK) {
        print_error(expr, &r, 1);
        return 1;
    }
    printf("%.12g\n", r.value);
    return 0;
}

int main(int argc, char **argv) {
    char line[LINE_CAP];
    CalcEnv env;
    History hist;

    if (argc > 1) return run_once(argc, argv);

    calc_env_init(&env);
    history_init(&hist);
    setup_history_path();
    /* Nap lich su cu; ghi lai ngay de file khong phinh qua HISTORY_MAX dong */
    if (history_load(&hist, history_path)) history_save(&hist, history_path);

    printf("\n  ===== MAY TINH BIEU THUC =====\n");
    printf("  Go 'help' de xem huong dan, 'quit' de thoat.\n\n");

    for (;;) {
        char *expr;
        CalcResult r;

        printf("> ");
        fflush(stdout);
        if (!fgets(line, sizeof line, stdin)) {      /* Ctrl+D / het input */
            printf("\n");
            break;
        }

        /* Dong qua dai: doc bo phan con lai de khong lam hong lan nhap sau */
        if (!strchr(line, '\n') && !feof(stdin)) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {}
            printf("  [!] Loi: %s (toi da %d ky tu)\n", calc_error_message(CALC_ERR_TOO_LONG), CALC_MAX_INPUT);
            continue;
        }

        expr = trim(line);
        if (*expr == '\0') continue;

        if (!strcmp(expr, "quit") || !strcmp(expr, "exit") || !strcmp(expr, "q")) break;
        if (!strcmp(expr, "help"))    { print_help(); continue; }
        if (!strcmp(expr, "history")) { history_print(&hist, stdout); continue; }
        if (!strcmp(expr, "vars"))    { print_vars(&env); continue; }
        if (!strcmp(expr, "clear")) {
            history_clear(&hist);
            history_save(&hist, history_path);
            printf("  Da xoa lich su.\n");
            continue;
        }

        r = calc_eval(&env, expr);
        if (r.error != CALC_OK) {
            print_error(expr, &r, 1);
            continue;
        }

        if (r.assigned[0]) printf("  %s = %.12g\n", r.assigned, r.value);
        else               printf("  = %.12g\n", r.value);

        if (history_add(&hist, expr, r.value))
            history_append_file(history_path, expr, r.value);
    }

    printf("  Tam biet! Hen gap lai.\n");
    history_clear(&hist);
    return 0;
}
