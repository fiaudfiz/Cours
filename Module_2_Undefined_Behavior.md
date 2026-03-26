# M2 — Undefined Behavior

---

## Table des Matières

1. [Définition](#1-définition)
2. [L'Overflow Signé](#2-loverflow-signé)
3. [Le débordement de tableau](#3-le-débordement-de-tableau-out-of-bounds)
4. [Le comportement du compilateur face à l'Undefined Behavior](#4-le-comportement-du-compilateur-face-à-lub)
5. [Schéma des passes d'optimisation](#schéma-des-passes-doptimisation)
6. [Résumé conceptuel](#résumé-conceptuel)

---

## 1. Définition

Le C99 classe les comportements en 3 catégories :
* **Défini** → le résultat est garanti, identique partout
* **Défini par l'implémentation** → le compilateur choisit, mais documente son choix
* **Indéfini** → le standard ne dit rien. **Tout peut arriver**.

La phrase clé du standard ISO C99 : "behavior, upon use of a nonportable or erroneous program construct [...] for which this International Standard imposes no requirements."

Le compilateur a le droit de supposer que l'UB n'arrive **jamais**. C'est là que ça devient intéressant.

## 2. L'overflow signé

**La règle**

En C99, le dépassement d'un entier signé est **indéfini**. Pas de wrapping, pas d'exception.
```c
int x = INT_MAX;
x = x + 1; /* UB — dépassement d'un entier signé */
```

L'overflow d'un entier **non signé**, lui, est parfaitement défini : il wrappe modulo 2^n.
```c
unsigned int u = UINT_MAX;
u = u + 1; /* défini — u = 0 */
```

**GCC** et **Clang** exploitent activement l'UB pour optimiser. Puisque l'overflow signé ne peut pas arriver (définition du standard), le compilateur est libre de raisonner ainsi :

```c
/* Est-ce que cette boucle se termine ? */
for (int i = 0; i < i + 1; i++)
{
    do_something();
}
```
Un humain dirait "ça boucle indéfiniment quand i a atteint INT_MAX". Le compilateur, lui, raisonne "i < i + 1 est toujours vrai car l'overflow signé n'existe pas" → il transforme ça en boucle infinie inconditionnelle, ou la supprime complètement **selon le contexte**.

Autre exemple classique :
```c
int foo(int x)
{
    if (x + 1 > x)
        return (1);
    return (0);
}
```
Avec le flag de compilation `-O2`, **GCC** compile ça en `return (1);` directement. Correct selon le standard, mais surprenant pour le développeur.

**Le flag `-fwrapv`**

**GCC**/**Clang** proposent `-fwrapv` pour **forcer** le wrapping en complément à 2. Ça désactive les optimisations liées à l'UB signé, utile pour le code legacy, mais ce n'est plus du C standard.

## 3. Le débordement de tableau (out-of-bounds)

**La règle**

Accéder à un tableau hors de ses bornes est **indéfini**. Pas de segfault garanti, pas de valeur indéfinie prévisible — UB.
```c
int arr[5];
arr[5] = 42;  /* UB — indice valable de 0 à 4 */
arr[-1] = 42; /* UB */
```

Pourquoi il n'y aura pas un segfault systématique ?

Le segfault est un effet possible de l'UB, mais pas le seul. L'adresse `arr[5]` est souvent une adresse mémoire **valide** (stack, heap, segments de données adjacents). L'accès réussit silencieusement, corrompt une autre variable, et le programme plante bien plus loin — ou pire, continue de fonctionner avec des données corrompues.

```c
int arr[5] = {0};
int important = 100;
arr[5] = 0; /* écrase peut-être 'important' selon l'alignement stack */
printf("%d\n", important); /* affiche potentiellement 0 */
```

**Optimisations agressives**

Le compilateur suppose que les accès sont valides. Il peut donc **supprimer des vérifications de bornes** qu'il juge redondantes :
```c
void process(int *arr, int n)
{
    if (n <= 0)
        return ;
    /* le compilateur sait que arr[0] sera accédé */
    /* donc n > 0, donc certaines branches peuvent être éliminées */
    for (int i = 0; i <= n; i++)
    {
        arr[i] = i; /* UB quand i == n */
    }
}
```

Le compilateur peut vectoriser cette boucle en supprimant des gardes de sécurité, rendant le comportement encore plus imprévisible.

**Le cas du pointeur arithmétique**

Un pointeur peut pointer un élément après la fin d'un tableau (c'est défini) mais le **déréférencer** est UB :
```c
int arr[5];
int *p = arr + 5; /* OK — pointeur valide (past the end) */
*p = 42;          /* déréférencement interdit — UB */
```

## 4. Le comportement du compilateur face à l'UB

**Le principe fondamental**

Le compilateur a le droit de supposer que le programme **ne contient pas d'UB**. Cette hypothèse est utilisée comme axiome dans les passes d'optimisation.

En conséquence : si une branche d'exécution **mènerait** à de l'UB, le compilateur peut supposer que cette branche **n'est jamais atteinte** et raisonner en conséquence.
```c
int foo(int *p) {
    int x = *p;      /* si p est NULL → UB */
    if (p == NULL)   /* le compilateur sait que p != NULL (sinon ligne précédente = UB) */
        return -1;   /* cette branche est DEAD CODE → supprimée */
    return x;
}
```

Ce pattern — vérification de nullité **après** déréférencement — a causé des failles de sécurité réelles dans le noyau Linux.

## 5. Schéma des passes d'optimisation
```
Code source C99
       │
       ▼
  Parsing / AST
       │
       ▼
  Passes IR (LLVM/GCC)
  ┌────────────────────────────────────┐
  │  "UB ne peut pas arriver"          │
  │  → dead code elimination           │
  │  → loop transformations            │
  │  → constant folding agressif       │
  │  → suppression de vérifications    │
  └────────────────────────────────────┘
       │
       ▼
  Code machine
```

**Les niveaux d'optimisation**

| Flag | Comportement UB |
| :--- | :-------------- |
| `-O0` | UB souvent "inoffensif" car peu d'optimisation |
| `-O1` | Quelques éliminations de code mort |
| `-O2` | Exploitation active de l'UB |
| `-O3` | Très agressif, vectorisation, UB amplifié |
| `-fsanitize=undefined` | Détecte l'UB à l'exécution (UBSan) |

**Les outils de détection**

```bash
# Undefined Behavior Sanitizer (Clang/GCC)
clang -fsanitize=undefined -g mon_programme.c

# Address Sanitizer (débordements mémoire)
clang -fsanitize=address -g mon_programme.c

# Les deux combinés
clang -fsanitize=undefined,address -g mon_programme.c
```

---

## 6. Résumé conceptuel
```plaintext
       Le standard C99 dit :
       "Si UB → aucune garantie"
                │
                ▼
       Le compilateur entend :
       "UB n'arrive JAMAIS"
                │
                ▼
       Il optimise en supposant
       que les cas UB sont impossibles
                │
          ┌─────┴──────┐
          ▼            ▼
     Code plus      Comportements
     rapide         surprenants / dangereux
```