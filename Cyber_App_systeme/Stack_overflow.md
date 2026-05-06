# Stack Overflow et exploitation de buffer Overflows

---

## Table des Matieres

---

Ce cours va s'appuyer sur cet article : 

Les buffers overflows representent 60% des annonces de securites du CERT () de nos jours.Le buffer overflow est le vecteur d'attaque le plus courant dans les intrusions systemes et particulierement dans les attaques a distance.
Des estimations indiquent que pour 1000 lignes de code, il y a entre 5 et 15 erreurs, ce cours va detailler tout d'abord l'organisation memoire de la stack, les potentielles exploitations des failles et quelques conseils pour eviter l'exploitation des buffer overflows.

### ELF et memoire virtuelle

memoire virtuelle : chaque programme quand il est execute obtient un espace memoire entierement isole.la memoire est adresse par mots (4 octets) et elle commence a l'adresse 0x00000000  et finit a l'adresse 0xffffffff , soit 4 Go adressables.Linux utilise pour les programmes executables le format ELF (Executable Linking Format) qui est compose de plusieurs sections.

L'espace virtuel est divise lui meme en 2 zones :
- l'espace user (0x0000000 -0xbfffffff)
- l'espace kernel (0xc0000000 - 0xfffffff)

Un processus user ne peut pas acceder a l'espace kernel mais l'inverse est possible.

Un exécutable ELF est transformé en une image processus par le program loader. 
Pour créer cette image en mémoire, le program loader va mapper en mémoire tous les loadable segments de l'exécutable et des librairies requises au moyen de l'appel système mmap(). Les exécutables sont chargés à l’adresse mémoire fixe 0x08048000 appelée « adresse de base ».

La figure suivante indique les sections principales d'un programme en memoire.La section .txt represente le code du programme.Dans la section .data sont placees les variables globales initialisees (elles sont connues a la compilation) et dans la section .bss les variables globales non-initialisees.

La stack contient les variables locales automatiques (une variable locale est automatique).Elle fonctionne sur le principe LIFO, et la stack croit vers les adresses basses, cad on commence par ex a 0xc0000000 et apres on decsend a ..... .A l'execution d'un programme, les arguments ainsi que les variables d'env sont egalement stockees dans la pile.Et les variables allouees dynamiquement (malloc, calloc...) sont stockees dans la heap. (les adresses croissent)

![alt text](image.png)

Exemple :

```c
int var;                        // bss
char var2[] = "buf1";           // data
static int var3;                // bss
char *var4                      // stack
char *var5 = malloc(32);        // heap
static char var6[] = "buf2";    // data
```

La commande size permet de connaitre les differentes sections d'un programme ELF et de leur adresse memoire

```bash
section size addr
.interp 0x13 0x80480f4
.note.ABI-tag 0x20 0x8048108
.hash 0x258 0x8048128
.dynsym 0x510 0x8048380
.dynstr 0x36b 0x8048890
8/92
.gnu.version 0xa2 0x8048bfc
.gnu.version_r 0x80 0x8048ca0
.rel.got 0x10 0x8048d20
.rel.bss 0x28 0x8048d30
.rel.plt 0x230 0x8048d58
.init 0x25 0x8048f88
.plt 0x470 0x8048fb0
.text 0x603c 0x8049420
.fini 0x1c 0x804f45c
.rodata 0x2f3c 0x804f480
.data 0xbc 0x80533bc
.eh_frame 0x4 0x8053478
.ctors 0x8 0x805347c
.dtors 0x8 0x8053484
.got 0x12c 0x805348c
.dynamic 0xa8 0x80535b8
.sbss 0x0 0x8053660
.bss 0x2a8 0x8053660
.comment 0x3dc 0x0
.note 0x208 0x0
Total 0xade9
```
(Des informations similaires mais plus détaillées peuvent être obtenues avec les
commandes readelf –e ou objdump -h). Nous voyons apparaître l’adresse en mémoire
et la taille (en bytes) des sections qui nous intéressent : .text, .data et .bss. 