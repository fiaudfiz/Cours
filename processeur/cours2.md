# Module 2 — L'Exécution : De la Syntaxe au Signal

---

## Table des matières

1. [La Pierre de Rosette — ISA](#1-la-pierre-de-rosette--isa-instruction-set-architecture)
2. [Le Registre PC](#2-le-chef-de-chantier--le-registre-pc)
3. [Fetch → Decode → Execute → Store](#3-le-cycle-de-vie--fetch--decode--execute--store)
4. [La Logique : `if` et `while`](#4-la-logique--comment-fonctionnent-if-et-while-)

---

Dans le Module 1, nous avons vu la machine (**Hardware**). Ici, nous voyons comment le code (**Software**) prend le contrôle de la machine.

---

## 1. La Pierre de Rosette : ISA *(Instruction Set Architecture)*

C'est le concept fondamental pour comprendre la portabilité (ou non) d'un code. Le CPU possède un vocabulaire limité et "câblé" dans ses circuits : c'est son **Jeu d'Instructions (ISA)**.

| Niveau | Exemple | Caractéristique |
| :--- | :--- | :--- |
| **C** *(haut niveau)* | `a = b + c;` | Portable — compilable pour n'importe quelle machine. |
| **Assembleur** *(ISA)* | `ADD R1, R2, R3` | Correspond 1 pour 1 à ce que le CPU sait faire. Spécifique à la machine. |
| **Code machine** *(binaire)* | `01001011 ...` | Les tensions électriques que le CPU "mange" réellement. |

> **Rôle du compilateur** (`gcc`, `clang`...) : traduire le C portable en ISA spécifique — `x86` pour un PC, `ARM` pour un téléphone.
> Copier un `.exe` (binaire x86) sur un Raspberry Pi (ARM) ne fonctionne pas : ils ne parlent pas la même langue.

---

## 2. Le Chef de Chantier : Le Registre PC

Parmi tous les registres du CPU, l'un d'eux est le **maître absolu du temps**.

- **Nom** : `PC` (*Program Counter*), ou `RIP` (*Instruction Pointer*) sur les systèmes Intel 64 bits.
- **Rôle** : contient l'adresse mémoire de la **prochaine instruction** à exécuter.
- **En cas de crash** *(Segfault)* : un *Segmentation Fault* survient souvent parce que le PC a tenté de lire une instruction à une adresse interdite (ex. : pointeur nul). Le debugger fournit la valeur du `PC`/`RIP` au moment du plantage — c'est l'endroit exact où le code a cassé.

---

## 3. Le Cycle de Vie — *Fetch → Decode → Execute → Store*

Pour exécuter une seule instruction, le processeur répète inlassablement cette boucle.

### Étape 1 — Fetch *(Rechercher)*

1. Le CPU consulte le registre `PC` (ex. : adresse `0x1004`).
2. Il envoie cette adresse sur le **Bus** vers la RAM (ou le Cache).
3. La RAM renvoie le contenu : l'instruction binaire.
4. Le CPU stocke cette instruction dans le registre **IR** (*Instruction Register*).
5. Le CPU incrémente le PC : `PC ← PC + taille_instruction` pour pointer vers la suite.

### Étape 2 — Decode *(Comprendre)*

1. L'Unité de Contrôle lit les bits dans l'**IR**.
2. Elle reconnaît le motif (**Opcode**). Ex. : *"C'est une addition."*
3. Elle identifie les données nécessaires. Ex. : *"Je dois utiliser le Registre A et le Registre B."*

### Étape 3 — Execute *(Agir)*

1. Les données filent vers l'**UAL**.
2. L'UAL effectue le calcul.
3. Le résultat est écrit dans le registre de destination.
4. Les **Flags** sont mis à jour (voir section suivante).

### Étape 4 — Store *(Stocker)*

Si l'instruction le requiert, le résultat est écrit en RAM.

---

## 4. La Logique : Comment fonctionnent `if` et `while` ?

Le cycle ci-dessus est linéaire. Pour gérer des conditions, il faut pouvoir **briser la ligne d'exécution**. C'est le rôle des **Sauts** (*Jumps*) et des **Drapeaux** (*Flags*).

### Les Drapeaux *(Flags)*

L'UAL ne fait pas que calculer — elle rapporte l'état du résultat dans un registre spécial (**EFLAGS**).

| Flag | Nom | Condition |
| :--- | :--- | :--- |
| `ZF` | *Zero Flag* | Le résultat est-il égal à `0` ? |
| `SF` | *Sign Flag* | Le résultat est-il négatif ? |

### Exemple concret

**Code C :**

```c
if (a == 0)
{
    goto error;
}
b = 10;
```

**Ce que le CPU exécute (assembleur simplifié) :**

```asm
CMP R1, 0       ; Compare R1 avec 0
```
> L'UAL effectue une soustraction fictive (`R1 − 0`). Si le résultat est `0`, elle allume le Zero Flag : `ZF = 1`.

```asm
JE 0x500        ; Jump if Equal
```
> Le CPU consulte le Zero Flag.
> - **Si `ZF = 1`** : le CPU écrase le registre `PC` avec l'adresse `0x500`. Au prochain cycle, il ira chercher l'instruction là-bas.
> - **Sinon** : il ne fait rien et continue tout droit.

```asm
MOV R2, 10      ; Exécuté seulement si le saut n'a pas eu lieu
```

---