# M3 — Sanitizers

---

## Table des Matières

1. [Principe Général](#1-principe-général)
2. [Présentation de 3 Sanitizers](#2-présentation-de-3-sanitizers--asan-ubsan-et-tsan)
3. [Points clés à retenir](#3-points-clés-sur-asan-ubsan-et-tsan)

---


## 1. Principe Général

Les Sanitizers sont des outils d'instrumentation compilés dans le **binaire**. Contrairement à **Valgrind** qui tourne en dehors du programme, les sanitizers modifient le code généré à la compilation pour insérer des vérifications à l'exécution. Le coût : le binaire est plus lent et plus gros. Le bénéfice : il dit exactement ce qui explose, où et surtout pourquoi.

Les différents Sanitizers présentés dans ce cours ne se remplacent pas mutuellement ; ils couvrent des choses différentes.

## 2. Présentation de 3 Sanitizers : ASan, UBSan et TSan

### ASan : AdressSanitizer

Ce qu'il détecte :
* **Heap** buffer overflow/underflow : on lit ou on écrit à côté d'un bloc **malloc**
* **Stack** buffer overflow : on déborde d'un tableau local
* Use-after-free : on déréférence un pointeur vers de la mémoire déjà libérée
* Use-after-return : on retourne un pointeur vers une variable locale (**UB** classique)
* Double-free : on appelle **free** 2 fois sur le même pointeur
* **Memory Leaks** via LeakSanitizer, intégré par défaut sous Linux

Ce qu'il ne détecte pas :
les accès hors limites dans des tableaux **statiquement alloués globaux** dans certains cas, et tout ce qui relève du comportement logique.

#### Compilation

```makefile
CFLAGS += -fsanitize=address -fno-omit-frame-pointer -g
LDFLAGS += -fsanitize=address
```

`-fno-omit-frame-pointer` est crucial : il permet à ASan de reconstruire la call stack lisiblement. Sans lui, les stack traces sont illisibles.

#### Lire un rapport ASan
```bash
==12345==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x602000000014
READ of size 4 at 0x602000000014 thread T0
    #0 0x401234 in foo /home/user/projet/foo.c:42
    #1 0x401567 in main /home/user/projet/main.c:10

0x602000000014 is located 4 bytes to the right of 16-byte region
[0x602000000000, 0x602000000010) allocated by thread T0 here:
    #0 0x7f... in malloc
    #1 0x401100 in foo /home/user/projet/foo.c:38
```
À lire dans l'ordre :
* Le type d'erreur (`heap-buffer-overflow`, `use-after-free`, etc.)
* L'opération (`READ`, ou `WRITE`) et sa taille
* La stack trace de l'accès fautif : la ligne qui a planté
* La description de la région : où était ce bloc, qui l'a alloué, à quelle distance on était.

#### Variables d'environnement utiles

```
ASAN_OPTIONS=detect_leaks=1        # activer LeakSanitizer (défaut Linux)
ASAN_OPTIONS=abort_on_error=1      # crash propre pour les core dumps
ASAN_OPTIONS=verbosity=1           # plus de détails internes
```

### UBSan : UndefinedBehaviorSanitizer

Ce qu'il détecte :
C'est le sanitizer que nous avons vu dans le cours sur l'**UB**. Il attrape à l'exécution les UB que le compilateur aurait le droit d'exploiter :


| Vérification | Ce qui est détecté |
| :----------- | :----------------- |
| `signed-integer-overflow` | `INT_MAX + 1` |
| `null` | déréférencement de pointeur nul |
| `bounds` | accès hors-limites sur des tableaux VLA ou avec taille connue |
| `shift` | décalage de bits d'une valeur négative ou trop grande |
| `vla-bound` | VLA de taille ≤ 0 |
| `alignement` | accès non aligné |
| `bool` | valeur non 0/1 dans un `_Bool` |
| `pointeur-overflow` | arithmétique de pointeur qui wrappe |

#### Compilation 

```
CFLAGS += -fsanitize=undefined -g
LDFLAGS += -fsanitize=undefined
```

Pour avoir des messages lisibles (sinon on a juste une adresse) :

```makefile
CFLAGS += -fsanitize=undefined -fno-sanitize-recover=all -g
```

`-fno-sanitize-recover=all` fait crasher le programme dès le premier UB au lieu de continuer (comportement par défaut : il affiche et continue, ce qui peut masquer la vraie cause).

### Lire un rapport UBSan
```bash
foo.c:42:15: runtime error: signed integer overflow: 2147483647 + 1
              cannot be represented in type 'int'
```

C'est le plus lisible des 3 : on a le fichier, la ligne, la colonne, le type d'UB, les valeurs impliquées.

#### Combinaison ASan + UBSan

Ces 2 sanitizers sont parfaitement compatibles et se combinent sans problème :
```
CFLAGS += -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all -g
LDFLAGS += -fsanitize=address,undefined
```

### TSan : ThreadSanitizer

Ce qu'il détecte :
* Les data races : 2 threads accèdent à la même zone mémoire, au moins un fait une écriture, sans synchronisation. C'est de l'**UB** en C. TSan instrumente chaque accès mémoire pour suivre quel thread a touché quoi, et quelles barrières de synchronisation ont été posées.

Il détecte aussi :
* Les deadlocks potentiels (verrous pris dans un ordre incohérent)
* Les fuites de mutex (un thread qui oublie de déverrouiller)

Ce qu'il ne détecte pas :
les races qui ne sont pas produites lors de cette exécution. TSan est dynamique : si le scheduling ne provoque pas la race pendant le test, il ne la voit pas. C'est pour ça qu'on rejoue les tests plusieurs fois sous TSan.

#### Compilation

```makefile
CFLAGS += -fsanitize=thread -g
LDFLAGS += -fsanitize=thread
```

⚠️ **TSan est incompatible avec ASan**. On ne peut pas les activer ensemble. Il faut deux targets séparées dans le Makefile.

### Lire un rapport TSan
```bash
==================
WARNING: ThreadSanitizer: data race (pid=12345)
  Write of size 4 at 0x7f... by thread T2:
    #0 increment /home/user/projet/counter.c:10

  Previous read of size 4 at 0x7f... by thread T1:
    #0 print_counter /home/user/projet/counter.c:20

  Location is global 'counter' of size 4 at 0x... (counter.c+0x...)

  Thread T2 (tid=..., running) created by main thread at:
    #0 pthread_create ...

  Thread T1 (tid=..., running) created by main thread at:
    #0 pthread_create ...
==================
```

À lire ici :
* L'opération fautive : qui écrit, qui lit, sur quoi
* Les stack traces des 2 threads impliqués
* La localisation de la variable en conflit
* Où les threads ont été créés pour pouvoir retrouver le contexte

##### Proposition d'une intégration Makefile propre

```makefile
CC      = gcc
CFLAGS  = -Wall -Wextra -Werror -g
SRC     = $(wildcard src/*.c)
OBJ     = $(SRC:.c=.o)

# Build normal
all: mon_programme

mon_programme: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# ASan + UBSan
asan: CFLAGS += -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all
asan: LDFLAGS += -fsanitize=address,undefined
asan: mon_programme

# TSan (target séparée, incompatible avec ASan)
tsan: CFLAGS += -fsanitize=thread -fno-omit-frame-pointer
tsan: LDFLAGS += -fsanitize=thread
tsan: mon_programme

clean:
	rm -f src/*.o mon_programme
```

##### Usage

```bash
make asan && ./mon_programme
make tsan && ./mon_programme
```

## 3. Points clés sur ASan, UBSan, et TSan

### Sur ASan :

* Le ralentissement est ~2x, l'overhead mémoire ~2-3x → acceptable pour des tests
* Sur macOS, le comportement diffère légèrement (pas de LeakSanitizer par défaut)

### Sur UBSan :

* `-fno-sanitize-recover=all` est presque toujours ce qui permet de trouver les erreurs : un UB silencieux est pire qu'un crash
* Les checks `bounds` ne couvrent pas tous les tableaux

### Sur TSan :

* Le ralentissement est ~5-15x et l'overhead mémoire ~5-10x
* Relancer les tests plusieurs fois augmente les chances de détecter une race
* Un rapport TSan sans race ne garantit pas l'absence de race