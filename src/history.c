#include "history.h"

#include <stdlib.h>
#include <string.h>

void history_init(History *h) {
    h->head = h->tail = NULL;
    h->count = 0;
}

static void remove_oldest(History *h) {
    HistoryEntry *old = h->head;
    if (!old) return;
    h->head = old->next;
    if (!h->head) h->tail = NULL;
    free(old->expr);
    free(old);
    h->count--;
}

int history_add(History *h, const char *expr, double value) {
    HistoryEntry *e = malloc(sizeof *e);
    size_t len = strlen(expr);
    if (!e) return 0;
    e->expr = malloc(len + 1);
    if (!e->expr) { free(e); return 0; }
    memcpy(e->expr, expr, len + 1);
    e->value = value;
    e->next = NULL;

    if (h->tail) h->tail->next = e;
    else         h->head = e;
    h->tail = e;
    h->count++;

    if (h->count > HISTORY_MAX) remove_oldest(h);
    return 1;
}

const HistoryEntry *history_get(const History *h, int index) {
    const HistoryEntry *e = h->head;
    int i;
    if (index < 1 || index > h->count) return NULL;
    for (i = 1; i < index; i++) e = e->next;
    return e;
}

void history_print(const History *h, FILE *out) {
    const HistoryEntry *e;
    int i = 1;
    if (!h->head) {
        fprintf(out, "  (Lich su trong)\n");
        return;
    }
    for (e = h->head; e; e = e->next, i++)
        fprintf(out, "  %3d. %s = %.12g\n", i, e->expr, e->value);
}

void history_clear(History *h) {
    while (h->head) remove_oldest(h);
}

int history_save(const History *h, const char *path) {
    const HistoryEntry *e;
    FILE *f = fopen(path, "w");
    if (!f) return 0;
    for (e = h->head; e; e = e->next)
        fprintf(f, "%s\t%.17g\n", e->expr, e->value);
    return fclose(f) == 0;
}

int history_append_file(const char *path, const char *expr, double value) {
    FILE *f = fopen(path, "a");
    if (!f) return 0;
    fprintf(f, "%s\t%.17g\n", expr, value);
    return fclose(f) == 0;
}

int history_load(History *h, const char *path) {
    char line[1024];
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    while (fgets(line, sizeof line, f)) {
        char *tab = strrchr(line, '\t');
        char *end;
        double v;
        if (!tab) continue;                 /* bo qua dong hong */
        *tab = '\0';
        v = strtod(tab + 1, &end);
        if (end == tab + 1) continue;
        if (line[0]) history_add(h, line, v);
    }
    fclose(f);
    return 1;
}
