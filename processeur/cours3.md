## Module 3 : Les Aléas du Pipeline (Pipeline Hazards)

### 1. Les Concepts Clés

**Pipeline** : L'autoroute du **CPU**. Technique qui permet de lancer une instruction avant que la précédente ne soit finie (parallélisme au niveau des instructions).
**Stall** (ou "Bulle") : Quand le **pipeline** doit se mettre en pause. C'est un cycle perdu où le CPU ne produit rien.

### 2. Les 3 Types d'Aléas (Les Ennemis)

#### A. L'Aléa Structurel (Resource Hazard)

C'est quoi ? Une "bagarre" de matériel. Deux instructions essaient d'utiliser le même morceau du processeur au même moment (ex: une instruction veut être lue en mémoire pendant qu'une autre veut écrire une donnée en mémoire).

Solution Hardware : Architecture de Harvard (modifiée).
On sépare physiquement les caches : L1 Instruction (pour le code) et L1 Data (pour les variables). Comme ça, on peut charger du code et lire une variable en même temps sans bouchon.

#### B. L'Aléa de Données (Data Hazard) - Le plus courant

C'est quoi ? Une dépendance. L'instruction B a besoin du résultat de l'instruction A, mais A n'a pas encore fini de calculer.
Exemple : int a = 1 + 1; int b = a * 2; (B doit attendre que A ait écrit la valeur de a).

Conséquence : Le CPU doit insérer des Bulles (**Stalls**). Il gèle l'instruction B pendant 1 ou 2 cycles le temps que A finisse.

#### C. L'Aléa de Contrôle (Control Hazard) - Le plus coûteux

C'est quoi ? L'incertitude du if. Le **CPU** charge les instructions à la suite, mais face à un saut (conditionnel if ou inconditionnel goto), il ne sait pas où aller tant que la condition n'est pas calculée.
Le Mécanisme : Le **Branch Predictor** (le parieur). Il devine le chemin et continue d'exécuter de manière spéculative.

Conséquence (Si erreur) : Le **Pipeline Flush**.
Le **CPU** doit jeter à la poubelle toutes les instructions chargées par erreur. C'est une perte énorme de cycles (pénalité de mauvaise prédiction).

### 3. Les Super-Solutions (Comment on supprime les Stalls)

#### A. Le Forwarding (ou Bypassing)

Pour quel problème ? L'Aléa de Données.

Le principe : Au lieu d'attendre que le résultat soit écrit officiellement dans le registre (étape Write-Back, à la fin), on le récupère dès qu'il sort de la calculatrice (**ALU**).

Le terme technique pour le "câble" : Cela s'appelle un Multiplexeur de Forwarding (ou **Forwarding Unit**). Il court-circuite le chemin normal pour relier directement la sortie de l'étage EX (Execute) de l'instruction A vers l'entrée de l'étage EX de l'instruction B.

#### B. L'Exécution dans le Désordre (Out-of-Order Execution - OoO)

Pour quel problème ? Tous les blocages qui durent longtemps (ex: attendre la **RAM**).
Le principe : Le **CPU** possède une "fenêtre d'instructions". Si l'instruction actuelle est bloquée, il regarde plus loin dans le code pour trouver des instructions indépendantes (qui n'ont pas besoin des résultats en attente) et les exécute tout de suite.

Analogie : À la caisse du supermarché, si le client devant toi a oublié de peser ses bananes, la caissière ne s'arrête pas : elle scanne les articles du client suivant en attendant.