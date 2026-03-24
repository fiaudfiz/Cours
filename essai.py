#!/usr/bin/env python3
import random
import time
import os
import sys

def matrix_rain():
    # Codes ANSI
    GREEN = '\033[92m'
    BRIGHT_GREEN = '\033[32m'
    RESET = '\033[0m'
    HIDE_CURSOR = '\033[?25l'
    SHOW_CURSOR = '\033[?25h'
    CLEAR = '\033[2J\033[H'

    try:
        print(HIDE_CURSOR, end='', flush=True)
        print(CLEAR, end='', flush=True)

        # Taille du terminal
        cols = os.get_terminal_size().columns
        rows = os.get_terminal_size().lines

        # Position verticale de chaque colonne
        drops = [random.randint(-rows, 0) for _ in range(cols)]
        # Grille de caractères affichés
        grid = [[' '] * cols for _ in range(rows)]

        while True:
            cols = os.get_terminal_size().columns
            rows = os.get_terminal_size().lines

            # Ajuster drops si le terminal a changé de taille
            while len(drops) < cols:
                drops.append(random.randint(-rows, 0))
            drops = drops[:cols]

            output = []
            for col in range(cols):
                row = drops[col]
                if 0 <= row < rows:
                    # Tête de la goutte : vert vif
                    char = str(random.randint(0, 9))
                    grid[row][col] = char
                    output.append(f'\033[{row+1};{col+1}H{BRIGHT_GREEN}{char}{RESET}')

                    # Traîne : vert sombre
                    trail_len = random.randint(5, 20)
                    for t in range(1, trail_len):
                        tr = row - t
                        if 0 <= tr < rows:
                            trail_char = str(random.randint(0, 9))
                            grid[tr][col] = trail_char
                            intensity = max(0, 255 - t * 20)
                            output.append(f'\033[{tr+1};{col+1}H{GREEN}{trail_char}{RESET}')

                    # Effacer la fin de traîne
                    erase_row = row - trail_len
                    if 0 <= erase_row < rows:
                        grid[erase_row][col] = ' '
                        output.append(f'\033[{erase_row+1};{col+1}H ')

                # Avancer la goutte
                drops[col] += 1
                if drops[col] > rows + random.randint(5, 20):
                    drops[col] = random.randint(-rows, -1)

            print(''.join(output), end='', flush=True)
            time.sleep(0.05)

    except KeyboardInterrupt:
        print(SHOW_CURSOR, end='')
        print(CLEAR, end='')
        print(f"{GREEN}Arrêt du programme.{RESET}")
        sys.exit(0)

if __name__ == '__main__':
    matrix_rain()
