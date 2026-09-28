# La Thread Pool

---

## Table des matières

1. [Définition](#définition)
2. [Les contraintes](#les-contraintes)
3. [L'arrêt](#larrêt)
4. [Combien de threads créer ?](#combien-de-threads-créer-)
5. [Récupérer le résultat d'une tâche](#récupérer-le-résultat-dune-tâche)
6. [La file bornée](#la-file-bornée)
7. [Squelette complet en C](#squelette-complet-en-c)
8. [Le piège du `submit` récursif](#le-piège-du-submit-récursif)
9. [Le work stealing (pour aller plus loin)](#le-work-stealing-pour-aller-plus-loin)

---

## Définition

Qui a déjà essayé de séparer les tâches d'un projet en différents threads, pour se rendre compte qu'au bout d'un moment, on a trop de threads et qu'on perd en performance ?

**Solution : la thread pool.** On crée N threads une seule fois, et on met en place une file d'attente pour les tâches. Les threads piochent dans cette file partagée, au lieu d'être créés et détruits à chaque tâche.

```plaintext
producteur -> [ file de tâches ] -> worker 1..N
```

Une thread pool est constituée de 3 choses :

- Les threads (workers)
- Une file de tâches partagée
- Une synchronisation (condition variable + mutex)

---

## Les contraintes

Dans une thread pool, il faut réussir à partager la file de tâches entre tous les threads, les synchroniser, et faire dormir un thread tant que la file d'attente est vide.

Pour éviter qu'un thread ne se réveille pour rien, on utilise une boucle `while` (et non un simple `if`) autour du `cond_wait`. Deux raisons :

1. **Un autre thread peut avoir pris la tâche** entre le réveil et l'exécution : la condition doit être re-vérifiée une fois le mutex re-acquis.
2. **Les réveils parasites (*spurious wakeups*)** : la spécification POSIX autorise `pthread_cond_wait` à retourner sans qu'aucun signal n'ait été envoyé. La boucle `while` re-vérifie donc la condition à chaque réveil.

```c
pthread_mutex_lock(&mutex);
while (file_vide(&file))                 // JAMAIS "if"
    pthread_cond_wait(&not_empty, &mutex);
```

---

## L'arrêt

À la fin du programme, on veut que la pool s'arrête. Problème : on a vu que les threads dorment tant qu'on ne leur donne pas de tâche, ce qui peut donner un programme qui ne se termine jamais.

Deux solutions :

- **Un flag `shutdown`** partagé entre tous les threads et protégé par le mutex de la file.
- **Des tâches spéciales (« poison pills »)** : on pousse N tâches spéciales dans la file, une par thread, et chaque thread qui en reçoit une s'éteint silencieusement.

### Le piège classique du flag `shutdown`

Lever le flag ne suffit pas : les threads qui dorment dans `cond_wait` ne le verront jamais tant qu'on ne les réveille pas. Il faut donc :

1. Lever le flag **sous mutex**.
2. Faire un **`pthread_cond_broadcast`** (pas un simple `signal`) pour réveiller *tous* les workers.
3. Modifier la condition de la boucle : `while (file vide && !shutdown)`.
4. Après la boucle, tester : si `shutdown` et file vide, le worker sort et se termine.
5. Le main appelle `pthread_join` sur chaque worker pour attendre leur fin.

```c
// Côté arrêt
pthread_mutex_lock(&pool->mutex);
pool->shutdown = true;
pthread_cond_broadcast(&pool->not_empty);   // réveille tous les workers
pthread_cond_broadcast(&pool->not_full);    // réveille aussi les producteurs bloqués
pthread_mutex_unlock(&pool->mutex);

for (int i = 0; i < pool->nb_threads; i++)
    pthread_join(pool->threads[i], NULL);
```

### Drain ou abandon ?

Il faut décider ce qu'on fait des tâches encore dans la file au moment de l'arrêt :

| Stratégie | Comportement | Conséquence |
| --- | --- | --- |
| **Drain** | Les workers finissent toutes les tâches restantes, puis s'arrêtent | Aucune tâche perdue, l'arrêt peut être long |
| **Abandon** | Les workers s'arrêtent dès que possible, la file est vidée sans exécution | Arrêt rapide, mais les tâches jamais exécutées ont un `future` jamais complété |

Attention avec l'abandon : pour chaque tâche jetée, il faut **compléter son future avec une erreur** (par exemple `ECANCELED`) et le signaler, sinon un `future_get` bloquera à jamais (deadlock silencieux, voir plus bas).

---

## Combien de threads créer ?

Le nombre de threads idéal dépend de la machine et du type de projet visé. Pour le déterminer, il faut d'abord connaître quelques définitions :

| Terme | Définition |
| --- | --- |
| Cœur physique | Vraie unité de calcul du processeur |
| Thread matériel | Contexte d'exécution exposé à l'OS. Avec l'hyperthreading (SMT), 1 cœur physique = 2 threads matériels |
| Thread logiciel | Créé par `pthread_create`, on peut en créer des milliers |

Seul un nombre limité de threads peuvent s'exécuter en même temps : les autres attendent dans les files du scheduler de l'OS, qui les fait tourner à tour de rôle (*context switch*).

### Bien connaître sa machine

- shell : `nproc` (threads matériels), `lscpu` (détail)
- C/POSIX : `sysconf(_SC_NPROCESSORS_ONLN)`
- C++ : `std::thread::hardware_concurrency()`
- Java : `Runtime.getRuntime().availableProcessors()`
- Python : `os.cpu_count()`

**Attention** : les 2 threads d'un même cœur se partagent les ressources. Pour du calcul pur, le SMT apporte typiquement un gain de **20 à 30 %** seulement : 12 threads matériels valent plutôt **environ 7 à 8 cœurs**, pas 12 (ordre de grandeur, à mesurer sur sa machine).

### Pourquoi un worker attend ?

Une tâche dépend souvent de quelque chose d'extérieur (réseau, disque, base de données...), selon le domaine dans lequel la pool est utilisée (cyber, projet graphique, blockchain, etc.). Or ces éléments sont beaucoup plus lents que le CPU : le worker est donc parfois bloqué en attendant des informations pour continuer à calculer.

| Opération | Ordre de grandeur |
| --- | --- |
| Opération CPU | ~1 nanoseconde |
| Accès disque dur | 0,1 à 10 millisecondes |
| Aller-retour réseau | 10 à 100 millisecondes |

Pendant l'attente, l'OS endort le thread : **il ne consomme pas de CPU**, et sa place est libre pour un autre.

- **CPU-bound** : la tâche calcule tout le temps (calcul, rendu, compression...)
- **I/O-bound** : la tâche passe l'essentiel de son temps à attendre (réseau, disque, base de données)

### Nombre de threads idéal

Objectif : que tous les threads matériels calculent en permanence.

```
N_threads = N_cpu * (temps_total / temps_calcul)
```

**D'où vient cette formule ?** Si un thread ne calcule que 10 % du temps (`temps_total / temps_calcul = 10`), il laisse son cœur libre 90 % du temps : il faut donc 10 threads comme lui pour occuper un cœur en permanence. Cette formule est popularisée par Brian Goetz dans *Java Concurrency in Practice*.

Ce nombre est le **maximum utile** de threads : au-delà, on ne gagne plus en performance, on en perd même !

### Exemple sur 12 threads matériels

Ce tableau présente le nombre maximum de threads idéal selon le type de tâche (ce n'est qu'un exemple : il est bon de vérifier soi-même le ratio pour connaître le nombre idéal pour son projet).

| Type de tâche | Calcul | Attente | N |
| --- | --- | --- | --- |
| CPU-bound pur | 100 % | 0 % | 12 × 1 = 12 |
| Mixte | 50 % | 50 % | 12 × 2 = 24 |
| Mixte | 20 % | 80 % | 12 × 5 = 60 |
| I/O-bound | 10 % | 90 % | 12 × 10 = 120 |

Selon le projet, la pool passe de 12 à 120 threads : c'est considérable et cela peut vraiment impacter les performances.

### Mesurer le ratio

Pour savoir combien de threads implémenter, il faut connaître le ratio entre le temps où le thread calcule et celui où il attend. Deux méthodes, un même calcul :

`ratio de calcul = temps CPU / temps réel`

#### Méthode 1 : dans le code

On ajoute dans son code des appels pour mesurer, **par tâche**, le temps réel et le temps CPU du thread :

- Temps réel : `clock_gettime(CLOCK_MONOTONIC, ...)`
- Temps CPU du thread : `clock_gettime(CLOCK_THREAD_CPUTIME_ID, ...)`

```c
struct timespec r0, r1, c0, c1;
clock_gettime(CLOCK_MONOTONIC, &r0);
clock_gettime(CLOCK_THREAD_CPUTIME_ID, &c0);

execute(tache);

clock_gettime(CLOCK_MONOTONIC, &r1);
clock_gettime(CLOCK_THREAD_CPUTIME_ID, &c1);
// ratio = (c1 - c0) / (r1 - r0)
```

C'est la méthode la plus fiable, car elle mesure directement le ratio d'**une tâche**.

#### Méthode 2 : avec le shell

`time ./programme` donne `real`, `user` et `sys`.

- **Programme mono-thread** :
  - `user + sys ≈ real` : CPU-bound
  - `user + sys < real` : I/O-bound
- **Programme multi-thread** : `user + sys` cumule le temps de tous les cœurs et **peut dépasser `real`**. Le ratio se calcule alors avec :

```
ratio ≈ (user + sys) / (real × nb_threads_actifs)
```

Pour un programme multi-thread, on obtient une meilleure mesure en lançant le programme avec **un seul worker** : le `time` redevient lisible comme en mono-thread.

Les deux méthodes donnent un **point de départ, pas une vérité** : le ratio est approximatif et varie selon la charge. Trop de threads coûte cher : stack de 1 à 8 Mo par thread, context switches, contention sur le mutex de la file.

> **Remarque** : pour de l'I/O très massive (des dizaines de milliers de connexions simultanées, le fameux problème **C10K**), les threads ne passent plus à l'échelle : on utilise l'asynchrone (`epoll`, async/await, event loop).

---

## Récupérer le résultat d'une tâche

**Problème** : le thread exécute une tâche et cette fonction calcule quelque chose. Comment celui qui a lancé la tâche peut-il récupérer le résultat (ou un éventuel code d'erreur), et comment sait-il que la tâche est terminée ?

On implémente une `struct` par tâche, appelée **future**, qui contient les informations nécessaires :

```c
typedef struct s_future
{
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    bool            ready;    // la tâche est-elle terminée ?
    void            *result;  // valeur retournée (NULL si échec)
    int             error;    // 0 = ok, sinon code d'erreur
}   t_future;
```

Pour comprendre à quoi servent cette struct et ses variables, décortiquons le flux complet d'une tâche.

### Étape 1 : Soumission (main)

- `submit(fonction, arg)` vérifie le flag `shutdown` (refuse si la pool s'arrête)
- crée le future (`ready = false`, `error = 0`)
- `lock(mutex_pool)`, push `{fonction, arg, future}`, `signal(cond_pool)`, `unlock`
- retourne le future au main, qui continue son code

### Étape 2 : Prise en charge (thread)

- `lock(mutex_pool)`
- `while (file vide && !shutdown) cond_wait(cond_pool, mutex_pool)`
- `pop` la tâche
- `unlock(mutex_pool)` (on relâche **avant** d'exécuter, sinon plus de parallélisme)

### Étape 3 : Exécution (thread)

- exécute `fonction(arg)`, **sans tenir aucun mutex**

### Étape 4 : Dépôt du résultat (thread)

- `lock(f.mutex)`
- Succès : `f.result = valeur` / Échec : `f.error = code`
- `f.ready = true`
- `cond_signal(f.cond)`
- `unlock(f.mutex)`

### Étape 5 : Récupération (main)

Le main peut appeler `future_get` **à n'importe quel moment** : avant la fin de la tâche (il dormira), pendant son exécution, ou même après (il repartira immédiatement). C'est exactement le rôle du `while (!f.ready)`.

- `future_get(f)` : `lock(f.mutex)`
- `while (!f.ready) cond_wait(f.cond, f.mutex)`
- lit `result` et `error`, puis `unlock(f.mutex)`
- `error != 0` : remonter l'erreur, sinon retourner `result`

### Gestion des erreurs

- Si la tâche échoue, le thread remplit `error`, laisse `result` vide, et met **quand même** `ready = true` + signal. Sinon le main dort à jamais dans `future_get` (**deadlock silencieux**).
- C'est le main qui décide quoi faire de l'erreur (réessayer, abandonner, logger), pas le thread.
- Langages avec exceptions : le worker capture l'exception, la stocke dans le future, et `get()` la relance chez le producteur (Java : `ExecutionException`).
- N'oublie pas les tâches **jamais exécutées** (arrêt en mode abandon) : leur future doit aussi être complété avec une erreur.

---

## La file bornée

Imaginons un main qui produit 1000 tâches/s alors que les threads n'en traitent que 200/s : chaque seconde, la file grossit de 800 tâches, et elle n'a aucune limite. À terme, la mémoire explose.

On doit donc **borner** la file : on fixe une capacité max (ex. 1000 tâches). Reste à décider **que faire quand la file est pleine** et qu'une nouvelle tâche arrive. Trois stratégies :

| Solution | Comportement | Quand l'utiliser | Exemple |
| --- | --- | --- | --- |
| **Bloquer** | `submit` attend qu'une place se libère | On ne veut rien perdre, donc on ralentit le main | Traitement de fichiers par lots |
| **Rejeter** | `submit` retourne une erreur | Le main décide de réessayer ou d'abandonner | Une API web qui répond `503 Service Unavailable` |
| **Jeter** | On supprime une tâche (la plus ancienne ou la plus nouvelle) | Données périmées sans intérêt | Frames d'un jeu vidéo |

Le terme **back pressure** vient de là : la file pleine « repousse » le main et le force à ralentir, au lieu de laisser la file s'accumuler.

La stratégie **bloquer** est la plus courante et la plus simple à mettre en place : on fait juste dormir le main tant que la file est pleine.

Il y a maintenant **deux raisons de dormir** :

- une file vide (le worker dort, sur `not_empty`)
- une file pleine (le producteur dort, sur `not_full`)

Toutes deux sont protégées par le **même mutex** (celui de la file).

```c
submit(tâche):
    lock(mutex)
    while (file pleine && !shutdown)
        cond_wait(not_full, mutex)
    if (shutdown) { unlock(mutex); return ERREUR; }
    push(tâche)
    cond_signal(not_empty)
    unlock(mutex)

worker:
    lock(mutex)
    while (file vide && !shutdown)
        cond_wait(not_empty, mutex)
    if (file vide && shutdown) { unlock(mutex); return; }
    tâche = pop()
    cond_signal(not_full)
    unlock(mutex)
    execute(tâche)
```

### Choisir la capacité

Une bonne base est un multiple du nombre de threads (entre **×2 et ×10**), à ajuster selon le projet **en mesurant** :

- trop petite : les workers risquent de manquer de travail, le producteur est bloqué trop souvent ;
- trop grande : on perd le bénéfice de la back pressure et on consomme de la mémoire.

---

## Le piège du `submit` récursif

Que se passe-t-il si une **tâche appelle elle-même `submit`** sur la même pool, avec une file bornée en mode « bloquer » ?

Scénario : la pool a 4 workers et une file de capacité 4. Les 4 workers exécutent chacun une tâche qui soumet une sous-tâche, alors que la file est pleine. Tous les workers dorment alors sur `not_full`, personne ne consomme la file : **deadlock**.

Pistes de solution :

- **Ne jamais bloquer depuis un worker** : utiliser la stratégie *rejeter* pour les soumissions faites depuis un worker.
- **Utiliser une file non bornée** pour les sous-tâches (au risque de la mémoire).
- **Faire exécuter la sous-tâche directement** par le worker courant si la file est pleine (*caller-runs policy*, comme le fait Java avec `CallerRunsPolicy`).
- Pour du fork/join massif, passer au **work stealing** (voir ci-dessous), conçu pour ce cas.

Même problème avec `future_get` appelé **depuis un worker** sur une tâche encore dans la file : si tous les workers attendent le résultat d'une tâche que personne ne peut exécuter, c'est le même deadlock.

---

## Le work stealing (pour aller plus loin)

Avec une file unique partagée, tous les threads se disputent le même mutex à chaque `pop`. Avec 12 threads et des tâches très courtes, ce mutex devient un **goulot d'étranglement** : les threads passent plus de temps à attendre le lock qu'à travailler.

**Solution :**

- Une file d'attente **par thread** (plus de mutex global partagé)
- Un thread prend les tâches de **sa propre file**, sans conflit avec les autres
- Quand sa file est vide, il va **voler** une tâche dans la file d'un autre thread

> **À n'implémenter que si les tâches n'ont pas d'ordre précis entre elles.**

Attention : le vol a un coût (synchronisation sur la file de la victime). Il ne vaut le coup que si la **contention sur le mutex global est réelle et mesurée** ; pour une pool de quelques threads avec des tâches longues, la file unique reste plus simple et suffisante.

### Les détails malins

- Le **propriétaire** prend les tâches à un bout de sa file (LIFO : la tâche la plus récente, dont les données sont encore dans le cache CPU). Le **voleur** prend à l'autre bout (la plus ancienne, souvent la plus « grosse »). Résultat : propriétaire et voleur se croisent rarement, donc peu de contention.
- Quand une tâche en génère d'autres (**fork/join**), elles vont dans la file locale du worker qui l'exécute : bonne localité (cache) et pas de mutex global.
- Les structures utilisées sont souvent **lock-free** (deque de Chase-Lev), pour éviter même les mutex par file.

