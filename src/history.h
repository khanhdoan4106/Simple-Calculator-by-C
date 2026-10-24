/*
 * history.h - Lich su phep tinh, luu bang danh sach lien ket don.
 *
 * Them vao cuoi: O(1) nho con tro tail.
 * Khi vuot qua HISTORY_MAX, phan tu cu nhat (head) bi xoa: O(1).
 */
#ifndef HISTORY_H
#define HISTORY_H

#include <stdio.h>

#define HISTORY_MAX 500

typedef struct HistoryEntry {
    char                *expr;
    double               value;
    struct HistoryEntry *next;
} HistoryEntry;

typedef struct {
    HistoryEntry *head;
    HistoryEntry *tail;
    int           count;
} History;

void                history_init(History *h);
int                 history_add(History *h, const char *expr, double value);  /* 0 neu het bo nho */
const HistoryEntry *history_get(const History *h, int index);                 /* index tu 1 */
void                history_print(const History *h, FILE *out);
void                history_clear(History *h);

/* Luu/doc file: moi dong "bieu_thuc<TAB>gia_tri". Tra ve 0 neu loi. */
int                 history_save(const History *h, const char *path);
int                 history_append_file(const char *path, const char *expr, double value);
int                 history_load(History *h, const char *path);

#endif
