# M2 — Undefined Behavior

---

## Tables des Matieres

1. [Definition](#1-definition)
2. [L'Overflow Signe](#2-loverflow-signe)
3. [Le debordement de tableau](#3-le-debordement-de-tableau-out-of-bounds)
4. [Le comportement du compilateur face a l'Undefined Behavior](#4-le-comportement-du-compilateur-face-a-lub)
5. [Schéma des passes d'optimisation](#schéma-des-passes-doptimisation)
6. [Résumé conceptuel](#résumé-conceptuel)

---

## 1. Definition

Le C99 classe les comportements en 3 categories:
* defini -> le resultat est donc garanti, identique partout
* defini par l'implementation -> le compilateur choisit, mais documente son choix
* indefini -> le standart ne dit rien. **Tout peut arriver**.

La phrases cle du standart ISO C99 : "behavior, upon use of a nonportable or erroneous program construct [...] for which this International Standard imposes no requirements."
Le compilateur a le droit de supposer que l'UB n'arrive **jamais**.C'est la que ca devient interessant.

## 2. L'overflow signe

**la regle**
En C99, le depassement d'un entier signe est **indefini**.Pas de wrapping (solution deja impentee), pas d'exception.
```
int x = INT_MAX;
x = x + 1; //UB car depassement d'un entier signe
```

L'overflow d'un entier **non signe**, lui est parfaitement defini : il wrappe modulo 2 puissance n.
```
unsigned int u = UINT_MAX;
u = u + 1; //defini u = 0;
```

**GCC** et **Clang** exploitent activement l'UB pour optimiser.Puisque l'overflow signe ne peut pas arriver (definition du standart), le compilateur est libre de raisonner ainsi :

```
//Est ce que cette boucle se termine ?
for (int i = 0; i < i + 1; i++)
{
    do_something();
}
```
Un humain dirait "ca boucle indefiniment quand i a atteint INT_MAX".Le compilateur, lui raisonne "i < i + 1 est toujours vrai car l'overflow signe n'existe pas" -> il transforme ca en boucle indefini inconditionelle, ou le supprime completement **selon le contexte**.

Autre exemple classique :
```
int foo(int x)
{
    if (x + 1 > x)
        return (1);
    return (0);
}
```
Avec le flag de compilation -O2, **GCC** compile ca en `return(1);` directement.Correct selon le standart, mais surprenant pour le developpeur.

Le flag **-fwrapv**
**GCC**/**Clang** proposent -fwrapv pour **forcer** le wrapping en complement a 2.Ca desactive les optimisations liees a l'UB signe, utile pour le code legacy, mais ce n'est plus du C standart.

## 3. Le debordement de tableau (out-of-bounds)

**la regle**

acceder a un tableau hors de ses bornes est **indefini**.Pas de segfault garanti, pas de valeur indefinie previsible - UB.
```
int arr[5];
arr[5] = 42;//UB indice valable de 0 a 4
arr[-1] = 42; //UB
```

Pourquoi il n'y aura pas un segfault systematique ?

Le segfault est un effet possible de l'UB, mais pas le seul.L'adresse `arr[5]` est souvent une adresse memoire **valide** (stack, heap, segments de donnees adjacent).L'acces reussit silencieusement, corrompt une autre variable, et le programe plante bien plus loin -- ou pire, continue de fonctionner avec des donnees corrompues.

```
int arr[5] = {0};
int important = 100;
arr[5] = 0; //ecrase peut etre 'important' selon l'alignement stack
printf("%d\n, important); //affiche potentiellement 0
```

Optimisations agressives

Le complilateur suppose que les acces sont valides.Il peut donc **supprimer des verifications de bornes** qu'il juge redondantes :
```
void process(int *arr, int n)
{
    if (n <= 0)
        return ;
    //le compilateur sait que arr[0] sera accede.
    // donc n > 0, donc certaines branches peuvent etre eliminees
    for (int i = 0; i <= n; i++)
    {
        arr[i] = i; //UB quand i == n
    }
}
```

le compilateur peut vectoriser cette boucle en supprimant des gardes de securite, rendant le comportement encore plus impreisible.

Le cas du pointeur arithmetique

Un pointeur peut pointer un element apres la fin d'un tableau (c'est defini) mais le **dereferencer** est UB :
```
int arr[5];
int *p = arr + 5; //OK pointeur valide (past the end)
*p = 42; //dereferencement interdit
```

## 4. Le comportement du compilateur face a l'UB

**le principe fondamental**

Le compilateur a le droit de supposer que le programme **ne contient pas d'UB**.Cette hypothese est utilisee comme axiome dans les passes d'optimisation.

En consequence : si une branche d'execution **menerait** a de l'UB, le compilateur peut supposer que cette branche **n'est jamais interdite** et raisonner en consequence.
```
int foo(int *p) {
    int x = *p;      // si p est NULL → UB
    if (p == NULL)   // le compilateur sait que p != NULL (sinon ligne précédente = UB)
        return -1;   // cette branche est DEAD CODE → supprimée
    return x;
}
```

Ce pattern — vérification de nullité **après** déréférencement — a causé des failles de sécurité réelles dans le noyau Linux.

### 5. Schéma des passes d'optimisation
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

Les niveaux d'optimisations

| Flag | Comportement UB |
| :--- | :-------------- |
| -O0  | UB souvent "inoffensif" car peu d'optimisation |
| -O1  | Quelques éliminations de code mort |
| -O2  | Exploitation active de l'UB |
| -O3  | Tres aggressif, vectorisation, UB amplifie |
| -fsanitize=undefined | Détecte l'UB à l'exécution (UBSan) |

Les outils de detection

```
# Undefined Behavior Sanitizer (Clang/GCC)
clang -fsanitize=undefined -g mon_programme.c

# Address Sanitizer (débordements mémoire)
clang -fsanitize=address -g mon_programme.c

# Les deux combinés
clang -fsanitize=undefined,address -g mon_programme.c
```

---

## 6. Résumé conceptuel
```
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
