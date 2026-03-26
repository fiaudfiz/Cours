# M1 — Modèle mémoire C99

---

## Table des Matières

1. [Les cinq régions et leurs durée de vie](#1-les-cinq-régions-et-leurs-durée-de-vie)
2. [La durée de vie](#2-la-durée-de-vie---le-concept-central)
3. [Les séquences points](#3-les-séquences-points)
4. [L'effectif type](#4-leffectif-type---ce-qui-fonde-le-strict-aliasing)

---

## 1. Les cinq régions et leurs durée de vie

### Texte

Le code compilé, en lecture seule. Les pointeurs de fonctions pointent ici.

### Data

Les variables globales et static initialisées avec une valeur non nulle. Elles existent dès le lancement du programme jusqu'à sa fin.

```c
int g = 42;              /* data segment */
static int s = 7;        /* data segment aussi */
const char *msg = "hi";  /* msg est dans data, "hi" est dans rodata */
```

### BSS (Block Started by Symbol)

Les globales et static non initialisées, ou initialisées à zéro. L'OS les zéroïse au lancement. La section BSS n'occupe pas de place dans le binaire sur disque, elle est juste une taille annoncée.

```c
int compteur;            /* BSS — vaut 0 au démarrage, garanti */
static int tampon[1024]; /* BSS — 4096 octets zeroed, pas dans le binaire */
```

### [Heap](https://github.com/fiaudfiz/Cours/tree/main/stack%20heap)

La mémoire allouée dynamiquement via malloc/calloc/realloc. Tout malloc doit être free quand on en a plus besoin.

```c
int *p = malloc(10 * sizeof(int));  /* heap */
if (!p)
    return (NULL);
/* utilisation... */
free(p);                            /* fin de vie explicite */
p = NULL;                           /* bonne pratique : évite le double-free */
```

### [Stack](https://github.com/fiaudfiz/Cours/tree/main/stack%20heap)

Les variables locales, les paramètres de fonctions, les adresses de retour. Gérée automatiquement : chaque appel de fonction empile une stack frame, chaque retour la dépile. C'est là que vit l'essentiel du code au début.


## 2. La durée de vie - le concept central

C99 introduit le terme lifetime : la période pendant laquelle un objet est garanti d'exister en mémoire. Accéder à un objet en dehors de sa durée de vie est un [**comportement indéfini**](https://github.com/fiaudfiz/Cours/blob/main/Module_2_Undefined_Behavior.md).

C99 introduit 3 durées de vie :

**Statique** : du lancement à la fin du programme.
```c
int global = 42;          /* storage duration: static */
static int local_static;  /* storage duration: static, mais portée locale */
```

**Automatique** : de l'entrée dans le bloc à la sortie.
```c
void foo(void)
{
    int x = 10;   /* storage duration: automatic */
    {
        int y = 20;   /* automatic, durée de vie = ce bloc uniquement */
    }
    /* y n'existe plus ici — son espace peut être réutilisé */
}
```

**Allouée** : du malloc jusqu'au free.
```c
int *p = malloc(sizeof(int));  /* storage duration: allocated */
*p = 99;
free(p);   /* fin de vie explicite */
```

## 3. Les séquences points

C99 dit : entre 2 séquence points, un objet ne doit être modifié qu'une seule fois, et sa valeur ne doit être lue que pour déterminer la nouvelle valeur à lui écrire. Toute violation est UB.

Un séquence point est un point dans l'exécution où tous les effets de bord précédents sont garantis terminés.

Les principaux en C99 :
```c
/* ; — fin d'expression complète */
/* , — l'opérateur virgule (pas les virgules d'arguments de fonction) */
/* && et || — après l'évaluation du côté gauche */
/* ?: — après la condition */
/* appel de fonction — avant l'entrée dans la fonction */

/* UB classiques — modification + lecture du même objet sans séquence point */
int i = 5;

i++ + i++;        /* UB — i modifié deux fois entre deux séquence points */
i = i++;          /* UB — modification + lecture pour autre chose que la valeur finale */
a[i] = i++;       /* UB — lecture de i (index) + modification de i */

/* Ce qui n'est PAS UB : */
i++;              /* une seule modification, c'est une expression complète */
int j = i++ + 1;  /* i lu pour calculer, puis incrémenté — une seule modif */
```

Le cas `i = i++` est intéressant, en réalité :
```c
int i = 5;
i = i++;
/* Selon le compilateur :
   gcc -O0  : i == 5  (l'ancienne valeur écrase l'incrémentation)
   clang -O2: i == 6  (l'incrémentation survit)
   autre    : n'importe quoi — c'est UB, le compilateur fait ce qu'il veut */
```

Les séquences points avec `&&` et `||` sont au contraire une garantie utile :
```c
/* Court-circuit garanti : si p == NULL, *p n'est jamais évalué */
if (p != NULL && *p > 0)
    printf("positif\n");

/* Avec & au lieu de && : les deux côtés sont toujours évalués — crash si p == NULL */
if (p != NULL & *p > 0)   /* NE PAS FAIRE */
    printf("crash possible\n");
```

## 4. L'effectif type - ce qui fonde le strict aliasing

C99 dit : chaque objet en mémoire a un effectif type, et on ne peut y accéder qu'avec une expression de type compatible.

Pour la mémoire statique et automatique, l'effectif type est le type déclaré — simple :
```c
int x = 42;     /* effective type : int pour toujours */
float f = 3.14f; /* effective type : float pour toujours */
```