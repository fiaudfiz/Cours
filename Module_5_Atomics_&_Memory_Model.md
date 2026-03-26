# M5 — Atomics & Memory Model (C11 / vue C99)

 **ce cours est trop dur, il faudra le lire apres le M30.**

---

## Table des matieres

---

On connait les **threads**, on connait les **race condition**, ici nous allons voir comment un seul store peut etre vu dans le mauvais ordre par un autre **thread**, et pas a cause d'un bug present dans le code, a cause du compilateur et du **CPU** et c'est le sujet de ce cours.

## 3 niveaux de reordonnancement

Exemple :
```c
int data  = 0;
int ready = 0;

// Thread A
data  = 42;
ready = 1;

// Thread B
while (!ready);
printf("%d\n", data); // ici on attend 42. On a pas forcement 42.
```

Logiquement c'est bon. En pratique ça peut imprimer `0`. Pourquoi ?

### **Niveau 1 — le compilateur.** 

`data` et `ready` sont deux variables indépendantes à ses yeux. À `-O2` il peut les réordonner si c'est plus efficace. Thread B voit `ready = 1` avant que `data = 42` soit écrit.

### **Niveau 2 — le CPU.** 

Les écritures ne vont pas directement en RAM. Elles passent par un **store buffer** propre à chaque cœur. Core 0 a écrit `data = 42` dans son buffer mais pas encore vidé en RAM. Core 1 lit RAM directement et voit l'ancienne valeur.
```c
Core 0  [store buffer: data=42]  →  RAM: data=0  ←  Core 1 lit ici
```

### **Niveau 3 — le cache.**

Comme vu dans les cours sur le **processeur**, chaque coeur a son cache **L1/L2**.Le protocole MESI propage les modifications entre coeurs, mais pas instantanement.Une ecriture sur Core 0 met un moment a etre visile sur Core 1.

---

Ces 3 niveaux sont independants.Bloquer le compilateur ne bloque pas le CPU.Il faut adresser les 3 en meme temps, c'est exactement ce que les atomics et les barrieres font.

## C99 : `__sync_*`

Ceci n'est pas dans le standart C99, c'est du GCC/Clang, mais c'est ce que on utilise lorque que on n'utilise pas C11.

```c
// Incrément atomique
__sync_fetch_and_add(&compteur, 1); // retourne l'ancienne valeur
__sync_add_and_fetch(&compteur, 1); // retourne la nouvelle

// Compare-And-Swap
// si *ptr == old → écrit new, retourne 1
// sinon          → retourne 0
__sync_bool_compare_and_swap(&ptr, old, new);

// Full memory fence — bloque compilateur ET CPU
__sync_synchronize();
```

le `__sync_synchronize()` , c'est une barriere memoire totale.Tout ce qui est ecrit avant est garanti visible par tous les coeurs avant que quoi ce que soit apres commence.

```c
// Thread A
data = 42;
__sync_synchronize();
ready = 1;

// Thread B
while (!ready);
__sync_synchronize();
printf("%d\n", data); // garanti : 42
```

La limite : c'est barriere totale ou rien.Pas de granularite, pas de controle fin.

## C11 : `stdatomic.h`

C11 Standardise tout ca. On declare une variable `_Atomic` et le compiateur garantit que toutes les operations dessus sont atomiques et correctement visibles.

```c
#include <stdatomic.h>

atomic_int compteur = 0;      // _Atomic int compteur = 0; — même chose

atomic_store(&compteur, 0);         // écriture atomique
int v = atomic_load(&compteur);     // lecture atomique
atomic_fetch_add(&compteur, 1);     // read-modify-write atomique
```

Sans preciser de memory order, tout tourne en `seq_cst` par defaut, le plus fort, le plus sur mais aussi le plus lent.

## Memory Orders, le vrai controle

C'est ici que on remarque que C11 devient serieux, chaque operation atomique accepte un memory order : on dit exactement quelles garanties on veut et on paie le prix en temps mais juste ce que on a demande.
```c
atomic_store_explicit(&x, 1, memory_order_release);
int v = atomic_load_explicit(&x, memory_order_acquire);
```

`memory_order_relaxed` -> Atomicite uniquement, aucune barriere generee, aucune garantie sur l'ordre de visibilite des autres ecritures.

```c
// Compteur de stats — l'ordre n'a aucune importance
atomic_fetch_add_explicit(&hits, 1, memory_order_relaxed);
```

Le plus rapide, a utiliser uniquement quand les variables autour sont independantes.

`memory_order_release` / `memory order_acquire` : c'est le duo central, le pattern/producteur/consommateur sans mutex.
```c
// Thread A — producteur
data = 42;
atomic_store_explicit(&ready, 1, memory_order_release);
// RELEASE : tout ce qui précède ce store est garanti visible
//           avant que ce store soit visible par quiconque
// Thread B — consommateur
while (!atomic_load_explicit(&ready, memory_order_acquire));
// ACQUIRE : une fois que je vois le RELEASE,
//           je vois aussi tout ce qui le précédait
printf("%d\n", data); // garanti : 42
```

Mécaniquement : `release` interdit au compilateur et au CPU de remonter des stores après ce point. `acquire` interdit de descendre des loads avant ce point. Ensemble ils forment un canal de synchronisation directionnel.

### `memory_order_seq_cst`

Ordre séquentiel total. Tous les threads voient toutes les opérations `seq_cst` dans le même ordre global. Génère une vraie barrière hardware — `dmb` sur ARM, `mfence` sur x86.

C'est le défaut implicite quand tu n'utilises pas `_explicit`. Le plus simple à raisonner, le plus coûteux.

### Récap

| Memory order | Barrière compilo | Barrière CPU | Usage |
|---|---|---|---|
| `relaxed` | ✗ | ✗ | Compteurs indépendants |
| `release` / `acquire` | Partielle | Partielle | Producer / consumer |
| `seq_cst` | Totale | Totale | Défaut, synchro générale |

---

## 5. CAS — la brique de tout algorithme lockfree

Le **Compare-And-Swap** c'est une instruction atomique hardware qui fait en une seule opération non-interruptible :
```c
si *ptr == expected → écrit desired, retourne true
sinon              → met expected à jour, retourne false
atomic_int val = 0;
int expected   = 0;

bool ok = atomic_compare_exchange_strong(&val, &expected, 1);
// val == 0 → val = 1,              ok = true
// val != 0 → expected = val actuel, ok = false
```

le pattern naturel, C'est la boucle CAS,tu retry jusqu'a reussir :
```c
int old, new;
do {
    old = atomic_load_explicit(&compteur, memory_order_relaxed);
    new = old + 1;
} while (!atomic_compare_exchange_weak(&compteur, &old, new));
```
`weak` vs `strong` : `weak` peut échouer spurieusement même si `*ptr == expected`  mais il est plus rapide sur ARM car il mappe directement sur `ldrex/strex`. En boucle tu prends toujours `weak`. En one-shot tu prends `strong`.
C'est exactement ce que `atomic_fetch_add` fait sous le capot sur les archs sans instruction d'incrément atomique native.

---

