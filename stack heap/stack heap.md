# Stack, Heap & Gestion Mémoire Avancée

---

## 1. La Stack — Le Domaine du CPU

La Stack est une zone de mémoire **automatique**, gérée directement par le processeur via un registre dédié : le **Stack Pointer** (`rsp` sur x86_64).

- **Coût d'un appel de fonction** : à chaque appel, le CPU effectue un *Push* sur la stack — il y dépose les arguments, l'adresse de retour, et les variables locales. Une récursion infinie remplit la stack jusqu'à débordement : c'est le **Stack Overflow**.
- **Localité du cache** : la Stack est petite et très fréquemment accédée — elle reste presque toujours dans le **[Cache L1](https://github.com/fiaudfiz/Cours/blob/main/processeur/cours4.md)**. C'est pourquoi accéder à une variable locale est infiniment plus rapide qu'accéder à un `malloc`.
- **Durée de vie** *(Scope)* : dès la sortie des accolades `}`, le Stack Pointer remonte. La donnée est techniquement encore présente mais considérée comme **morte**. Ne jamais retourner l'adresse d'une variable locale.

---

## 2. La Heap — Le Domaine de l'OS

La Heap est un espace géré par le **Kernel**. Un `malloc` ne parle pas au CPU — il parle à la librairie standard C, qui demande de la mémoire à l'OS via les appels système `brk` ou `mmap`.

- **Fragmentation externe** : On alloues 10 blocs de 10 octets, On libères les blocs pairs. On as 50 octets libres, mais on ne peut pas allouer un bloc de 20 octets car la mémoire libre n'est pas contiguë. Limiter les allocations/libérations frénétiques permet d'éviter ça.
- **Overhead de `malloc`** : chaque bloc alloué consomme plus que ce qui a été demandé. `malloc` stocke des **méta-données** (taille du bloc, état du bloc suivant...) juste avant l'adresse retournée. Un `malloc(1)` peut en réalité consommer **16 à 32 octets**.

---

## 3. Le "Hors-Programme" — La Différence entre les Bons et les Cracks

### A. La Gestion des Pages Mémoire

Le CPU et l'OS ne voient pas la mémoire comme une suite d'octets, mais comme des **Pages** (généralement 4 Ko).

- La MMU traduit chaque adresse virtuelle en adresse physique.
- **Page Fault** : accéder à une zone de la heap non encore utilisée suspend l'exécution — l'OS cherche une page physique, l'associe au programme, puis reprend.

> Un expert sait qu'un `memset` sur un grand `malloc` force la création immédiate de toutes les pages — utile pour **optimiser la latence** en production.

### B. L'[Alignement](memory alignement) et le Padding *(Data Alignment)*

Le processeur lit les données par blocs de 4 ou 8 octets. Le compilateur insère du **padding invisible** pour respecter cet alignement.

```c
struct s_data {
    char a;    // 1 octet
               // 3 octets de padding ajoutés par le compilateur
    int  b;    // 4 octets
};
// Taille réelle : 8 octets, pas 5
```

Avec 1 million de telles structures en liste chaînée, **3 Mo de RAM sont gâchés**. Un expert réorganise ses variables **par taille décroissante** pour minimiser le padding.

### C. Cache Miss et Listes Chaînées — *Le Tueur de Performance*

| Structure | Comportement CPU | Performance |
| :--- | :--- | :--- |
| **Tableau** | Éléments contigus en mémoire. Le CPU anticipe et pré-charge les suivants (*Prefetching*). | Rapide |
| **Liste chaînée** | Le nœud suivant peut être n'importe où. Le CPU doit s'arrêter et aller chercher en RAM à chaque itération. | Lent |

> **Solution de crack — Block-linked list** : chaque nœud contient un petit tableau de 16 éléments au lieu d'un seul. On cumule la rapidité d'insertion de la liste *et* la rapidité de lecture du cache.

---

## 4. Stratégie de Développement

- **Privilégier la Stack** : si la taille est connue et raisonnable (ex. : buffer de 4096 octets), il faut utiliser la Stack. C'est "gratuit" en termes de performance.
- **Mallocer intelligemment** : au lieu de 100 `malloc` pour 100 nœuds, il faut faire un seul `malloc(sizeof(node) * 100)` et le gerer soi-meme. C'est un **Arena Allocator** — la technique utilisée par les moteurs de jeux et les bases de données pour atteindre des performances extrêmes.
- **Valgrind est notre meilleur ami, mais le cerveau est premier** : Valgrind dit *où* ça fuite, pas *pourquoi* l'architecture est lente à cause d'une mauvaise utilisation du cache.

---

## 5. Tableau Récapitulatif

| Concept | Développeur moyen | Expert |
| :--- | :--- | :--- |
| **Erreur mémoire** | *"J'ai un segfault, je cherche où."* | Utiliser `gdb` et analyser la stack frame. |
| **Malloc** | *"J'en fais partout, c'est dynamique."* | a éviter dans les boucles critiques. |
| **Structures** | *"Je mets les variables au pif."* | Aligner ses variables pour le CPU. |
| **Performance** | *"Le code est propre, c'est bon."* | Minimiser les cache misses. |

---