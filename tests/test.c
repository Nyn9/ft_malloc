#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include "ft_malloc.h"

typedef struct { const char *name; void (*fn)(void); int expect_segv; } t_test;

static int g_fail = 0;

#define CHECK(cond, msg) do { \
    if (cond) printf("  [OK]   %s\n", msg); \
    else { printf("  [FAIL] %s (ligne %d)\n", msg, __LINE__); g_fail++; } \
} while (0)

static int fits_in_one_zone(size_t size, size_t zone) {
    size_t page = sysconf(_SC_PAGESIZE);
    uintptr_t first = (uintptr_t)malloc(size);
    if (!first)
        return 0;
    uintptr_t start = first & ~(uintptr_t)(page - 1);
    for (int i = 1; i < 100; i++) {
        uintptr_t p = (uintptr_t)malloc(size);
        if (!p || p < start || p + size > start + zone)
            return 0;
    }
    return 1;
}

static void test_100_per_zone(void) {
    printf("Test 1 : 100 allocations de taille max dans une meme zone\n");
    size_t page = sysconf(_SC_PAGESIZE);
    CHECK(fits_in_one_zone(128, 4 * page), "100 blocs TINY de 128 octets tiennent dans 4 pages");
    CHECK(fits_in_one_zone(4096, 128 * page), "100 blocs SMALL de 4096 octets tiennent dans 128 pages");
}

static void test_basic(void) {
    printf("Test 2 : allocation simple\n");
    char *p = malloc(42);
    CHECK(p != NULL, "retourne non NULL");
    if (!p) return;
    memset(p, 'A', 42);
    int ok = 1;
    for (int i = 0; i < 42; i++)
        if (p[i] != 'A') ok = 0;
    CHECK(ok, "ecriture/lecture de 42 octets");
}

static void test_alignment(void) {
    printf("Test 3 : alignement sur 16 (tailles 1 a 600)\n");
    int ok = 1;
    for (size_t s = 1; s <= 600; s++) {
        void *p = malloc(s);
        if (!p || (uintptr_t)p % 16 != 0) {
            printf("    taille %zu : %p\n", s, p);
            ok = 0;
        }
    }
    CHECK(ok, "tous les pointeurs sont des multiples de 16");
}

static void test_no_overlap(void) {
    printf("Test 4 : pas de chevauchement entre blocs\n");
    enum { N = 200 };
    unsigned char *ptrs[N];
    size_t sizes[N];
    int ok = 1;
    for (int i = 0; i < N; i++) {
        sizes[i] = 1 + (i * 37) % 300;
        ptrs[i] = malloc(sizes[i]);
        if (!ptrs[i]) { ok = 0; break; }
        memset(ptrs[i], i % 251, sizes[i]);
    }
    CHECK(ok, "200 allocations de tailles variees");
    for (int i = 0; i < N && ok; i++)
        for (size_t j = 0; j < sizes[i]; j++)
            if (ptrs[i][j] != (unsigned char)(i % 251)) { ok = 0; break; }
    CHECK(ok, "contenu de chaque bloc intact");
}

static void test_zones(void) {
    printf("Test 5 : TINY / SMALL / LARGE\n");
    size_t page = sysconf(_SC_PAGESIZE);
    size_t tiny_max = 4 * page / 128;
    size_t small_max = 16 * page / 128;
    void *t = malloc(tiny_max);
    void *s = malloc(tiny_max + 1);
    void *s2 = malloc(small_max);
    void *l = malloc(small_max + 1);
    CHECK(t && s && s2 && l, "toutes les bornes retournent un pointeur");
    if (t) memset(t, 1, tiny_max);
    if (s) memset(s, 2, tiny_max + 1);
    if (s2) memset(s2, 3, small_max);
    if (l) memset(l, 4, small_max + 1);
    CHECK(1, "ecriture complete aux bornes sans crash");
}

static void test_new_heap(void) {
    printf("Test 6 : debordement vers une nouvelle zone\n");
    int ok = 1;
    for (int i = 0; i < 1000; i++) {
        char *p = malloc(128);
        if (!p) { ok = 0; break; }
        memset(p, 0xAB, 128);
    }
    CHECK(ok, "1000 allocations TINY (plusieurs zones)");
}

static void test_large(void) {
    printf("Test 7 : LARGE\n");
    size_t sz = 5 * 1024 * 1024;
    unsigned char *p = malloc(sz);
    CHECK(p != NULL, "5 Mo");
    if (!p) return;
    memset(p, 0x5A, sz);
    CHECK(p[0] == 0x5A && p[sz - 1] == 0x5A, "premier et dernier octet ecrits");
    CHECK((uintptr_t)p % 16 == 0, "aligne");
    void *p2 = malloc(sz);
    CHECK(p2 != NULL && p2 != (void *)p, "deux LARGE distincts");
}

static void test_errors(void) {
    printf("Test 8 : tailles invalides\n");
    volatile size_t huge = SIZE_MAX;
    CHECK(malloc(huge) == NULL, "SIZE_MAX -> NULL");
    huge = SIZE_MAX - 1;
    CHECK(malloc(huge) == NULL, "SIZE_MAX - 1 -> NULL");
    huge = SIZE_MAX - 4096;
    CHECK(malloc(huge) == NULL, "SIZE_MAX - 4096 -> NULL");
    huge = (size_t)1 << 62;
    CHECK(malloc(huge) == NULL, "2^62 -> NULL (mmap echoue)");
}

static void test_zero(void) {
    printf("Test 9 : malloc(0)\n");
    void *p = malloc(0);
    void *q = malloc(0);
    CHECK(p != NULL && q != NULL, "malloc(0) retourne un pointeur non NULL");
    CHECK((uintptr_t)p % 16 == 0 && (uintptr_t)q % 16 == 0, "pointeurs alignes sur 16");
    CHECK(p != q, "deux malloc(0) donnent deux pointeurs differents");
    void *r = malloc(16);
    CHECK(r != NULL && r != p && r != q, "malloc(16) distinct des deux precedents");
}

static void test_stress(void) {
    printf("Test 10 : stress (10000 allocations aleatoires)\n");
    enum { N = 10000 };
    static unsigned char *ptrs[N];
    static size_t sizes[N];
    srand(42);
    int ok = 1;
    for (int i = 0; i < N; i++) {
        int r = rand() % 100;
        sizes[i] = r < 70 ? 1 + rand() % 128
                 : r < 95 ? 129 + rand() % 384
                 : 513 + rand() % 20000;
        ptrs[i] = malloc(sizes[i]);
        if (!ptrs[i] || (uintptr_t)ptrs[i] % 16) { ok = 0; break; }
        memset(ptrs[i], i & 0xFF, sizes[i]);
    }
    CHECK(ok, "allocations reussies et alignees");
    for (int i = 0; i < N && ok; i++)
        for (size_t j = 0; j < sizes[i]; j++)
            if (ptrs[i][j] != (unsigned char)(i & 0xFF)) { ok = 0; break; }
    CHECK(ok, "aucune corruption entre blocs");
}


/* ---------- tests free ---------- */

static void test_free_null(void) {
    printf("Test 11 : free(NULL)\n");
    free(NULL);
    void *p = malloc(32);
    CHECK(p != NULL, "malloc fonctionne apres free(NULL)");
}

static void test_free_reuse(void) {
    printf("Test 12 : free puis reutilisation du meme emplacement\n");
    char *p = malloc(64);
    CHECK(p != NULL, "premiere allocation");
    if (!p) return;
    memset(p, 'X', 64);
    free(p);
    char *q = malloc(64);
    CHECK(q == p, "meme adresse reutilisee (bloc libre pris en entier)");
}

static void test_double_free(void) {
    printf("Test 13 : double free\n");
    char *a = malloc(64);
    char *b = malloc(64);
    CHECK(a && b, "deux allocations");
    if (!a || !b) return;
    memset(b, 0x77, 64);
    free(a);
    free(a);
    char *c = malloc(64);
    char *d = malloc(64);
    CHECK(c && d, "allocations apres double free");
    CHECK(c != d, "deux allocations distinctes (pas de bloc attribue deux fois)");
    CHECK(c != b && d != b, "le bloc b encore occupe n'est pas reattribue");
    int ok = 1;
    for (int i = 0; i < 64; i++)
        if (b[i] != 0x77) ok = 0;
    CHECK(ok, "donnees de b intactes");
}

static void test_invalid_free(void) {
    printf("Test 14 : free de pointeurs invalides\n");
    static int global;
    int local;
    unsigned char *p = malloc(64);
    CHECK(p != NULL, "allocation");
    if (!p) return;
    memset(p, 0x42, 64);
	volatile size_t off = 1;
	free(p + off);
	off = 16;
	free(p + off);
	off = 8;
    free(p - off);       // dans l'en-tete du bloc
	off = 32;
    free(p - off);      // debut de l'en-tete du bloc
	off = 64;
    free(p + off);      // juste apres les donnees
	void *volatile bad;
	bad = &local;      free(bad);
	bad = &global;     free(bad);
	bad = (void *)1;   free(bad);
    int ok = 1;
    for (int i = 0; i < 64; i++)
        if (p[i] != 0x42) ok = 0;
    CHECK(ok, "donnees intactes apres les free invalides");
    unsigned char *q = malloc(64);
    CHECK(q != NULL && q != p, "p est toujours considere comme occupe");
    free(p);
    unsigned char *r = malloc(64);
    CHECK(r == p, "le free valide fonctionne toujours");
}

static void test_merge_left(void) {
    printf("Test 15 : fusion avec le voisin de gauche\n");
    char *a = malloc(16), *b = malloc(16), *c = malloc(16), *d = malloc(16);
    CHECK(a && b && c && d, "4 allocations de 16");
    (void)c; (void)d;
    free(a);
    free(b);
    void *x = malloc(64);   // 16 + 32 (en-tete absorbe) + 16
    CHECK(x == a, "a et b fusionnes : un bloc de 64 tient a l'adresse de a");
}

static void test_merge_right(void) {
    printf("Test 16 : fusion avec le voisin de droite\n");
    char *a = malloc(16), *b = malloc(16), *c = malloc(16), *d = malloc(16);
    CHECK(a && b && c && d, "4 allocations de 16");
    (void)c; (void)d;
    free(b);
    free(a);
    void *x = malloc(64);
    CHECK(x == a, "b puis a fusionnes : un bloc de 64 tient a l'adresse de a");
}

static void test_merge_both(void) {
    printf("Test 17 : fusion des deux cotes\n");
    char *a = malloc(16), *b = malloc(16), *c = malloc(16), *d = malloc(16);
    CHECK(a && b && c && d, "4 allocations de 16");
    (void)d;
    free(a);
    free(c);
    free(b);
    void *x = malloc(112);  // 3 * 16 + 2 * 32
    CHECK(x == a, "a, b, c fusionnes : un bloc de 112 tient a l'adresse de a");
}

static void test_split(void) {
    printf("Test 18 : split d'un bloc libre\n");
    char *p = malloc(128);
    char *guard = malloc(16);
    CHECK(p && guard, "allocations");
    if (!p || !guard) return;
    free(p);
    char *q = malloc(16);
    CHECK(q == p, "le petit bloc est pris au debut du bloc libre");
    char *r = malloc(16);
    CHECK(r == q + 48, "le reste libre est reutilise juste apres (16 + en-tete 32)");
    memset(q, 1, 16);
    memset(r, 2, 16);
    memset(guard, 3, 16);
    CHECK(q[15] == 1 && r[0] == 2 && guard[0] == 3, "pas de chevauchement");
}

static void test_loop_stable(void) {
    printf("Test 19 : boucle malloc/free (la derniere zone TINY est conservee)\n");
    void *first = malloc(32);
    CHECK(first != NULL, "premiere allocation");
    free(first);
    int ok = 1;
    for (int i = 0; i < 1000; i++) {
        void *p = malloc(32);
        if (p != first) ok = 0;
        free(p);
    }
    CHECK(ok, "meme adresse a chaque tour sur 1000 tours");
}

static void test_free_orders(void) {
    printf("Test 20 : free dans differents ordres\n");
    enum { N = 1000 };
    static void *ptrs[N];
    int ok = 1;
    for (int round = 0; round < 3; round++) {
        for (int i = 0; i < N; i++) {
            ptrs[i] = malloc(128);
            if (!ptrs[i]) { ok = 0; break; }
            memset(ptrs[i], round + 1, 128);
        }
        if (round == 0)
            for (int i = 0; i < N; i++) free(ptrs[i]);
        else if (round == 1)
            for (int i = N - 1; i >= 0; i--) free(ptrs[i]);
        else {
            for (int i = 0; i < N; i += 2) free(ptrs[i]);
            for (int i = 1; i < N; i += 2) free(ptrs[i]);
        }
    }
    CHECK(ok, "3 cycles de 1000 allocations/free (ordre croissant, decroissant, pairs/impairs)");
    CHECK(malloc(100) != NULL, "allocation possible apres tout liberer");
}

static void test_large_free(void) {
    printf("Test 21 : free d'un LARGE rend la zone au systeme\n");
    unsigned char *p = malloc(5 * 1024 * 1024);
    CHECK(p != NULL, "5 Mo alloues");
    if (!p) return;
    memset(p, 1, 5 * 1024 * 1024);
    free(p);
    *(volatile unsigned char *)p = 1;   // doit segfaulter : zone demappee
    CHECK(0, "aucun segfault : la zone n'a pas ete munmap");
}

static void test_large_own_zone(void) {
    printf("Test 22 : chaque LARGE a sa propre zone\n");
    size_t page = sysconf(_SC_PAGESIZE);
    uintptr_t a = (uintptr_t)malloc(5000);
    uintptr_t b = (uintptr_t)malloc(5000);
    CHECK(a && b, "deux LARGE de 600 octets");
    CHECK(a / page != b / page, "dans des pages differentes (zones distinctes)");
}

static void check_list(const char *where) {
    for (t_heap *h = g_heap; h; h = h->next) {
        if (h->next && h->next->prev != h) {
            write(2, "liste cassee apres ", 19);
            write(2, where, strlen(where));
            write(2, "\n", 1);
            abort();
        }
        if (!h->next) break;
    }
}

static void test_free_stress(void) {
    printf("Test 23 : stress malloc/free melanges\n");
    enum { SLOTS = 512, OPS = 50000 };
    static unsigned char *ptrs[SLOTS];
    static size_t sizes[SLOTS];
    static unsigned char tags[SLOTS];
    srand(1234);
    int ok = 1;
    for (int op = 0; op < OPS && ok; op++) {
        int i = rand() % SLOTS;
        if (ptrs[i]) {
            for (size_t j = 0; j < sizes[i]; j++)
                if (ptrs[i][j] != tags[i]) { ok = 0; break; }
            free(ptrs[i]);
			check_list("free");
            ptrs[i] = NULL;
        } else {
            int r = rand() % 100;
            sizes[i] = r < 70 ? 1 + rand() % 128
                     : r < 95 ? 129 + rand() % 384
                     : 513 + rand() % 20000;
            ptrs[i] = malloc(sizes[i]);
			check_list("malloc");
            if (!ptrs[i] || (uintptr_t)ptrs[i] % 16) { ok = 0; break; }
            tags[i] = op & 0xFF;
            memset(ptrs[i], tags[i], sizes[i]);
        }
    }
    CHECK(ok, "50000 operations sans corruption ni pointeur mal aligne");
    for (int i = 0; i < SLOTS; i++) {
        if (!ptrs[i]) continue;
        for (size_t j = 0; j < sizes[i]; j++)
            if (ptrs[i][j] != tags[i]) ok = 0;
        free(ptrs[i]);
    }
    CHECK(ok, "contenu intact avant le free final");
    CHECK(malloc(100) != NULL, "allocation possible apres tout liberer");
}

/* ---------- helpers ---------- */

static void fill(void *p, size_t n, unsigned char v) {
    memset(p, v, n);
}

static int is_filled(const void *p, size_t n, unsigned char v) {
    const unsigned char *c = p;
    for (size_t i = 0; i < n; i++)
        if (c[i] != v) return 0;
    return 1;
}

/* ---------- tests realloc ---------- */

static void test_realloc_null(void) {
    printf("Test 24 : realloc(NULL, n) equivaut a malloc(n)\n");
    void *p = realloc(NULL, 64);
    CHECK(p != NULL, "retourne un pointeur");
    CHECK((uintptr_t)p % 16 == 0, "aligne");
    if (p) { fill(p, 64, 1); free(p); }
}

static void test_realloc_zero(void) {
    printf("Test 25 : realloc(p, 0)\n");
    void *p = malloc(64);
    CHECK(p != NULL, "allocation");
    if (!p) return;
    CHECK(realloc(p, 0) == NULL, "retourne NULL");
    CHECK(malloc(64) != NULL, "malloc fonctionne apres (p a ete libere)");
}

static void test_realloc_same(void) {
    printf("Test 26 : realloc a la meme taille\n");
    unsigned char *p = malloc(64);
    CHECK(p != NULL, "allocation");
    if (!p) return;
    fill(p, 64, 0x11);
    CHECK(realloc(p, 64) == p, "64 -> 64 : meme pointeur");
    CHECK(realloc(p, 60) == p, "64 -> 60 (arrondi a 64) : meme pointeur");
    CHECK(is_filled(p, 64, 0x11), "contenu intact");
    free(p);
}

static void test_realloc_shrink(void) {
    printf("Test 27 : realloc plus petit\n");
    unsigned char *p = malloc(128);
    unsigned char *guard = malloc(16);
    CHECK(p && guard, "allocations");
    if (!p || !guard) return;
    fill(p, 128, 0x22);
    fill(guard, 16, 0x33);
    unsigned char *q = realloc(p, 32);
    CHECK(q == p, "128 -> 32 : meme pointeur");
    CHECK(is_filled(q, 32, 0x22), "32 premiers octets conserves");
    unsigned char *r = malloc(64);
    CHECK(r != NULL && r != q, "le reste libere est reutilisable");
    if (r) fill(r, 64, 0x44);
    CHECK(is_filled(q, 32, 0x22), "pas de chevauchement avec le reste");
    CHECK(is_filled(guard, 16, 0x33), "bloc voisin intact");
}

static void test_realloc_shrink_nosplit(void) {
    printf("Test 28 : realloc plus petit sans place pour un split\n");
    unsigned char *p = malloc(64);
    unsigned char *guard = malloc(16);
    CHECK(p && guard, "allocations");
    if (!p || !guard) return;
    fill(p, 64, 0x55);
    fill(guard, 16, 0x66);
    CHECK(realloc(p, 48) == p, "64 -> 48 : meme pointeur");
    CHECK(realloc(p, 32) == p, "64 -> 32 (reste trop petit) : meme pointeur");
    CHECK(is_filled(p, 32, 0x55), "contenu conserve");
    CHECK(is_filled(guard, 16, 0x66), "bloc voisin intact");
}

static void test_realloc_grow_inplace(void) {
    printf("Test 29 : realloc plus grand sur place (voisin libre)\n");
    unsigned char *a = malloc(32);
    unsigned char *b = malloc(64);
    unsigned char *c = malloc(16);
    CHECK(a && b && c, "3 allocations");
    if (!a || !b || !c) return;
    fill(a, 32, 0x77);
    fill(c, 16, 0x88);
    free(b);
    unsigned char *x = realloc(a, 80);
    CHECK(x == a, "32 -> 80 avec voisin libre : meme pointeur");
    CHECK(x && is_filled(x, 32, 0x77), "32 premiers octets conserves");
    if (x) fill(x, 80, 0x99);
    CHECK(is_filled(c, 16, 0x88), "bloc suivant intact apres ecriture des 80 octets");
}

static void test_realloc_grow_split(void) {
    printf("Test 30 : realloc plus grand sur place avec reste libre\n");
    unsigned char *a = malloc(32);
    unsigned char *b = malloc(128);
    unsigned char *c = malloc(16);
    CHECK(a && b && c, "3 allocations");
    if (!a || !b || !c) return;
    fill(a, 32, 0x12);
    fill(c, 16, 0x34);
    free(b);
    unsigned char *x = realloc(a, 64);
    CHECK(x == a, "32 -> 64 : meme pointeur");
    if (x) fill(x, 64, 0x56);
    unsigned char *y = malloc(64);
    CHECK(y != NULL, "le reste libre est reutilisable");
    if (y) fill(y, 64, 0x78);
    CHECK(x && is_filled(x, 64, 0x56), "bloc realloc intact");
    CHECK(is_filled(c, 16, 0x34), "bloc suivant intact");
}

static void test_realloc_grow_move(void) {
    printf("Test 31 : realloc plus grand avec deplacement\n");
    unsigned char *a = malloc(32);
    unsigned char *b = malloc(32);
    CHECK(a && b, "2 allocations");
    if (!a || !b) return;
    fill(a, 32, 0xA1);
    fill(b, 32, 0xB2);
    unsigned char *x = realloc(a, 96);
    CHECK(x != NULL, "retourne un pointeur");
    if (!x) return;
    CHECK(x != a, "voisin occupe : le bloc change d'adresse");
    CHECK(is_filled(x, 32, 0xA1), "contenu copie");
    fill(x, 96, 0xC3);
    CHECK(is_filled(b, 32, 0xB2), "voisin intact apres ecriture des 96 octets");
}

static void test_realloc_categories(void) {
    printf("Test 32 : changement de categorie TINY <-> SMALL <-> LARGE\n");
    unsigned char *p = malloc(100);
    CHECK(p != NULL, "TINY 100");
    if (!p) return;
    fill(p, 100, 0xD1);
    p = realloc(p, 1000);
    CHECK(p && is_filled(p, 100, 0xD1), "TINY -> SMALL : contenu conserve");
    if (!p) return;
    fill(p, 1000, 0xD2);
    p = realloc(p, 100000);
    CHECK(p && is_filled(p, 1000, 0xD2), "SMALL -> LARGE : contenu conserve");
    if (!p) return;
    fill(p, 100000, 0xD3);
    p = realloc(p, 2000);
    CHECK(p && is_filled(p, 2000, 0xD3), "LARGE -> SMALL : contenu conserve");
    if (!p) return;
    fill(p, 2000, 0xD4);
    p = realloc(p, 50);
    CHECK(p && is_filled(p, 50, 0xD4), "SMALL -> TINY : contenu conserve");
    free(p);
}

static void test_realloc_large(void) {
    printf("Test 33 : realloc LARGE\n");
    size_t s1 = 200000, s2 = 400000, s3 = 150000;
    unsigned char *p = malloc(s1);
    CHECK(p != NULL, "alloc LARGE");
    if (!p) return;
    fill(p, s1, 0xE1);
    CHECK(realloc(p, s1) == p, "meme taille : meme pointeur");
    unsigned char *q = realloc(p, s2);
    CHECK(q && is_filled(q, s1, 0xE1), "agrandi : contenu conserve");
    if (!q) return;
    fill(q, s2, 0xE2);
    unsigned char *r = realloc(q, s3);
    CHECK(r && is_filled(r, s3, 0xE2), "reduit : contenu conserve");
    free(r);
}

static void test_realloc_invalid(void) {
    printf("Test 34 : realloc sur pointeur invalide ou taille impossible\n");
    int local;
    void *volatile bad = &local;
    CHECK(realloc(bad, 32) == NULL, "adresse de pile -> NULL");
    unsigned char *p = malloc(64);
    CHECK(p != NULL, "allocation");
    if (!p) return;
    fill(p, 64, 0xF1);
    volatile size_t huge = SIZE_MAX;
    CHECK(realloc(p, huge) == NULL, "taille impossible -> NULL");
    CHECK(is_filled(p, 64, 0xF1), "ancien bloc intact apres l'echec");
    volatile size_t off = 16;
    CHECK(realloc(p + off, 32) == NULL, "p + 16 -> NULL");
    CHECK(is_filled(p, 64, 0xF1), "ancien bloc toujours intact");
    free(p);
}

static void test_realloc_stress(void) {
    printf("Test 35 : stress malloc/realloc/free melanges\n");
    enum { SLOTS = 256, OPS = 50000 };
    static unsigned char *ptrs[SLOTS];
    static size_t sizes[SLOTS];
    static unsigned char tags[SLOTS];
    srand(777);
    int ok = 1;
    for (int op = 0; op < OPS && ok; op++) {
        int i = rand() % SLOTS;
        int r = rand() % 100;
        size_t ns = r < 70 ? 1 + rand() % 128
                  : r < 95 ? 129 + rand() % 900
                  : 1025 + rand() % 20000;
        if (ptrs[i] && !is_filled(ptrs[i], sizes[i], tags[i])) { ok = 0; break; }
        if (!ptrs[i]) {
            ptrs[i] = malloc(ns);
        } else if (rand() % 4 == 0) {
            free(ptrs[i]);
            ptrs[i] = NULL;
            continue;
        } else {
            size_t keep = sizes[i] < ns ? sizes[i] : ns;
            unsigned char *q = realloc(ptrs[i], ns);
            if (!q || !is_filled(q, keep, tags[i])) { ok = 0; break; }
            ptrs[i] = q;
        }
        if (!ptrs[i] || (uintptr_t)ptrs[i] % 16) { ok = 0; break; }
        sizes[i] = ns;
        tags[i] = op & 0xFF;
        fill(ptrs[i], ns, tags[i]);
    }
    CHECK(ok, "50000 operations : contenu conserve, pointeurs alignes");
    for (int i = 0; i < SLOTS; i++) {
        if (!ptrs[i]) continue;
        if (!is_filled(ptrs[i], sizes[i], tags[i])) ok = 0;
        free(ptrs[i]);
    }
    CHECK(ok, "contenu intact avant le free final");
    CHECK(malloc(100) != NULL, "allocation possible apres tout liberer");
}

static void test_show_alloc_mem(void) {
    printf("Test 36 : show_alloc_mem (a comparer avec l'enonce)\n");
    unsigned char *a = malloc(42);
    unsigned char *b = malloc(84);
    unsigned char *c = malloc(3725);
    unsigned char *d = malloc(48847);
    if (!a || !b || !c || !d) {
        printf("  malloc a echoue\n");
        g_fail = 1;
        return;
    }
    fill(a, 42, 0xA1);
    fill(b, 84, 0xB2);
    fill(c, 3725, 0xC3);
    fill(d, 48847, 0xD4);

    printf("attendu : 42 et 84 (TINY), 3725 (SMALL), 48847 (LARGE), Total 52698\n\n");
    fflush(stdout);
    show_alloc_mem();

    if (!is_filled(a, 42, 0xA1) || !is_filled(b, 84, 0xB2)
        || !is_filled(c, 3725, 0xC3) || !is_filled(d, 48847, 0xD4)) {
        printf("  contenu corrompu : blocs qui se chevauchent\n");
        g_fail = 1;
    }

    free(a);
    free(b);
    free(c);
    free(d);

    printf("\nattendu apres free : zones TINY et SMALL sans bloc, plus de LARGE, Total 0\n\n");
    fflush(stdout);
    show_alloc_mem();
}

static const t_test g_tests[] = {
    {"100_per_zone", test_100_per_zone, 0},
    {"basic", test_basic, 0},
    {"alignment", test_alignment, 0},
    {"no_overlap", test_no_overlap, 0},
    {"zones", test_zones, 0},
    {"new_heap", test_new_heap, 0},
    {"large", test_large, 0},
    {"errors", test_errors, 0},
    {"zero", test_zero, 0},
    {"stress", test_stress, 0},
    {"free_null", test_free_null, 0},
    {"free_reuse", test_free_reuse, 0},
    {"double_free", test_double_free, 0},
    {"invalid_free", test_invalid_free, 0},
    {"merge_left", test_merge_left, 0},
    {"merge_right", test_merge_right, 0},
    {"merge_both", test_merge_both, 0},
    {"split", test_split, 0},
    {"loop", test_loop_stable, 0},
    {"free_orders", test_free_orders, 0},
    {"large_free", test_large_free, 1},
    {"large_own_zone", test_large_own_zone, 0},
    {"free_stress", test_free_stress, 0},
    {"realloc_null", test_realloc_null, 0},
    {"realloc_zero", test_realloc_zero, 0},
    {"realloc_same", test_realloc_same, 0},
    {"realloc_shrink", test_realloc_shrink, 0},
    {"realloc_shrink_nosplit", test_realloc_shrink_nosplit, 0},
    {"realloc_grow_inplace", test_realloc_grow_inplace, 0},
    {"realloc_grow_split", test_realloc_grow_split, 0},
    {"realloc_grow_move", test_realloc_grow_move, 0},
    {"realloc_categories", test_realloc_categories, 0},
    {"realloc_large", test_realloc_large, 0},
    {"realloc_invalid", test_realloc_invalid, 0},
    {"realloc_stress", test_realloc_stress, 0},
	{"show_alloc_mem", test_show_alloc_mem, 0},
};

static int run_test(const t_test *t) {
    fflush(stdout);
    pid_t pid = fork();
    if (pid < 0)
        return 1;
    if (pid == 0) {
        g_fail = 0;
        t->fn();
        fflush(stdout);
        _exit(g_fail != 0);
    }
    int st;
    waitpid(pid, &st, 0);
    if (WIFSIGNALED(st)) {
        if (t->expect_segv && WTERMSIG(st) == SIGSEGV) {
            printf("  [OK]   %s : segfault attendu apres munmap\n", t->name);
            return 0;
        }
        printf("  [FAIL] %s : tue par le signal %d\n", t->name, WTERMSIG(st));
        return 1;
    }
    return WEXITSTATUS(st) != 0;
}

int main(void) {
    static char outbuf[4096];
    setvbuf(stdout, outbuf, _IOLBF, sizeof(outbuf));

    int failed = 0;
    size_t n = sizeof(g_tests) / sizeof(*g_tests);
    for (size_t i = 0; i < n; i++)
        failed += run_test(&g_tests[i]);

    printf("\n%s (%d test(s) en echec sur %zu)\n",
           failed ? "ECHEC" : "TOUS LES TESTS PASSENT", failed, n);
    return failed != 0;
}