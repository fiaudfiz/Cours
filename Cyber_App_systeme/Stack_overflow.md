# Stack Overflow et exploitation de buffer Overflows

---

## Table des Matieres

---

Ce cours va s'appuyer sur cet article : 

Les buffers overflows representent 60% des annonces de securites du CERT () de nos jours.Le buffer overflow est le vecteur d'attaque le plus courant dans les intrusions systemes et particulierement dans les attaques a distance.
Des estimations indiquent que pour 1000 lignes de code, il y a entre 5 et 15 erreurs, ce cours va detailler tout d'abord l'organisation memoire de la stack, les potentielles exploitations des failles et quelques conseils pour eviter l'exploitation des buffer overflows.

## ELF et memoire virtuelle

memoire virtuelle : chaque programme quand il est execute obtient un espace memoire entierement isole.Sur une architecture x86 32 bits, l’espace adressable théorique est de 4 Go. Sur x86-64 moderne, l’espace virtuel est bien plus grand..Linux utilise pour les programmes executables le format ELF (Executable Linking Format) qui est compose de plusieurs sections.

L'espace virtuel est divise lui meme en 2 zones :
- l'espace user (0x0000000 -0xbfffffff)
- l'espace kernel (0xc0000000 - 0xfffffff)

Un processus user ne peut pas acceder a l'espace kernel mais l'inverse est possible.

Un exécutable ELF est transformé en une image processus par le program loader. 
Pour créer cette image en mémoire, le program loader va mapper en mémoire tous les loadable segments de l'exécutable et des librairies requises au moyen de l'appel système mmap(). Historiquement sans PIE, les binaires étaient souvent chargés à des adresses fixes comme 0x08048000. Aujourd’hui, avec PIE + ASLR, ces adresses sont souvent randomisées.

La figure suivante indique les sections principales d'un programme en memoire.La section .txt represente le code du programme.Dans la section .data sont placees les variables globales initialisees (elles sont connues a la compilation) et dans la section .bss les variables globales non-initialisees.

La stack contient les variables locales automatiques (une variable locale est automatique).Elle fonctionne sur le principe LIFO, et la stack croit vers les adresses basses, cad on commence par ex a 0xc0000000 et apres on decsend a ..... .A l'execution d'un programme, les arguments ainsi que les variables d'env sont egalement stockees dans la pile.Et les variables allouees dynamiquement (malloc, calloc...) sont stockees dans la heap. (les adresses croissent)

![alt text](image.png)

Exemple :

```c
int var;                        // bss
char var2[] = "buf1";           // data

int main()
{
    static int var3;                // bss
    char *var4                      // stack
    char *var5 = malloc(32);        // heap
    static char var6[] = "buf2";    // data
}
```

La commande size permet de connaitre les differentes sections d'un programme ELF et de leur adresse memoire

```bash
➜  size -A -x a.out

section                  size       addr
.note.gnu.build-id       0x24   0x400318
.init                    0x1b   0x40033c
.plt                     0x20   0x400360
.text                   0x12f   0x400380
.fini                     0xd   0x4004b0
.interp                  0x1c   0x401000
.gnu.hash                0x1c   0x401020
.dynsym                  0x60   0x401040
.dynstr                  0x4a   0x4010a0
.gnu.version              0x8   0x4010ea
.gnu.version_r           0x30   0x4010f8
.rela.dyn                0x30   0x401128
.rela.plt                0x18   0x401158
.rodata                  0x10   0x401170
.eh_frame_hdr            0x2c   0x401180
.eh_frame                0x8c   0x4011b0
.note.gnu.property       0x20   0x401240
.note.ABI-tag            0x20   0x401260
.init_array               0x8   0x402df8
.fini_array               0x8   0x402e00
.dynamic                0x1d0   0x402e08
.got                     0x10   0x402fd8
.got.plt                 0x20   0x402fe8
.data                     0x4   0x403008
.bss                      0x4   0x40300c
.comment                 0x5a        0x0
.annobin.notes          0x14f        0x0
.gnu.build.attributes   0x144   0x405010
Total                   0x90a

```
(Des informations similaires mais plus détaillées peuvent être obtenues avec les
commandes readelf –e ou objdump -h). Nous voyons apparaître l’adresse en mémoire
et la taille (en bytes) des sections qui nous intéressent : .text, .data et .bss. 

## Appel d'une fonction en assembleur

C'est maintenant l'occasion de faire une mini parenthese sur comment est appelee une fonction en assembleur car cela sera utile pour la suite.

### Les registres %eip %ebp et %esp (x86-32 bits) et RIP RSP RBP (x86-64)


Nous avons ici les memes registres mais une difference de nomination et egalement de taille puisque les registres x84-32 bits font 4 octets et les registres x86-64 bits font 8 octets mais les roles restent les memes.

#### %eip || RIP

Extended Instrucion Pointer : c'est le curseur du CPU, il indique quelle instruction executer avec un pointeur sur la suivante.Le CPU fait la boucle suivante : lire instruction a %eip --> executer --> %eip++

#### %esp || RSP

Extended Stack Pointer : il pointe toujours vers le sommet de la stack, il bouge a chaque push (ajout) ou pop (suppression) sur la stack. On rappele ici que la stack grandit en decrementant les adresses.

#### %ebp || RBP

Extended Base Pointer : c'est le point de repere fixe de la fonction, il permet de retrouver ou sont les varaibles et les parametres.Il reste stable pendant toute l'execution de la fonction.

### appel de la fonction

#### Prologue de la fonction

#### Epilogue de la fonction

#### Rapport avec les buffers overflows

Si on deborde d'un buffer local, on remonte dans la pile :

```
%esp → [sommet actuel]
       [buffer local]     ← débordement ici...
       [autre variable]   
       -------------------
%ebp → [ancien %ebp]       ← écrase ça...
       [adresse retour]    ← puis ÇA !
       [paramètres]
```

--> Consequence ici, le retour de la fonction est modifie ce qui peut crasher le programme ou executer une autre fonction pour prendre le controle de l'ordinateur.

## Exemple de Stack overflow

Quand on place dans un espace memoire plus de donnee qu'il ne peut en contenir, on cree un buffer overflow. Ici nous allons voir le cas avec un buffer non-alloue donc sur la stack.Les donnees mises en trop sont quand meme inserees en memoire meme si elles ecrasent des donnees qu'elles ne devraient pas.Nous allons voir dans cet exemple que si on ecrase certaines donnees on peut arriver a prendre le controle du programme.Voici un exemple de code vunerable :

```c
#include <stdio.h>
#include <string.h>

int main(int ac, char **av)
{
	char buffer[256];

	if (ac > 1)
		strcpy(buffer, av[1]);
}
```

Ce programme ne fait rien de plus de prendre le premier argument envoye a ce programme et le placer dans le buffer avec l'appel a la fonction strcpy.Seulement, a aucun moment du programme on regarde la taille de av[1] pour que cette taille soit inferieure a celle du buffer.Un probleme arrive lorsque l'utilisateur donne un argument plus grand que la taille du buffer :
```bash
➜ /a.out "$(python3 -c "print('A'*300)")"
[1]    1678256 segmentation fault (core dumped)  ./a.out "$(python3 -c "print('A'*300)")"
➜  
```

Pour mieux comprendre ce segfault, nous allons regarder au niveau des fichiers core cree a la fin d'un segfault :
```bash
coredumpctl gdb a.out

```

ici nous ouvrons avec gdb le coredump de cet executable apres l'avoir execute:

```bash
       Message: Process 1675463 (a.out) of user 44516092 dumped core.
                
                Stack trace of thread 1675463:
                #0  0x00000000004004ad main (/home/miouali/Documents/essai_de_codes/a.out + 0x4ad)
                #1  0x4141414141414141 n/a (n/a + 0x0)
                ELF object binary architecture: AMD x86-64
```

Nous voyons bien que l'adresse #1 a ete modifie en 0x4141414141414141 ce qui signifie "AAAAAAAA".La ligne #0 nous indique a quel endroit du programme on etait lorsque le segfault a ete recu.Nous avons bien modifie l'adresse de retour par "AAAAAAAA", et cette adresse ne mene nulle part ici, mais quelqu'un peut tres bien injecter une adresse valide d'un autre code ou pour ouvrir un shell.

Il faut neanmoins savoir que pour faire crasher ce programme, il faut rentrer une valeur >= 264, et non 256, mais pourquoi ?
Le code a ete executee sur une machine x86-64, donc apres le buffer, on trouve d'abord le RIP (8 octets), puis l'adresse de retour (8 octets).Des que on va commencer a toucher a l'adresse de retour, le programme va crasher donc ici 256 + 8 = 264.
