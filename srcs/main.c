#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

void *ft_malloc(size_t size);

static int g_fail = 0;

#define CHECK(cond, msg) do { \
    if (cond) printf("  [OK]   %s\n", msg); \
    else { printf("  [FAIL] %s (ligne %d)\n", msg, __LINE__); g_fail++; } \
} while (0)

static void test_100_per_zone(void) {
    printf("Test 1 : au moins 100 allocations dans une meme zone TINY\n");
    size_t page = sysconf(_SC_PAGESIZE);
    size_t zone = 4 * page;
    char *first = ft_malloc(128);
    CHECK(first != NULL, "premiere allocation");
    if (!first) return;
    int same_zone = 1;
    for (int i = 1; i < 100; i++) {
        char *p = ft_malloc(128);
        if (!p || p < first || (size_t)(p - first) >= zone)
            same_zone = 0;
    }
    CHECK(same_zone, "100 blocs de 128 octets tiennent dans 4 pages");
}

static void test_basic(void) {
    printf("Test 2 : allocation simple\n");
    char *p = ft_malloc(42);
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
        void *p = ft_malloc(s);
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
        ptrs[i] = ft_malloc(sizes[i]);
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
    void *t = ft_malloc(tiny_max);
    void *s = ft_malloc(tiny_max + 1);
    void *s2 = ft_malloc(small_max);
    void *l = ft_malloc(small_max + 1);
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
        char *p = ft_malloc(128);
        if (!p) { ok = 0; break; }
        memset(p, 0xAB, 128);
    }
    CHECK(ok, "1000 allocations TINY (plusieurs zones)");
}

static void test_large(void) {
    printf("Test 7 : LARGE\n");
    size_t sz = 5 * 1024 * 1024;
    unsigned char *p = ft_malloc(sz);
    CHECK(p != NULL, "5 Mo");
    if (!p) return;
    memset(p, 0x5A, sz);
    CHECK(p[0] == 0x5A && p[sz - 1] == 0x5A, "premier et dernier octet ecrits");
    CHECK((uintptr_t)p % 16 == 0, "aligne");
    void *p2 = ft_malloc(sz);
    CHECK(p2 != NULL && p2 != (void *)p, "deux LARGE distincts");
}

static void test_errors(void) {
    printf("Test 8 : tailles invalides\n");
    CHECK(ft_malloc(SIZE_MAX) == NULL, "SIZE_MAX -> NULL");
    CHECK(ft_malloc(SIZE_MAX - 1) == NULL, "SIZE_MAX - 1 -> NULL");
    CHECK(ft_malloc(SIZE_MAX - 4096) == NULL, "SIZE_MAX - 4096 -> NULL");
    CHECK(ft_malloc((size_t)1 << 62) == NULL, "2^62 -> NULL (mmap echoue)");
}

static void test_zero(void) {
    printf("Test 9 : malloc(0)\n");
    void *p = ft_malloc(0);
	CHECK(p == NULL, "allocation suivante fonctionne");
    void *q = ft_malloc(16);
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
        ptrs[i] = ft_malloc(sizes[i]);
        if (!ptrs[i] || (uintptr_t)ptrs[i] % 16) { ok = 0; break; }
        memset(ptrs[i], i & 0xFF, sizes[i]);
    }
    CHECK(ok, "allocations reussies et alignees");
    for (int i = 0; i < N && ok; i++)
        for (size_t j = 0; j < sizes[i]; j++)
            if (ptrs[i][j] != (unsigned char)(i & 0xFF)) { ok = 0; break; }
    CHECK(ok, "aucune corruption entre blocs");
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
    printf("\n%s (%d echec(s))\n", g_fail ? "ECHEC" : "TOUS LES TESTS PASSENT", g_fail);
    return g_fail != 0;
}