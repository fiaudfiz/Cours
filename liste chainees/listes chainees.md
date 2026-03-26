# Les Listes Chainees (Linked Lists)

## 1. La Philosophie : Pourquoi ça existe ?

En informatique, pour stocker une suite de données, on a deux grandes stratégies :

-L'approche "Hôtel" (**Tableau**) : On réserve 10 chambres d'un coup. Elles sont toutes collées. C'est pratique pour appeler la chambre n°5, mais si on veut une 11ème chambre et que l'hôtel est plein à côté, on doit déménager tout le monde ailleurs.

-L'approche "Jeu de piste" (**Liste Chaînée**) : Chaque personne a une chambre n'importe où dans la ville. Mais chaque personne a un papier avec l'adresse de la personne suivante. Pour trouver la 5ème personne, il faut forcément passer par la 1, puis la 2, la 3...

## 2. Anatomie d'un Nœud (Node)

Un nœud est l'unité de base. En mémoire, il se compose de deux parties distinctes :

-Le Champ de Donnée (**Data**) : La valeur utile (un entier, un flottant, une structure client...).

-Le Pointeur (**Next**) : Une variable qui stocke l'adresse mémoire du nœud suivant.

Le concept de "La Tête" (**Head**) : C'est le seul point d'entrée. C'est un pointeur qui pointe vers le premier nœud. Si on perds la tête, on perds tout le reste de la chaîne (car personne ne sait où elle commence).

## 3. Analyse de Performance : Le duel face au Tableau

Il faut bien comprendre la complexité algorithmique (O notation).

### A. L'accès aux données (Le "Lookup")

**Tableau : O(1).** C'est instantané. L'ordinateur calcule l'adresse via : adresse_debut + (index * taille_type).

**Liste : O(n).** Si on cherches le 100ème élément, on DOIS parcourir les 99 précédents. C'est le gros point faible de la liste.

### B. L'insertion et la suppression

**Tableau : O(n).** Si on veux insérer un chiffre au début d'un tableau de 1000 cases, on doit décaler les 1000 éléments d'une case vers la droite. C'est lourd.

**Liste : O(1).**

-On crée un nouveau nœud.

-On branche son "Next" sur la suite.

-On branche le "Next" du précédent sur ton nouveau nœud. Rien ne bouge en mémoire, on change juste des fils électriques.

## 4. Les différentes familles de listes

### 1. La Liste Simplement Chaînée (Singly Linked)

C'est la plus basique. On ne peut aller que vers l'avant. Si tu es au nœud 3, tu ne peux pas revenir au 2 sans recommencer depuis le début.On ne possede que un seul pointeur **next**.

Exemple : A COMPLETER

### 2. La Liste Doublement Chaînée (Doubly Linked)

Chaque nœud a deux pointeurs : **next** (suivant) et **prev** (précédent).

Avantage : On peut naviguer dans les deux sens et supprimer un nœud plus facilement.
Inconvénient : Ça consomme plus de mémoire (un pointeur en plus par nœud).

### 3. La Liste Circulaire

Le dernier nœud, au lieu de pointer vers NULL, pointe vers la Head.

Usage : Idéal pour les systèmes qui tournent en boucle (ex: un lecteur de musique qui répète une playlist, ou le temps de parole d'un processeur partagé entre plusieurs applis).Pour la parcourir, il faut donc "sauvegarder" la **head** pour ne pas boucler a l'infini.

Exemple : PUSH_SWAP

## 5. Gestion de la Mémoire (Le point critique en C)

Contrairement aux tableaux déclarés comme int tab[10] (mémoire automatique), les listes utilisent la mémoire dynamique.

Allocation : On utilise malloc. Cela réserve une place dans le Tas ([Heap](https://github.com/fiaudfiz/Cours/tree/main/stack%20heap)). Cette mémoire ne disparaît jamais toute seule.
Libération : Quand on supprime un élément, il faut faire un free(). Si on ne le fait pas, le programme consomme de plus en plus de RAM jusqu'au plantage : c'est la fuite mémoire (Memory Leak LIEN SYMBOLIQUE).

Le Segfault : C'est l'erreur n°1. Elle arrive quand tu essaies d'accéder à node->next alors que node est déjà NULL.
