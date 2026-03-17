# Modele Memoire du C

## Les cinqs regions et leurs duree de vie

### Texte

    le code compile, en lecture seule.Les pointeurs de fonctions pointent ici.

### Data 
    
    Les varaibles globales et static initialisees avec une valeur non nulle.Elles existent des le lancement du programme jusqu'a sa fin.

```c
    int g = 42;              /* data segment */
    static int s = 7;        /* data segment aussi */
    const char *msg = "hi";  /* msg est dans data, "hi" est dans rodata */
```

### BSS (Block Started by Symbol)

    les globales et static non initialisees, ou initialiseesm a zero.l'OS les zeroise au lancement. La section BSS n'occupe pas de place dans le binaire sur disque, elle est juste unetaille annoncee.

```c
int compteur;            /* BSS — vaut 0 au démarrage, garanti */
static int tampon[1024]; /* BSS — 4096 octets zeroed, pas dans le binaire */
```

### Heap

    la memoire allouee dynamiquement via malloc/calloc/realloc.Tout malloc doit etre free quand on en a plus besoin.

```c
int *p = malloc(10 * sizeof(int));  /* heap */
if (!p)
    return (NULL);
/* utilisation... */
free(p);                            /* fin de vie explicite */
p = NULL;                           /* bonne pratique : évite le double-free */\
```

### Stack 

    les varaibles locales, les parametres de fonctions, les adresses de retour.Geree automatiquement: chaque appel de fonction empile une stack frame, chaque retour la depile.C'est la que vit l'essentiel du code au debut.


## La duree de vie - le concept central

C99 introduit le terme lifetime : la periode pendant laquelle un objet est garanti d'exister en memoire.Acceder a un objet en dehors de sa duree de vie est un comportement indefini.

C99 introduit 3 durees de vies :
-statique : du lancement a la fin du programme
```
int global = 42;          /* storage duration: static */
static int local_static;  /* storage duration: static, mais portée locale */
```
-automatique : de l'entree dans le bloc a la sortie
```
void foo(void)
{
    int x = 10;   /* storage duration: automatic */
    {
        int y = 20;   /* automatic, durée de vie = ce bloc uniquement */
    }
    /* y n'existe plus ici — son espace peut être réutilisé */
}
```
-allouee : du malloc jusqu'au free.
```
int *p = malloc(sizeof(int));  /* storage duration: allocated */
*p = 99;
free(p);   /* fin de vie explicite */
```

## Les Sequences points

C99 dit : entre 2 sequence points, un objet ne doit etre modifie qu'une seule fois, et sa valeur ne doit etre lue que pour determiner la nouvelle valeur a lui ecrire.Toute violation est UB.
Un sequence point est un point dans l'execution ou tous les effets de bord precedents sont garantis termines.
Les principaux en C99:
```
/* ; — fin d'expression complète */
/* , — l'opérateur virgule (pas les virgules d'arguments de fonction) */
/* && et || — après l'évaluation du côté gauche */
/* ?: — après la condition */
/* appel de fonction — avant l'entrée dans la fonction */

/* UB classiques — modification + lecture du même objet sans sequence point */
int i = 5;

i++ + i++;        /* UB — i modifié deux fois entre deux sequence points */
i = i++;          /* UB — modification + lecture pour autre chose que la valeur finale */
a[i] = i++;       /* UB — lecture de i (index) + modification de i */

/* Ce qui n'est PAS UB : */
i++;              /* une seule modification, c'est une expression complète */
int j = i++ + 1;  /* i lu pour calculer, puis incrémenté — une seule modif */

```

le cas i = i++ est interessant, en realite :
```
int i = 5;
i = i++;
/* Selon le compilateur :
   gcc -O0  : i == 5  (l'ancienne valeur écrase l'incrémentation)
   clang -O2: i == 6  (l'incrémentation survit)
   autre    : n'importe quoi — c'est UB, le compilateur fait ce qu'il veut */
```

Les sequences points avec && et || sont au contraire une garantie utile :
```
/* Court-circuit garanti : si p == NULL, *p n'est jamais évalué */
if (p != NULL && *p > 0)
    printf("positif\n");

/* Avec & au lieu de && : les deux côtés sont toujours évalués — crash si p == NULL */
if (p != NULL & *p > 0)   /* NE PAS FAIRE */
    printf("crash possible\n");
```

## L'effectif type - ce qui fonde le script aliasing

C99 dit : chaque objet en memoire a un effectif type, et on ne peut y acceder qu'avec une expression de type compatible.
Pour la memoire statique et automatiquem l'effectif type est le type declare- simple :
```
int x = 42; //effective type : int pour toujours
float f = 3.14f; //effective type : float pour toujours
```