### 1. Le Module VCF (Victory by Continuous Four)

C'est le seul moyen d'atteindre Depth 20+.

    Le constat : Parfois, une victoire nécessite une série de 15 attaques forcées. Votre Beam Search à Depth 10 ne la verra pas (ou la coupera).

    La solution : Avant de lancer Minimax, on lance un "Solver VCF".

        Il ne regarde que : "Je pose, ça fait 4. Il pare. Je pose, ça fait 4..."

        Il va tout droit. S'il trouve une victoire, on joue le coup immédiatement.

        Temps de calcul : ~1ms pour une profondeur 30.

    Impact : L'IA devient impitoyable sur les finitions.

Stratégie C : "Opening Book" (Livre d'Ouverture) - Priorité Facile

    Le constat : Les 3 ou 4 premiers coups du Gomoku sont théoriques. Les recalculer à chaque fois est inutile.

    La solution : Coder en dur (Hardcode) les 3 premiers coups optimaux (ex: Pro règle, Long Pro, etc.) ou utiliser un petit fichier de hashs précalculés.

    Impact : Gain de temps de 0.5s au début, et assurance de ne pas tomber dans un piège d'ouverture connu.

### 2. Killer Move Heuristic (Optimisation du Tri)
*   **Principe :** Si un coup `X` provoque une coupure (beta cutoff) à une profondeur donnée, il y a de fortes chances qu'il soit aussi un bon coup ailleurs dans l'arbre à la même profondeur.
*   **Action :** On stocke 2 "Killer Moves" par profondeur. On les teste juste après le coup de la Transposition Table.
*   **Gain :** Améliore encore le tri des coups, réduisant le nombre de nœuds à explorer.

### 3. Late Move Reduction (LMR)
*   **Principe :** Dans le PVS, si on cherche un coup qui n'est ni le premier (PV), ni un Killer Move, et qu'on est à une grande profondeur, on assume que ce coup est probablement mauvais.
*   **Action :** On le recherche avec une profondeur réduite (ex: Depth - 2). Si le score est étonnamment bon, on le recherche à pleine profondeur.
*   **Gain :** Permet de "survoler" les branches peu prometteuses et de creuser très profond dans les branches principales. C'est risqué mais payant à haute profondeur.

### 4. Bitboards (Optimisation CPU pure)
*   **Principe :** Actuellement, `evaluate_board` utilise des boucles `for` et des tableaux.
*   **Action :** Représenter le plateau par des entiers de 64 bits (ou tableaux de bits). Vérifier un alignement devient une opération binaire (`AND`, `SHIFT`) qui prend 1 cycle CPU au lieu de 50.
*   **Gain :** Vitesse d'exécution x10. C'est lourd à coder (refonte totale), mais c'est ce que font les moteurs d'échecs