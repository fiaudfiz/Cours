# Cours — Le Fork *(Clonage de Processus)*

---

## 1. Définition

`fork()` est l'appel système pilier de la gestion des processus sous UNIX. Il permet à un processus (le **Père**) de créer une copie conforme de lui-même (le **Fils**) dans un nouvel espace mémoire.

---

## 2. Le Mécanisme Copy-on-Write *(COW)*

Contrairement à ce qu'on pourrait penser, `fork()` ne duplique **pas** immédiatement toute la RAM du père — ce qui serait très lent.

1. Le système **partage d'abord** la même mémoire physique entre père et fils.
2. La copie réelle ne se déclenche que si l'un des deux tente de **modifier** une donnée.
3. Résultat : `fork()` est **extrêmement rapide**.

---

## 3. Valeurs de Retour — *Le Sélecteur*

C'est le seul moment en C où une fonction renvoie **deux valeurs différentes au même instant** — une dans chaque processus.

| Processus | Valeur de retour de `fork()` |
| :--- | :--- |
| **Père** | Le PID du fils *(nombre > 0)* |
| **Fils** | `0` |
| **Erreur** | `-1` *(le système ne peut plus créer de processus)* |

```c
pid_t pid = fork();

if (pid == 0)
    // Code exécuté par le fils
else if (pid > 0)
    // Code exécuté par le père
else
    // Erreur
```

---

## 4. L'Héritage — *Ce qui est copié*

Le fils hérite de presque tout du père. Points clés pour **[Pipex](https://github.com/fiaudfiz/Pipex)** :

| | Élément | Comportement |
| :---: | :--- | :--- |
| ✅ | **Descripteurs de fichiers** *(FD)* | Le fils accède aux mêmes fichiers et pipes ouverts que le père. |
| ✅ | **Variables** | Même valeur au moment du `fork`, puis **indépendantes** ensuite. |
| ❌ | **PID** | Le fils possède son propre identifiant unique. |

---

## 5. Synchronisation et Cycle de Vie

### `wait` & `waitpid` — Attendre ses fils

Un père ne doit jamais laisser ses fils mourir sans s'en occuper.

- `wait(int *status)` — attend qu'**un** fils quelconque se termine.
- `waitpid(pid, &status, options)` — attend un **fils spécifique** *(crucial si tu gères plusieurs commandes)*.

> **Pourquoi ?** Pour récupérer le **code de sortie** (*exit status*) et libérer les ressources système.

---

### Les Zombies

Un processus **Zombie** est un fils qui a terminé (`exit`), mais dont le père n'a pas encore appelé `wait()`.

- Il reste une entrée dans la table des processus du noyau.
- **Danger** : trop de zombies peuvent saturer le système et bloquer la création de nouveaux processus.

### Les Orphelins

Si le père meurt avant le fils, le fils devient un **Orphelin**. Il est automatiquement adopté par le processus `init` *(PID 1)*, qui se chargera d'appeler `wait()` à sa place.

---

## 6. Application dans Pipex

`fork()` permet à chaque commande (`cmd1`, `cmd2`) de s'exécuter dans son propre espace mémoire.

```
1. Le Père crée le pipe.
2. Le Père fork → premier fils  (exécute cmd1).
3. Le Père fork → deuxième fils (exécute cmd2).
4. Le Père ferme ses pipes et attend ses deux fils avec waitpid.
```

---