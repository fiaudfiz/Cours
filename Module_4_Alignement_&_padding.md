# M4 — Alignement & padding

---

## Table des Matières

1. [sizeof et offsetof](#1-sizeof-et-offsetof)
2. [`__attribute__((packed))`](#2-__attribute__packed)
3. [Impact sur le cache L1](#3-impact-sur-le-cache-l1)
4. [Impact sur la vectorisation AVX2](#4-impact-sur-la-vectorisation-avx2)
5. [Mesurer avec `pahole`](#5-mesurer-avec-pahole)
6. [Alternative sans `pahole`](#6-alternative-sans-pahole---wpadded)

---

Le processeur ne lit pas la RAM octet par octet. Il la lit par blocs alignés dont la taille dépend du type (4 octets pour un int, etc.). Pour rendre cette lecture possible en 1 seule opération, l'adresse de la donnée doit être un multiple de sa taille → Alignement.

Si l'adresse n'est pas alignée, 2 cas selon l'architecture :
* x86 : Le CPU fait 2 accès mémoire et recolle les morceaux (cela rend le programme plus lent bien sûr)
* ARM strict / certains RISC : **bus** error, le programme plante.

Le compilateur évite ce problème en insérant du padding : des octets de rembourrage invisibles dans les structs.

## 1. sizeof et offsetof

```c
#include <stddef.h>

struct Exemple {
    char  a;   /* 1 octet  */
    int   b;   /* 4 octets */
    char  c;   /* 1 octet  */
};
```

On pourrait croire que `sizeof(struct Exemple) == 6`. C'est faux.
```c
Offset 0 : a     (1 octet)
Offset 1 : [pad] (3 octets) ← le compilateur insère ici
Offset 4 : b     (4 octets)
Offset 8 : c     (1 octet)
Offset 9 : [pad] (3 octets) ← pour que la struct soit multiple de 4
```
`sizeof(struct Exemple) == 12`

Le padding de fin existe pour que si on fait un tableau de ces structs, chaque élément reste correctement aligné.

`offsetof(struct Exemple, b)` retourne 4, le décalage en octets depuis le début de la struct jusqu'au champ b. C'est un outil fondamental, défini dans `<stddef.h>`.
```c
printf("%zu\n", sizeof(struct Exemple));       /* 12 */
printf("%zu\n", offsetof(struct Exemple, b));  /* 4  */
printf("%zu\n", offsetof(struct Exemple, c));  /* 8  */
```

L'alignement d'une struct est celui de son membre le plus grand. La taille totale de la struct est un multiple de cet alignement.

## 2. `__attribute__((packed))`

GCC/Clang permettent de forcer une struct sans padding :
```c
struct __attribute__((packed)) PackedEx {
    char  a;
    int   b;
    char  c;
};
/* sizeof == 6, offsetof(b) == 1 */
```
`b` est maintenant à l'adresse `struct_base + 1`, qui n'est pas un multiple de 4.

Quand c'est utile :
* Parsing de protocole réseau ou de formats binaires où la disposition des octets est imposée (fichier BMP, paquets Ethernet, registres matériels)
* Sérialisation/désérialisation où on contrôle la lecture octet par octet

**Pourquoi c'est dangereux par défaut** :
* Accès non alignés → ralentissement sur x86, crash sur ARM
* Le compilateur ne peut plus vectoriser automatiquement les boucles sur ces données
* `memcpy` interne pour accéder aux membres dans certains cas

**Alternative propre** : utiliser `memcpy` explicitement pour extraire les champs d'un buffer brut, sans `packed` :
```c
uint32_t val;
memcpy(&val, buffer + offset, sizeof(val));
val = ntohl(val);  /* conversion endianness si réseau */
```

## 3. Impact sur le [cache L1](https://github.com/fiaudfiz/Cours/blob/main/processeur/cours4.md)

Le [**cache L1**](https://github.com/fiaudfiz/Cours/blob/main/processeur/cours4.md) travaille par **cache lines** de 64 octets sur x86-64 moderne. Quand on veut accéder à une adresse, le [CPU](https://github.com/fiaudfiz/Cours/blob/main/processeur/cours1.md) charge les 64 octets contigus autour de cette adresse, qu'on en ait besoin ou non.

### False sharing

Si 2 threads modifient des variables différentes qui se trouvent dans la même **cache line**, le protocole de cohérence cache (MESI) force une invalidation à chaque écriture. Résultat : les threads se battent sur une ligne de cache alors qu'ils ne partagent aucune donnée logiquement.
```c
/* Problème : x et y dans la même cache line */
struct {
    int x;  /* thread 1 écrit ici */
    int y;  /* thread 2 écrit ici */
} partagé;

/* Solution : forcer l'alignement sur une cache line */
struct {
    int x;
    char _pad[60];  /* ou __attribute__((aligned(64))) */
} thread1_data;

struct {
    int y;
} thread2_data;
```

### Accès séquentiel vs accès dispersé

Une struct qui tient dans 1 ou 2 cache lines est chargée en 1 ou 2 accès. Une struct avec beaucoup de padding ou de pointeurs vers d'autres zones mémoire génère des **caches misses** en cascade (pointer chasing). C'est la différence entre un tableau de structs compacts (cache-friendly) et un tableau de pointeurs vers des structs éparpillées en heap.

## 4. Impact sur la vectorisation AVX2

AVX2 opère sur des [registres](https://github.com/fiaudfiz/Cours/blob/main/processeur/cours4.md) 256 bits. Pour qu'une boucle soit auto-vectorisée, le compilateur doit être capable de charger plusieurs éléments contigus en une instruction.
```c
/* Vectorisable : floats contigus, pas de padding */
float tab[256];
for (int i = 0; i < 256; i++) tab[i] *= 2.0f;
/* → 8 floats traités par instruction AVX2 */

/* Difficilement vectorisable : stride non unitaire à cause du padding */
struct Point { float x; float y; char flag; /* pad 3 */ };
Point pts[256];
for (int i = 0; i < 256; i++) pts[i].x *= 2.0f;
/* x n'est pas contiguë en mémoire — stride de 12 octets */
```

La solution classique en calcul intensif est le SoA (Structure of Arrays) plutôt que AoS (Array of Structures) :

```c
/* AoS — mauvais pour la vectorisation */
struct Particule { float x, y, z, masse; };
Particule particules[N];

/* SoA — bon pour la vectorisation */
struct {
    float x[N];
    float y[N];
    float z[N];
    float masse[N];
} particules;
```

En SoA, quand on traite tous les `x`, ils sont contigus en mémoire, AVX2 charge 8 floats d'un coup et applique l'opération sur les 8 simultanément.

Pour garantir l'alignement des buffers utilisés avec AVX2 :

```c
float *buf = aligned_alloc(32, N * sizeof(float));  /* aligné sur 32 octets */
/* ou */
float buf[N] __attribute__((aligned(32)));
```

## 5. Mesurer avec `pahole`

`pahole` est un outil qui lit les informations de debug DWARF dans le binaire et affiche la disposition **réelle** des structs, avec padding explicitement indiqué.

### Compilation (debug info obligatoire)

```bash
gcc -g -o mon_programme main.c
```

### Usage de base

```bash
pahole mon_programme
```

Sortie typique :
```c
struct Mal {
    char    a;         /*  0     1 */
    /* 7 bytes hole */ /*  1     7 */
    double  b;         /*  8     8 */
    char    c;         /* 16     1 */
    /* 3 bytes hole */ /* 17     3 */
    int     d;         /* 20     4 */
    /* size: 24, cachelines: 1, members: 4 */
    /* sum members: 14, holes: 2, sum holes: 10 */
    /* last cacheline: 24 bytes */
};
```
Chaque trou est rendu visible avec sa taille. La dernière ligne donne le bilan : 14 octets utiles, 10 octets gaspillés.

### Options utiles

```bash
pahole --show_paddings mon_programme   /* filtrer uniquement les structs avec padding */
pahole -C Mal mon_programme           /* afficher uniquement la struct 'Mal' */
pahole --reorganize mon_programme     /* suggérer un réordonnancement optimal */
```

`--reorganize` est particulièrement utile : il sort la version réorganisée de la struct sans qu'on ait à le faire à la main.

## 6. Alternative sans `pahole` : `-Wpadded`

GCC et Clang ont un warning qui signale le padding à la compilation :
```makefile
CFLAGS += -Wpadded
```
Il signale même les cas intentionnels, mais c'est un outil très utile.