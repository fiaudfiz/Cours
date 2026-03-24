# Les Tubes (Pipes) sous UNIX

---

## Table des matières

1. [Définition](#1-définition)
2. [Le concept de Flux (Stream)](#2-le-concept-de-flux-stream)
3. [Implémentation technique en C](#3-implémentation-technique-en-c)
4. [La Règle d'Or : Gestion des File Descriptors](#4-la-règle-dor--gestion-des-file-descriptors-fd)
5. [Redirection avec dup2()](#5-redirection-avec-dup2)

---

## 1. Définition

Un **pipe** (ou *tube anonyme*) est un mécanisme de **communication inter-processus (IPC) unidirectionnel**.

Il permet de **chaîner des processus** de sorte que la sortie de l'un (`stdout`) alimente directement l'entrée du suivant (`stdin`).

```
[Processus A] ──stdout──▶ [PIPE] ──stdin──▶ [Processus B]
```

---

## 2. Le concept de Flux (Stream)

Contrairement aux anciens systèmes (type MS-DOS) qui utilisaient des **Pseudo-Pipes** (stockage sur disque via des fichiers `.tmp`), UNIX utilise la **mémoire vive (RAM)**.

| Caractéristique | Pseudo-Pipes (Fichiers) | Tubes UNIX (Mémoire) |
|:----------------|:--------------------------|:------------------------|
| **Vitesse**     | Lente (accès disque)       | Ultra-rapide (RAM)      |
| **Espace**      | Consomme du stockage       | Consomme un buffer limité |
| **Parallélisme**| Séquentiel (l'un après l'autre) | Simultané (flux en continu) |

---

## 3. Implémentation technique en C

### 3.1 La création : `pipe()`

La fonction `pipe(int fd[2])` demande au système de créer un tube anonyme.  
Elle remplit un tableau de **deux descripteurs de fichiers** :

```
fd[0]  ◀──────  Lecture  (Read end)
fd[1]  ──────▶  Écriture (Write end)
```

| Descripteur | Rôle | Direction |
|:------------|:-----|:----------|
| `fd[0]`     | Le bout par lequel on **lit** les données | ← Sortie du tube |
| `fd[1]`     | Le bout par lequel on **injecte** les données | → Entrée du tube |

> **Astuce mémo :** `0` = Output (sortie/lecture), `1` = Input (entrée/écriture)

---

### 3.2 Buffering & Synchronisation

#### Capacité

Sous Linux, le buffer fait généralement **64 KB**.

#### Comportement bloquant

| Situation | Effet |
|:----------|:------|
| Buffer **plein** | Le processus qui **écrit** (`write`) est mis en pause jusqu'à ce que le lecteur libère de la place |
| Buffer **vide** | Le processus qui **lit** (`read`) est mis en pause jusqu'à ce que l'écrivain injecte des données |

####  Atomicité

> Les écritures de **moins de 4 KB** sont garanties **atomiques** (non mélangées entre processus).

---

## 4. La Règle d'Or : Gestion des File Descriptors (FD)

> ⚠️ **Dans Pipex**, après un `fork()`, les descripteurs sont **dupliqués**. Il est **impératif** de fermer les bouts inutilisés.

### Deux dangers critiques

####  Blocage infini

Un `read` ne se terminera **jamais** (ne recevra jamais `EOF`) tant qu'il reste un seul `fd[1]` (écriture) ouvert dans **n'importe quel processus**.

```
⚠️  fd[1] encore ouvert quelque part  →  read() bloqué indéfiniment
```

####  SIGPIPE

Si un processus tente d'**écrire** dans un `fd[1]` alors que **tous les `fd[0]`** (lecteurs) sont fermés → le programme **crash** (Signal Pipe).

```
⚠️  Tous les fd[0] fermés  +  write() sur fd[1]  →  SIGPIPE → crash
```

### Règle à retenir

```
Ferme toujours le bout du pipe que tu n'utilises pas,
dans chaque processus (parent ET enfant).
```

---

## 5. Redirection avec `dup2()`

Pour que les commandes shell (comme `ls` ou `wc`) utilisent le pipe **sans le savoir**, on utilise `dup2(old_fd, new_fd)`.

Cette fonction **remplace** `new_fd` par `old_fd` — le processus croit lire/écrire sur stdin/stdout, mais il utilise en réalité le pipe.

### Cas d'usage

| Appel | Effet |
|:------|:------|
| `dup2(fd[1], STDOUT_FILENO)` | Redirige la **sortie standard** vers l'**entrée du tube** |
| `dup2(fd[0], STDIN_FILENO)`  | Redirige l'**entrée standard** vers la **sortie du tube** |

### Schéma récapitulatif

```
[cmd1]
  stdout  →  dup2(fd[1], STDOUT)  →  fd[1]  →  [PIPE BUFFER]  →  fd[0]  →  dup2(fd[0], STDIN)  →  stdin  →  [cmd2]
```

---

## Récapitulatif général

```
pipe()          →  Crée le tube (fd[0] lecture, fd[1] écriture)
fork()          →  Duplique le processus (et les FDs !)
dup2()          →  Branche stdin/stdout sur le pipe
close()         →  Ferme les FDs inutilisés (OBLIGATOIRE)
exec()          →  Lance la commande dans l'enfant
```

---
