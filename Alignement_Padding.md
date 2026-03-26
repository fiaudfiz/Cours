# Alignement et Padding

Ce cours sera complete par 2 autres cours (un cours sur les tailles des types et l'autre qui parle un peu de padding).

---

## Tables des Matieres


---

Le processeur ne lit pas la RAM octet par octet.Il la lit par bloc aligne dont la taille depend du type (4 octet pour un int etc).Pour rendre cette lecture possible en 1 seule operation, l'adresse de la donnee doit etre un multiple de sa taille --> Alignement.

Si l'adresse n'est pas alignee, 2 cas selon l'architecture :
* x86 : Le CPU fait 2 acces memoire et recolle les morceaux (cela rend le programe plus lent bien sur)
* ARM strict / certains RISC : **bus** error, le programme plante.

Le compilateur evite ce probleme en inserant du padding : des octets de rembourrage invisibles dans les structs.

## sizeof et offsetof

```c
#include <stddef.h>

struct Exemple {
    char  a;   // 1 octet
    int   b;   // 4 octets
    char  c;   // 1 octet
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
Le padding de fin existe pour que si on fait un tableay de ces structs, chaque element reste correctement aligne.

`offsetof(struct Exemple, b)` retourne 4, le decalage en octet depuis le debut de la struct jusqu'au champ b.C'est un outil fondamental, defini dans `<stddef.h>`.
```
printf("%zu\n", sizeof(struct Exemple));       // 12
printf("%zu\n", offsetof(struct Exemple, b));  // 4
printf("%zu\n", offsetof(struct Exemple, c));  // 8
```

L'alignement d'une struct est celui de son membre le plus important.La taille totale de la struct est un multiple de cet alignement.

## `__attribute__((packed))`

GCC/Clang permettent de forcer une struct sans padding :
```c
struct __attribute__((packed)) PackedEx {
    char  a;
    int   b;
    char  c;
};
// sizeof == 6, offsetof(b) == 1
```
`b` est maintenant a l'adresse `struct_base + 1`, qui n'est pas un multiple de 4.

Quand c'est utile :
* parsing de protocole reseau ou de formats binaires ou la disposition des octets est imposee (fichier BMPm paquets Ethernet, registres materiels)
* Serialisation/deserialisation ou on controle la lecture octet par octet

**Pourquoi c'est dangereux par defaut**:
* Acces non alignes -> ralentissement sur x86, crash sur ARM
* le compilateur ne peut plus vectoriser automatiquement les boucles sur ces donnees
* `memcpy` interne pour acceder aux membres dans certains cas

**Alternative propre** : utiliser `memcpy` explicitement pour extraire les champs d'un buffer brut, sans `packed` :
```c
uint32_t val;
memcpy(&val, buffer + offset, sizeof(val));
val = ntohl(val);  // conversion endianness si réseau
```

## Impact sur le cache L1

Le **cache L1** travaille par **cache lines** de 64 octets sur x86-64 moderne.Quand on veut acceder a une adresse, le CPU charge les 64 octets contigus autour de cette adresse, que on en aie besoin ou non.

### False sharing

Si 2 threads modifient des variables differentes qui se trouvent dans le meme **cache line**, le protocole de coherence cache (MESI) force une invalidation a chaque ecriture.Resultat : les threads se battent sur une ligne de cache alors qu'ils ne partagent aucune donnee logiquement.
```c
// Problème : x et y dans la même cache line
struct {
    int x;  // thread 1 écrit ici
    int y;  // thread 2 écrit ici
} partagé;

// Solution : forcer l'alignement sur une cache line
struct {
    int x;
    char _pad[60];  // ou __attribute__((aligned(64)))
} thread1_data;

struct {
    int y;
} thread2_data;
```

### Acces sequentiel vs acces disperse

Une struct qui tient dans 1 ou 2 cache lines est chargee en 1 ou 2 acces.Une struct avec beaucoup de padding ou de pointeurs vers d'autres zones memoire genere des **caches misses** en cascade (pointer chasing).C'est la difference entre un tableau de struct compacts (cache-friendly) et un tableau de pointeurs vers des structs eparpillees en heap.

## Impact sur la vectorisation AVX2

AVX2 opere sur des registres 256 bits. Pour qu'une boucle soit auto-vectorisee, le compilateur doit etre capable de charger plusieurs elements contigus en une instruction.
```c
// Vectorisable : float contigus, pas de padding
float tab[256];
for (int i = 0; i < 256; i++) tab[i] *= 2.0f;
// → 8 floats traités par instruction AVX2

// Difficilement vectorisable : stride non unitaire à cause du padding
struct Point { float x; float y; char flag; /* pad 3 */ };
Point pts[256];
for (int i = 0; i < 256; i++) pts[i].x *= 2.0f;
// x n'est pas contiguë en mémoire — stride de 12 octets
```

La Solution Classique en calcul intensif est le SoA (Structure of Arrays) plutot que AoS (Arrays of Structure)

```c
// AoS — mauvais pour la vectorisation
struct Particule { float x, y, z, masse; };
Particule particules[N];

// SoA — bon pour la vectorisation
struct {
    float x[N];
    float y[N];
    float z[N];
    float masse[N];
} particules;
```

En SoA, quand on traite tous les `x`, ils sont contigus en memoire, AVX2 charge 8 floats d'un coup et applique l'operation sur les 8 simultanement.

Pour garantir l'alignement des buffers utilises avec AVX2 :

```
float *buf = aligned_alloc(32, N * sizeof(float));  // aligné sur 32 octets
// ou
float buf[N] __attribute__((aligned(32)));
```

## Mesurer avec `pahole`

`pahole` est un outil qui lit les informations de debug DWARF dans le binaire et affiche la disposition **reelle** des structs, avec padding explicitement indique.

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
Chaque trou est rendu visible avec sa taille.La derniere ligne donne le bilan : 14 octets utiles, 10 octets gaspilles.

### Options utiles

```bash
pahole --show_paddings mon_programme       # filtrer uniquement les structs avec padding
pahole -C Mal mon_programme               # afficher uniquement la struct 'Mal'
pahole --reorganize mon_programme         # suggérer un réordonnancement optimal
```

`--reorganize` est particulierement utile : il sort la version reorganisee de la struct sans que on aie a le faire a la main.

## Alternative sans `pahole` : `-Wpadded`

GCC et Clang ont un warning qui signale le padding a la compilation :
```makefile
CFLAGS += -Wpadded
```
Il signale meme les cas intentionnels, mais c'est un outil super utile.