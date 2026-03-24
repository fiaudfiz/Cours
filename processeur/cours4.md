## Module 4 : La Hiérarchie Mémoire (Memory Hierarchy)

### 1. Le Problème : Le "Memory Wall"

Le **CPU** va des centaines de fois plus vite que **RAM**. Pour ne pas que le **CPU** passe sa vie à attendre ses données, on utilise une structure en pyramide.

### 2. La Pyramide des Caches (Du plus rapide au plus lent)

Niveau	Emplacement	Capacité (env.)	Latence (cycles)

Registres	Dans le cœur CPU	~1 Ko	0 cycle
Cache L1	Collé au cœur	32 - 64 Ko	~4 cycles
Cache L2	Proche du cœur	256 - 512 Ko	~12 cycles
Cache L3	Partagé (tous cœurs)	8 - 64 Mo	~40 cycles
RAM	Barrette externe	16 - 64 Go	~200+ cycles




>| NOM | Localisation | Taille | Cycles |
>| :--- | :--- | :--- | :--- |
>| **Registres** | Dans le coeur CPU | ~1 Ko | 0 cycle |
>| **Cache L1** | Collé au cœur | 32 - 64 Ko | ~4 cycles |
>| **Cache L2** | Proche du coeur | 256 - 512 Ko | ~12 cycles |
>| **Cache L3** | Partagé (tous cœurs) | 8 - 64 Mo | ~40 cycles |
>| **RAM**	    | Barrette externe	| 16 - 64 Go | ~200+ cycles |


Shutterstock


3. Mécanisme : Cache Hit vs Cache Miss

Le CPU ne parle jamais directement à la RAM s'il peut l'éviter.

    Cache Hit (Succès) : La donnée est trouvée dans l'un des caches. Le CPU continue à pleine vitesse.

    Cache Miss (Échec) : La donnée n'est nulle part dans les caches. Le CPU doit "caler" (Stall) et attendre que la donnée remonte de la RAM.

4. Le Principe de Localité (Comment le cache anticipe)

Le succès d'un programme repose sur deux comportements que le cache exploite :
A. Localité Temporelle (Le "Déjà-vu")

    Concept : Si une donnée est utilisée, elle a de fortes chances d'être réutilisée très bientôt.

    Application : Les compteurs de boucles (i, j), les accumulateurs de somme.

    Gestion : Le cache garde la donnée au chaud. Si le cache est plein, il utilise l'algorithme LRU (Least Recently Used) : il évince la donnée la plus ancienne pour faire de la place.

B. Localité Spatiale (Le "Voisinage" / Cache Line)

    Concept : Si on accède à une adresse X, on accédera probablement à X+1,X+2...

    L'Unité de transfert : La Cache Line. Le CPU ne déplace jamais moins de 64 octets à la fois entre la RAM et le cache.

    Exemple : Si tu demandes un int (4 octets) à l'index tab[0], le CPU charge en réalité les 64 octets suivants. Tu reçois donc tab[0] à tab[15] dans ton cache L1 instantanément. Les 15 suivants sont "gratuits".

5. Cas d'école : Le parcours de Matrice

Une matrice int matrix[10000][10000] est stockée de manière contiguë (en un seul bloc) en RAM, ligne par ligne (Row-major order).

    Parcours par Ligne (Rapide) : En lisant matrix[0][0] puis matrix[0][1], tu profites de la Cache Line. Tu as 1 Cache Miss pour 16 accès.

    Parcours par Colonne (Lent) : En lisant matrix[0][0] puis matrix[1][0], tu sautes de 40 000 octets en RAM à chaque fois. Tu tombes systématiquement en dehors de la Cache Line actuelle.

    Résultat : 100% de Cache Miss. Le code peut être 10 à 50 fois plus lent.