# Module 1 sur le processeur — L'Infrastructure Physique du Calcul

---

## Table des matières

1. [L'Horloge](#1-le-rythme--lhorloge-clock)
2. [Le Transistor](#2-la-matière--le-transistor)
3. [Le Modèle de Von Neumann](#3-lorganisation--le-modèle-de-von-neumann-1945)
4. [Anatomie Interne du CPU](#4-anatomie-interne-du-cpu)

---

Pour qu'un ordinateur exécute du code, il doit convertir de l'énergie (électrons) en logique (mathématiques). Cette transformation repose sur un rythme et une structure précise.

---

## 1. Le Rythme : L'Horloge *(Clock)*

Le processeur ne travaille pas en continu, mais par "sauts". L'horloge est un composant qui génère un signal électrique oscillant.

- **Signal** : une alternance entre une tension basse (`0`) et une tension haute (`1`).
- **Battement** *(front montant)* : le moment précis où le signal passe de `0` à `1`. C'est ce signal qui déclenche le mouvement des données dans tous les circuits simultanément.
- **Fréquence** : le nombre de battements par seconde, mesuré en Hertz (Hz).
  - `1 Hz` = 1 battement / seconde
  - `3 GHz` = 3 milliards de battements / seconde
- **Cycle** : l'intervalle de temps entre deux battements. Sur un CPU à 3 GHz, un cycle dure **0,33 nanoseconde** — c'est le temps qu'a le courant pour traverser les circuits avant le prochain battement. C'est la **limite de vitesse absolue** de la machine.

---

## 2. La Matière : Le Transistor

C'est l'unité de base de l'intelligence matérielle. Gravés sur une plaque de silicium, ils sont **plusieurs milliards** dans un processeur moderne.

- **Définition** : un interrupteur microscopique sans partie mécanique, contrôlé par une tension électrique.
- **Logique binaire** :
  - Courant passe → `1` (Vrai)
  - Courant bloqué → `0` (Faux)
- **Portes logiques** : en combinant quelques transistors, on crée des portes (`AND`, `OR`, `NOT`) qui permettent de réaliser des additions ou des comparaisons.

---

## 3. L'Organisation : Le Modèle de Von Neumann *(1945)*

Presque tous les ordinateurs (PC, smartphones) utilisent cette structure. Elle repose sur la **séparation du calcul et du stockage**, reliés par des **bus** (autoroutes de données).

| Composant | Rôle |
| :--- | :--- |
| **CPU** *(Central Processing Unit)* | Le cerveau. Traite les instructions les unes après les autres. |
| **RAM** *(Mémoire)* | L'entrepôt. Instructions du programme et variables sont stockées **au même endroit**, sous forme de nombres. |
| **I/O** *(Entrées / Sorties)* | Clavier, disque dur, écran, réseau. |

> **Goulot d'étranglement** : le CPU est des centaines de fois plus rapide que la RAM. L'un des plus grands défis de l'informatique est d'éviter que le CPU reste "les bras croisés" à attendre ses données.

---

## 4. Anatomie Interne du CPU

Le processeur est divisé en deux zones qui communiquent en permanence.

### 4.1 Le Chemin de Données *(Datapath)* — "L'Atelier"

C'est ici que l'on manipule la matière.

- **UAL** *(Unité Arithmétique et Logique)* : la calculatrice du CPU. Elle prend deux valeurs, applique une opération (addition, multiplication, comparaison bit à bit) et renvoie le résultat.
- **Registres** : des cases mémoire situées **à l'intérieur du CPU**, sur le même circuit que l'UAL.
  - Très peu nombreux (ex. : 16 ou 32 registres sur un processeur x86-64).
  - **Différence avec la [Stack](https://github.com/fiaudfiz/Cours/tree/main/stack%20heap)** : la [Stack](https://github.com/fiaudfiz/Cours/tree/main/stack%20heap)) se trouve en **RAM**. Les registres sont encore plus proches et plus rapides.
  - Un accès registre → **1 cycle**. Un accès RAM (même la stack) → **des dizaines de cycles**.

### 4.2 L'Unité de Contrôle *(Control Unit)* — "Le Contremaître"

Elle ne calcule rien — elle **dirige**.

1. Elle va chercher l'instruction en RAM.
2. Elle la décode.
3. Elle envoie les ordres électriques aux composants :
   > *"Registre A, envoie ta valeur à l'UAL. Registre B, fais de même. UAL, additionne-les !"*

---