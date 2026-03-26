# Module 3 sur le processeur — Les Aléas du Pipeline *(Pipeline Hazards)*

---

## Table des matières

1. [Concepts Clés](#1-concepts-clés)
2. [Les Ennemis](#2-les-3-types-daléas--les-ennemis)
3. [Les Solutions](#3-les-solutions--comment-supprimer-les-stalls)

---

## 1. Concepts Clés

- **Pipeline** : l'autoroute du CPU. Technique qui permet de lancer une instruction avant que la précédente ne soit terminée — c'est du parallélisme au niveau des instructions.
- **Stall** *(ou "Bulle")* : un cycle perdu où le pipeline se met en pause et le CPU ne produit rien.

---

## 2. Les 3 Types d'Aléas — *Les Ennemis*

### A. L'Aléa Structurel *(Resource Hazard)*

**Problème** : une "bagarre" de matériel. Deux instructions tentent d'utiliser le même composant du processeur au même moment (ex. : une instruction veut lire en mémoire pendant qu'une autre veut y écrire).

**Solution hardware** : l'**Architecture de Harvard** (modifiée).

On sépare physiquement les caches en deux :

| Cache | Rôle |
| :--- | :--- |
| **L1 Instruction** | Réservé au code |
| **L1 Data** | Réservé aux variables |

Résultat : on peut charger du code et lire une variable **simultanément**, sans bouchon.

---

### B. L'Aléa de Données *(Data Hazard)* — Le plus courant

**Problème** : une dépendance. L'instruction B a besoin du résultat de A, mais A n'a pas encore fini de calculer.

```c
int a = 1 + 1;
int b = a * 2;  // B doit attendre que A ait écrit la valeur de a
```

**Conséquence** : le CPU insère des **Bulles** (*Stalls*) — il gèle l'instruction B pendant 1 ou 2 cycles le temps que A se termine.

---

### C. L'Aléa de Contrôle *(Control Hazard)* — Le plus coûteux

**Problème** : l'incertitude du `if`. Le CPU charge les instructions à la suite, mais face à un saut conditionnel (`if`) ou inconditionnel (`goto`), il ne sait pas où aller tant que la condition n'est pas évaluée.

**Mécanisme** : le **Branch Predictor** — le parieur. Il devine le chemin probable et continue d'exécuter de manière **spéculative**.

**Conséquence en cas d'erreur** : le **Pipeline Flush**.

> Le CPU jette à la poubelle toutes les instructions chargées par erreur. C'est une perte énorme de cycles — c'est la **pénalité de mauvaise prédiction**.

---

## 3. Les Solutions — *Comment supprimer les Stalls*

### A. Le Forwarding *(ou Bypassing)*

**Problème ciblé** : l'Aléa de Données.

**Principe** : au lieu d'attendre que le résultat soit officiellement écrit dans le registre (étape *Write-Back*, en fin de pipeline), on le récupère **dès qu'il sort de l'UAL**.

Un câble spécialisé — la **Forwarding Unit** *(ou Multiplexeur de Forwarding)* — court-circuite le chemin normal pour relier directement :

```
Sortie de l'étage EX (instruction A)  →  Entrée de l'étage EX (instruction B)
```

---

### B. L'Exécution dans le Désordre *(Out-of-Order Execution — OoO)*

**Problème ciblé** : tous les blocages de longue durée (ex. : attendre la RAM).

**Principe** : le CPU dispose d'une **fenêtre d'instructions**. Si l'instruction courante est bloquée, il regarde plus loin dans le code pour trouver des instructions **indépendantes** — celles qui n'ont pas besoin des résultats en attente — et les exécute immédiatement.

> **Analogie** : à la caisse du supermarché, si le client devant toi a oublié de peser ses bananes, la caissière ne s'arrête pas — elle scanne les articles du client suivant en attendant.

---