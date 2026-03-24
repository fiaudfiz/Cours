# Référence — Tailles des Types C

> Architecture **x86_64** — Linux — Norme 42
> Les tailles et alignements dépendent de l'ABI.
> L'alignement d'un type est ≤ à sa taille. Le padding est ajouté pour respecter cet alignement dans les structs.

---

## Tableau des Types par Taille

| Taille | Type(s) |
| :---: | :--- |
| **16 octets** | `long double` |
| **8 octets** | `double`, `long long`, `unsigned long long` |
| **8 octets** | `long`, `unsigned long`, `size_t`, `ssize_t` |
| **8 octets** | `void *`, `char *`, `int *`, *(tout pointeur)* |
| **4 octets** | `int`, `unsigned int`, `float` |
| **2 octets** | `short`, `unsigned short` |
| **1 octet** | `char`, `unsigned char`, `_Bool` |

---

## Par Catégorie

### Flottants

| Type | Taille |
| :--- | :---: |
| `float` | 4 octets |
| `double` | 8 octets |
| `long double` | 16 octets |

### Entiers

| Type | Taille |
| :--- | :---: |
| `char` / `unsigned char` / `_Bool` | 1 octet |
| `short` / `unsigned short` | 2 octets |
| `int` / `unsigned int` | 4 octets |
| `long` / `unsigned long` | 8 octets |
| `long long` / `unsigned long long` | 8 octets |
| `size_t` / `ssize_t` | 8 octets |

### Pointeurs

| Type | Taille |
| :--- | :---: |
| `void *`, `char *`, `int *`, *(tout pointeur)* | 8 octets |

---