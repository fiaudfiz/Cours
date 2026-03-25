# Les Sanitizers

---

## Tables des matieres






---

## 1. Principe General

Les Sanitizers sont des outils d'instrumentation compiles dans le **binaire**.Contrairement a **Valgrind** qui tourne en dehors du programme, les sanitizers modifient le code genere a la compilation pour inserer des verifications a l'execution.Le cout : le binaire est plus lent et plus gros.Le benefice : Il dit exactement ce qui explose, ou et surtout pourquoi.

Les Differents Sanitizers presente dans ce cours ne se remplacent pas mutuellement; ils couvrent des choses differentes.

## Presentation de 3 Sanitizers : ASan, UBSan et TSan

### ASan : AdressSanitizer

Ce qu'il detecte:
* **Heap** buffer overflow/underflow : on lit ou on ecrit a cote d'un bloc **malloc**
* **Stack** buffer overflow : on deborde d'un tableau local
* Use-after-free : on dereference un pointeur vers de la memoire deja liberee
* Use-after-return : on retourne un pointeur vers une variable local (**UB** classique)
* Double-free : on appelle **free** 2 fois sur le meme pointeur
* **Memory Leaks** via LeakSanitizer, integre par defaut sous Linux

Ce qu'il ne detecte pas :
les acces hors limites dans des tableaux **statiquement alloues globaux** dans certains cas, et tout ce qui releve du comportement logique.

#### Compilation

```
CFLAGS += -fsanitize=address -fno-omit-frame-pointer -g
LDFLAGS += -fsanitize=address
```

`-fno-omit-frame-pointer` est crucial : il permet à ASan de reconstruire la call stack lisiblement. Sans lui, les stack traces sont illisibles.

####  Lire un rapport ASan
```
==12345==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x602000000014
READ of size 4 at 0x602000000014 thread T0
    #0 0x401234 in foo /home/user/projet/foo.c:42
    #1 0x401567 in main /home/user/projet/main.c:10

0x602000000014 is located 4 bytes to the right of 16-byte region
[0x602000000000, 0x602000000010) allocated by thread T0 here:
    #0 0x7f... in malloc
    #1 0x401100 in foo /home/user/projet/foo.c:38
```
A lire dans l'ordre :
* Le type d'erreur (`heap-buffer-overflow`, `use-after-free`, etc...)
* L'operation (`READ`, ou `WRITE`) et sa taille
* La stack trace de l'acces fautif : la ligne qui a plante
* La description de la region : ou etait ce bloc, qui l'a alloue, a quelle distance on etait.

#### Variables d'environnement utiles

```
ASAN_OPTIONS=detect_leaks=1        # activer LeakSanitizer (défaut Linux)
ASAN_OPTIONS=abort_on_error=1      # crash propre pour les core dumps
ASAN_OPTIONS=verbosity=1           # plus de détails internes
```

### UBSan : UndefinedBehaviorSanitizer

Ce qu'il detecte:
C'est le sanitizer que nous avons vu dans le cours sur l'**UB**.Il attrape a l'execution les UB que le compilateur aurait le droit d'exploiter :


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

Pour avoir des messages lisibles (sinon on a juste un adresse) :

```
CFLAGS += -fsanitize=undefined -fno-sanitize-recover=all -g
```

`-fno-sanitize-recover=all` fait crasher le programme dès le premier UB au lieu de continuer (comportement par défaut : il affiche et continue, ce qui peut masquer la vraie cause).

### Lire un rapport UBSan
```
foo.c:42:15: runtime error: signed integer overflow: 2147483647 + 1
              cannot be represented in type 'int'
```

C'est le plus lisible des 3 : On a la fichier, la ligne, la colonne, le type d'UB, les valeurs impliquées.

#### Combinaison ASan + UBSan

Ces 2 sanitizers sont parfaitement compatibles et se combinent sans probleme :
```
CFLAGS += -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all -g
LDFLAGS += -fsanitize=address,undefined
```

### TSan : TheadSanitizer

Ce qu'il detecte :
* les data races : 2 threads accedent a la meme zone memoire, au moins un fait une ecriture, sans synchronisation.C'est de l'**UB** en C.TSan instrumente chaque acces memoire pour suivre quel thread a touche quoi, et quelles barrieres de synchronisation ont ete posees.

Il detecte aussi :
* les deadslocks potentiels (verrous pris dans un ordre incoherent)
* les fuites de mutex (un thread qui oublie de deverouiller)

Ce qu'il ne detecte pas :
les races qui ne sont pas produits lors de cette execution.TSan est dynamique : si le scheduling ne provoque pas la race pendant le test, il ne la voit pas.C'est pour ca que on rejoue les tests plusieurs fois sous TSan.

#### Compilation

```
CFLAGS += -fsanitize=thread -g
LDFLAGS += -fsanitize=thread
```

⚠️ **TSan est incompatible avec ASan**. On ne peut pas les activer ensemble. Il faut deux targets séparées dans le Makefile.

### Lire un rapport TSan
```
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

A lire ici :
* L'operation fautive : qui ecrit, qui lit, sur quoi
* Les stacks traces des 2 threads impliques
* La localisation de la variable en conflit
* Ou les threads ont ete crees pour pouvoir retrouver le contexte

##### Proposition d'une integration Makefile propre

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

## Points cles sur ASan, UBSan, et TSan

### Sur ASan :

* le ralentissement est ~2x, l'overhead memoire ~2-3x -> acceptable pour des tests
* Sur macOS, le comportement differe legerement (pas de LeakSanitizer par defaut)

### Sur UBSan :

* `-fno-sanitize-recover=all` est presque toujours ce qui permet de trouver les erreurs : un UB silencieux est pire qu'un crash
* Les checks `bounds` ne couvrent pas tous les tableaux

### Sur TSan :

* Le ralentissement est ~5-15x et l'overhead memoire ~5-10x
* Relancer les tests plusieurs fois augmente les chances de detecter une race
* Un rapport TSan sans race ne garantit pas l'absence de race