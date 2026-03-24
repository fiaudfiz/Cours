# Module 5 — Le Parallélisme de Données *(SIMD)*

---

## Table des matières

1. [Concepts Fondamentaux](#1-concepts-fondamentaux)
2. [Architcture des Registres Vectoriels](#2-architecture-des-registres-vectoriels)
3. [Le Jeu d'Instruction](#3-le-jeu-dinstructions-isa-extensions)
4. [Implementation en C](#4-implémentation-en-c--deux-approches)
5. [Le Problème de la Divergence](#5-le-problème-de-la-divergence)

---

## 1. Concepts Fondamentaux

### A. La Taxonomie de Flynn

Système de classification des architectures d'ordinateurs, inventé par Michael Flynn en 1966. Il classe les machines selon deux axes : le **nombre de flux d'instructions** et le **nombre de flux de données**.

| Modèle | Nom | Description |
| :--- | :--- | :--- |
| **SISD** | *Single Instruction, Single Data* | Modèle séquentiel classique. Une instruction modifie une seule donnée à la fois. Ex. : `c = a + b`. |
| **SIMD** | *Single Instruction, Multiple Data* | Modèle vectoriel. Une seule instruction (le "chef d'orchestre") modifie plusieurs données simultanément (l'orchestre). |

### B. Scalaire vs Vectoriel

- **Code scalaire** : code C standard. On manipule des variables unitaires (`int`, `float`, `double`). Une variable = **une valeur**.
- **Code vectoriel** : on manipule des "vecteurs" — de petits tableaux de taille fixe qui rentrent pile dans un registre processeur. Une variable = **un paquet de valeurs** (ex. : 4 `double` d'un coup). À ne pas confondre avec `std::vector` du C++.

---

## 2. Architecture des Registres Vectoriels

Ces registres se trouvent physiquement à l'intérieur du cœur du CPU, juste à côté de l'UAL. C'est la mémoire la plus rapide qui existe — plus rapide que le Cache L1.

Pour supporter le SIMD, le processeur dispose de registres dédiés, bien plus larges que les registres généraux (`RAX`, `RBX` — 64 bits) :

| Registre | Extension | Taille |
| :--- | :--- | :--- |
| **XMM** | SSE | 128 bits |
| **YMM** | AVX / AVX2 | 256 bits |
| **ZMM** | AVX-512 | 512 bits *(surtout sur serveurs)* |

**Capacité d'un registre YMM (256 bits = 32 octets) :**

| Type | Taille unitaire | Valeurs simultanées |
| :--- | :---: | :---: |
| `double` | 64 bits | **4** |
| `float` / `int` | 32 bits | **8** |
| `char` | 8 bits | **32** |

---

## 3. Le Jeu d'Instructions *(ISA Extensions)*

Le passage au code vectoriel requiert que le CPU comprenne de nouvelles instructions assembleur.

- **SSE** *(Legacy)* : utilise les registres `XMM`. Ex. : `addps` *(Add Packed Single-precision)*.
- **AVX / AVX2** *(Moderne)* : utilise les registres `YMM`. Les instructions adoptent le format **VEX** (commencent souvent par `v`).

```asm
vaddpd   ; Vector Add Packed Double-Precision
         ; → Additionne des paquets de flottants double précision
```

---

## 4. Implémentation en C — Deux Approches

### A. Les Intrinsics *(Bas niveau)*

Fonctions C fournies par `<immintrin.h>`, servant de pont direct vers l'assembleur. Correspondance **1 pour 1** avec une instruction assembleur.

```c
_mm256_add_pd(a, b);  // → génère exactement vaddpd dans le binaire
```

> **Inconvénient — Non portable** : ces fonctions sont liées à l'architecture x86 (Intel/AMD). Compiler ce code pour un Apple M1/M2/M3 (ARM) échouera — ARM utilise son propre jeu d'instructions vectorielles (**NEON**).

### B. Vector Extensions *(Abstraction compilateur)*

Approche plus moderne et lisible, proposée par GCC/Clang.

```c
typedef double v4d __attribute__((vector_size(32)));
```

- **Surcharge d'opérateur** : le compilateur comprend que `+` entre deux `v4d` doit générer `vaddpd` pour additionner les 4 éléments simultanément.
- **Auto-vectorisation** : avec les flags `-O3 -mavx2`, le compilateur peut transformer automatiquement une boucle `for` en une seule instruction SIMD.

---

## 5. Le Problème de la Divergence

En code scalaire (**SISD**), le processeur peut prendre des chemins différents pour chaque donnée (via des `if` / `else`).

En **SIMD**, les données sont "liées" dans le registre — elles subissent la même instruction au même moment.

**Exemple — Projet [Fract-ol](https://github.com/fiaudfiz/Fract-ol) :**

On calcule 4 pixels. Le pixel #1 a terminé (sorti du cercle), mais les pixels #2, #3, #4 doivent continuer. Le processeur ne peut pas stopper le calcul pour le #1 seul.

### La Solution : Le Masquage *(Masking)*

1. On compare tout le monde : `vcmppd`.
2. Cela génère un **masque** — un vecteur binaire Vrai/Faux :

| Pixel | État | Masque |
| :--- | :--- | :--- |
| #1 | Terminé | `0x000...` *(tout à zéro)* |
| #2, #3, #4 | Continue | `0xFFF...` *(tout à un)* |

3. On applique des opérations logiques (`AND`, `OR`) pour filtrer : les calculs continuent pour tout le monde, mais les résultats ne sont enregistrés **que là où le masque vaut `1`**.

---

## 6. Contraintes de Performance

### A. Alignement Mémoire

Le CPU charge les données par blocs de 128 ou 256 bits.

| Cas | Description | Instruction |
| :--- | :--- | :--- |
| **Aligné** | L'adresse est un multiple de 32 (AVX). Le CPU charge en une seule traite. | `vmovaps` |
| **Non-aligné** | L'adresse est "bancale". Le bloc peut chevaucher deux Cache Lines — le CPU doit faire deux lectures et recoller les morceaux. | `vmovups` |

### B. Latence vs Débit *(Throughput)*

- **Latence** : le temps qu'il faut à une voiture pour traverser l'autoroute.
- **Débit** : le nombre de voitures qui passent en une heure.

> Le SIMD n'améliore **pas** la latence — `a + b` ne va pas plus vite.
> Le SIMD explose le **débit** — on effectue 4 calculs `a + b` dans le même laps de temps.

---