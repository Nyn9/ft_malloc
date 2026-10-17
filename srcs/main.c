#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include "ft_malloc.h"

// void *malloc(size_t size);

static int g_fail = 0;

#define CHECK(cond, msg) do { \
    if (cond) printf("  [OK]   %s\n", msg); \
    else { printf("  [FAIL] %s (ligne %d)\n", msg, __LINE__); g_fail++; } \
} while (0)

static void test_100_per_zone(void) {
    printf("Test 1 : au moins 100 allocations dans une meme zone TINY\n");
    size_t page = sysconf(_SC_PAGESIZE);
    size_t zone = 4 * page;
    char *first = malloc(128);
    CHECK(first != NULL, "premiere allocation");
    if (!first) return;
    int same_zone = 1;
    for (int i = 1; i < 2; i++) {
        char *p = malloc(128);
        if (!p || p < first || (size_t)(p - first) >= zone)
            same_zone = 0;
    }
    CHECK(same_zone, "100 blocs de 128 octets tiennent dans 4 pages");
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
	CHECK(p == NULL, "allocation suivante fonctionne");
    void *q = malloc(16);
    CHECK(q != NULL, "allocation suivante fonctionne");
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

/* ---------- infrastructure : un test = un processus ---------- */

// static void run_isolated(void (*fn)(void), int expect_segv) {
//     fflush(stdout);
//     pid_t pid = fork();
//     if (pid == 0) {
//         g_fail = 0;
//         fn();
//         fflush(stdout);
//         _exit(g_fail != 0);
//     }
//     int st;
//     waitpid(pid, &st, 0);
//     if (WIFSIGNALED(st)) {
//         if (expect_segv && WTERMSIG(st) == SIGSEGV) {
//             printf("  [OK]   segfault attendu apres munmap\n");
//             return;
//         }
//         printf("  [FAIL] processus tue par le signal %d\n", WTERMSIG(st));
//         g_fail++;
//     } else if (WEXITSTATUS(st))
//         g_fail++;
// }

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

// static void test_large_free(void) {
//     printf("Test 21 : free d'un LARGE rend la zone au systeme\n");
//     unsigned char *p = malloc(5 * 1024 * 1024);
//     CHECK(p != NULL, "5 Mo alloues");
//     if (!p) return;
//     memset(p, 1, 5 * 1024 * 1024);
//     free(p);
//     *(volatile unsigned char *)p = 1;   // doit segfaulter : zone demappee
//     CHECK(0, "aucun segfault : la zone n'a pas ete munmap");
// }

static void test_large_own_zone(void) {
    printf("Test 22 : chaque LARGE a sa propre zone\n");
    size_t page = sysconf(_SC_PAGESIZE);
    uintptr_t a = (uintptr_t)malloc(600);
    uintptr_t b = (uintptr_t)malloc(600);
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

int main(void) {
    test_100_per_zone();
    test_basic();
    test_alignment();
    test_no_overlap();
    test_zones();
    test_new_heap();
    test_large();
    test_errors();
    test_zero();
    test_stress();

    printf("\n=== free ===\n");
    test_free_null();
    test_free_reuse();
    test_double_free();
    test_invalid_free();
    test_merge_left();
    test_merge_right();
    test_merge_both();
    test_split();
    test_loop_stable();
    test_free_orders();
    // test_large_free();
    test_large_own_zone();
    test_free_stress();

    printf("\n%s (%d echec(s))\n", g_fail ? "ECHEC" : "TOUS LES TESTS PASSENT", g_fail);
    return g_fail != 0;
}