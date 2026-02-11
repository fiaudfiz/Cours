📘 COURS : Les Tubes (Pipes) sous UNIX
1. Définition

Un pipe (ou tube anonyme) est un mécanisme de communication inter-processus (IPC) unidirectionnel. Il permet de chaîner des processus de sorte que la sortie de l'un (stdout) alimente directement l'entrée du suivant (stdin).
2. Le concept de Flux (Stream)

Contrairement aux anciens systèmes (type MS-DOS) qui utilisaient des Pseudo-Pipes (stockage sur disque via des fichiers .tmp), UNIX utilise la mémoire vive (RAM).
Caractéristique	Pseudo-Pipes (Fichiers)	Tubes UNIX (Mémoire)
Vitesse	Lente (accès disque)	Ultra-rapide (RAM)
Espace	Consomme du stockage	Consomme un buffer limité
Parallélisme	Séquentiel (l'un après l'autre)	Simultané (flux en continu)
3. Implémentation technique en C
La création : pipe()

La fonction pipe(int fd[2]) demande au système de créer un tube anonyme. Elle remplit un tableau de deux descripteurs de fichiers :

    fd[0] (Read) : Le bout par lequel on lit les données.

    fd[1] (Write) : Le bout par lequel on injecte les données.

    Astuce mémo : 0 = Output (sortie du tube/lecture), 1 = Input (entrée du tube/écriture).

Le Buffering & Synchronisation

    Capacité : Sous Linux, le buffer fait généralement 64 KB.

    Blocage :

        Si le buffer est plein, le processus qui écrit (write) est mis en pause jusqu'à ce que le lecteur libère de la place.

        Si le buffer est vide, le processus qui lit (read) est mis en pause jusqu'à ce que l'écrivain injecte des données.

    Atomicité : Les écritures de moins de 4 KB sont garanties "atomiques" (non mélangées).

4. La Règle d'Or : Gestion des File Descriptors (FD)

Dans Pipex, après un fork(), les descripteurs sont dupliqués. Il est impératif de fermer les bouts inutilisés.

    Blocage infini : Un read ne se terminera jamais (ne recevra jamais EOF) tant qu'il reste un seul fd[1] (écriture) ouvert dans n'importe quel processus.

    SIGPIPE : Si un processus tente d'écrire dans un fd[1] alors que tous les fd[0] (lecteurs) sont fermés, le programme crash (Signal Pipe).

5. Redirection avec dup2()

Pour que les commandes shell (comme ls ou wc) utilisent le pipe sans le savoir, on utilise dup2(old_fd, new_fd).

    dup2(fd[1], STDOUT_FILENO) : Redirige la sortie standard vers l'entrée du tube.

    dup2(fd[0], STDIN_FILENO) : Redirige l'entrée standard vers la sortie du tube.