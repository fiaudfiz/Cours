# Module 6 — Mémoire Virtuelle, MMU & Système d'Exploitation

---

## Table des matières

1. [L'Espace d'Adressage Virtuel](#1-lespace-dadressage-virtuel-virtual-address-space--vas)
2. [La MMU *(Memory Management Unit)*](#2-la-mmu-memory-management-unit--le-traducteur)
3. [La Pagination](#3-la-pagination-paging--le-découpage)
4. [La Page Table](#4-la-page-table-table-des-pages)
5. [Le Page Fault](#5-le-page-fault-linterruption)

---

## 1. L'Espace d'Adressage Virtuel *(Virtual Address Space — VAS)*

Chaque programme tourne dans sa propre **réalité virtuelle**.

- **Adresse virtuelle** : un simple numéro (index) utilisé par le programme. Quand un programme demande l'adresse `0x100`, il ne demande *pas* la 256ème case de la barrette de RAM — il demande la 256ème case de son monde imaginaire.
- **Adresse physique** : l'emplacement réel des électrons dans la puce de RAM.

**Pourquoi ce niveau d'abstraction ?**

| Bénéfice | Explication |
| :--- | :--- |
| **Isolation** | Si le Programme A bugue et écrit à `0x100`, il ne peut pas écraser les données du Programme B — leur `0x100` pointe ailleurs dans la vraie RAM. |
| **Continuité** | Un programme peut croire disposer d'1 Go de mémoire d'un seul bloc, alors que l'OS lui a en réalité alloué 1000 petits morceaux éparpillés sur la barrette. |

---

## 2. La MMU *(Memory Management Unit)* — Le Traducteur

Composant physique (silicium) situé entre le CPU et la RAM. Son rôle : traduire en temps réel chaque adresse virtuelle en adresse physique.

> Quand le CPU dit *"Donne-moi ce qu'il y a à l'adresse virtuelle `0xABC`"*, la MMU consulte son dictionnaire et ordonne à la RAM : *"Envoie le contenu de la case physique `0x999` !"*

### Le TLB *(Translation Lookaside Buffer)*

Le cache personnel de la MMU. Comme consulter la Page Table en RAM est lent, la MMU conserve ses dernières traductions en mémoire.

| Cas | Résultat |
| :--- | :--- |
| **TLB Hit** | Traduction instantanée. |
| **TLB Miss** | La MMU doit aller lire la Page Table en RAM — environ **200 cycles** de pénalité. |

---

## 3. La Pagination *(Paging)* — Le Découpage

L'OS ne gère pas la mémoire octet par octet (le dictionnaire de traduction serait trop volumineux). Il découpe tout en **Pages**.

- **Page** *(virtuelle)* : bloc de **4096 octets** (4 Ko) — l'unité de base de la mémoire.
- **Cadre** *(Page Frame — physique)* : la "case" de 4 Ko dans la vraie RAM.

> **Analogie** : la mémoire est un livre.
> - L'adresse virtuelle, c'est *"Page 4, ligne 10"*.
> - La MMU traduit ça en *"Étagère 2, Rayon B, Volume 5"*.
> - On ne peut pas emprunter juste une ligne — l'OS donne forcément la page entière.

---

## 4. La Page Table *(Table des Pages)*

Le "dictionnaire" stocké en RAM, contenant la liste des correspondances virtuel → physique. C'est aussi un **garde-barrière** : pour chaque page, il stocke des **bits de permission**.

| Bit | Rôle |
| :--- | :--- |
| **Present Bit** | Cette page est-elle actuellement en RAM ? *(ou sur le disque ?)* |
| **Read/Write Bit** | Le programme a-t-il le droit de modifier cette page ? |
| **User/Supervisor Bit** | Cette page est-elle réservée au Kernel ? |

---

## 5. Le Page Fault *(L'Interruption)*

L'un des concepts les plus importants de l'informatique moderne.

**Déroulement :**

1. Le programme tente d'accéder à l'adresse `0x500`.
2. La MMU consulte la Page Table et constate que le **Present Bit = 0** — la page n'est pas physiquement en RAM.
3. Le CPU s'arrête net et lève une exception : le **Page Fault**.
4. Le Kernel intervient en urgence : il trouve une page de RAM libre, y charge les données, met le Present Bit à `1`, puis signale au CPU : *"C'est bon, ré-essaie."*

> C'est grâce à ce mécanisme qu'on peut lancer un jeu de 100 Go avec seulement 16 Go de RAM : l'OS ne charge que les pages nécessaires à l'instant T. C'est le **Demand Paging**.

---

## 6. Vocabulaire *Deep Tech* — Récapitulatif

| Terme | Définition |
| :--- | :--- |
| **Kernel Space** | Zone mémoire réservée à l'OS. Inaccessible aux programmes utilisateur. |
| **User Space** | Le bac à sable de tes programmes. |
| **Context Switch** | Passage du CPU du Programme A au Programme B. Implique de vider le TLB et charger la Page Table du nouveau programme — opération coûteuse. |
| **Fragmentation interne** | On demandes 10 octets, l'OS alloue 4096 octets (une page entière) — 4086 octets sont gâchés. |
| **Fragmentation externe** | Plein de petits trous de mémoire libre, mais aucun n'est assez grand pour un bloc contigu. |

---