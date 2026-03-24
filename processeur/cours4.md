# Module 4 — La Hiérarchie Mémoire *(Memory Hierarchy)*

---

## Table des matières

1. [Le "Memory Wall"](#1-le-problème--le-memory-wall)
2. [La Pyramide des Caches](#2-la-pyramide-des-caches)
3. [Cache Hit vs Cache Miss](#3-mécanisme--cache-hit-vs-cache-miss)
4. [Le Principe de Localité](#4-le-principe-de-localité)
5. [Cas d'École : Parcours de Matrice](#5-cas-décole--parcours-de-matrice)

---

## 1. Le Problème : Le "Memory Wall"

Le **CPU** va des centaines de fois plus vite que la **RAM**. Pour éviter que le CPU passe sa vie à attendre ses données, on utilise une structure en pyramide appelée **hiérarchie mémoire**.

---

## 2. La Pyramide des Caches

> Du plus rapide au plus lent.

| Niveau | Emplacement | Capacité | Latence |
| :--- | :--- | :--- | ---: |
| **Registres** | Dans le cœur CPU | ~1 Ko | 0 cycle |
| **Cache L1** | Collé au cœur | 32 – 64 Ko | ~4 cycles |
| **Cache L2** | Proche du cœur | 256 – 512 Ko | ~12 cycles |
| **Cache L3** | Partagé (tous cœurs) | 8 – 64 Mo | ~40 cycles |
| **RAM** | Barrette externe | 16 – 64 Go | ~200+ cycles |

<!-- Schéma : pyramide des niveaux de cache (source à ajouter) -->

---

## 3. Mécanisme : Cache Hit vs Cache Miss

Le CPU ne communique jamais directement avec la RAM s'il peut l'éviter. À chaque accès mémoire, deux cas se présentent :

- **Cache Hit** — La donnée est trouvée dans l'un des caches. Le CPU continue à pleine vitesse.
- **Cache Miss** — La donnée est absente des caches. Le CPU doit **caler** (*Stall*) et attendre que la donnée remonte de la RAM.

---

## 4. Le Principe de Localité

Le succès d'un programme repose sur deux comportements que le cache exploite naturellement.

### A. Localité Temporelle — *Le "Déjà-vu"*

- **Concept** : une donnée récemment utilisée a de fortes chances d'être réutilisée très bientôt.
- **Exemples typiques** : compteurs de boucle (`i`, `j`), accumulateurs de somme.
- **Gestion** : le cache conserve la donnée au chaud. Si le cache est plein, l'algorithme **LRU** (*Least Recently Used*) évince la donnée la plus ancienne pour libérer de la place.

### B. Localité Spatiale — *Le "Voisinage" / Cache Line*

- **Concept** : si on accède à l'adresse `X`, on accédera probablement à `X+1`, `X+2`...
- **L'unité de transfert — la Cache Line** : le CPU ne déplace jamais moins de **64 octets** à la fois entre la RAM et le cache.
- **Exemple concret** : demander un `int` (4 octets) à `tab[0]` provoque le chargement des 64 octets suivants en cache L1. Les éléments `tab[0]` à `tab[15]` sont disponibles instantanément — les 15 suivants sont **"gratuits"**.

---

## 5. Cas d'École : Parcours de Matrice

Une matrice `int matrix[10000][10000]` est stockée de façon **contiguë** en RAM, **ligne par ligne** (*row-major order*).

### Parcours par ligne — Rapide

```c
for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++)
        sum += matrix[i][j]; // accès contigu en mémoire
```

On profite pleinement de la Cache Line : **1 Cache Miss pour 16 accès**.

### Parcours par colonne — Lent

```c
for (int j = 0; j < N; j++)
    for (int i = 0; i < N; i++)
        sum += matrix[i][j]; // saute 40 000 octets à chaque fois
```

On tombe systématiquement en dehors de la Cache Line courante : **100% de Cache Miss**.

> **Résultat** : le parcours par colonne peut être **10 à 50× plus lent** que le parcours par ligne, à code identique.

---