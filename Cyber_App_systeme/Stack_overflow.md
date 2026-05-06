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

#### Espace User

![alt text](image.png)