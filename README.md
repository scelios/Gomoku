<!-- Gomoku will be a project about a min max algorithm on a gomoku game.
Please check the roadmap

# create a dedicated Xauthority for docker and merge current cookie
xauth nlist $DISPLAY | sed -e 's/^..../ffff/' | xauth -f /tmp/.docker.xauth nmerge - -->

Gomoku AI - 42 Project

Ce projet implémente un bot capable de jouer au Gomoku (règles incluant la capture de paires et l'interdiction du double-trois) sur un plateau 19x19. L'IA est conçue en C pour une performance maximale, utilisant des algorithmes de recherche avancés pour atteindre une profondeur de jeu compétitive en moins de 0.5 seconde par coup.
🧠 Architecture Algorithmique

L'intelligence artificielle repose sur un algorithme Minimax fortement optimisé. Voici les concepts clés utilisés pour réduire la complexité exponentielle du jeu (19×19 cases).
1. Algorithmes de Recherche

    Minimax : L'algorithme de base. Il simule l'arbre des coups possibles : l'IA (Max) tente de maximiser son score, tandis qu'elle suppose que l'adversaire (Min) jouera parfaitement pour le minimiser.

    Alpha-Beta Pruning (Élagage Alpha-Bêta) : Optimisation critique du Minimax. Elle permet de "couper" (ne pas explorer) des branches entières de l'arbre dès qu'il est prouvé qu'un coup est moins bon qu'une option déjà trouvée précédemment. Cela permet d'aller beaucoup plus profond sans perte de précision.

    Iterative Deepening (Approfondissement Itératif) : Au lieu de lancer une recherche directe à profondeur 10 (qui pourrait timeout), l'IA cherche d'abord à profondeur 2, puis 4, puis 6, etc. Si le temps limite (0.45s) est atteint, elle s'arrête et renvoie le meilleur coup de la dernière itération complétée.

    Aspiration Windows : Lors de l'approfondissement itératif, l'IA suppose que le score ne variera pas énormément d'une profondeur à l'autre. Elle cherche donc dans une "fenêtre" de score restreinte (ex: [score_precedent - 400, score_precedent + 400]). Si le score sort de cette fenêtre, une recherche complète est relancée.

2. Optimisation des Branches (Pruning & Reduction)

    Move Ordering (Ordonnancement des Coups) : Pour que l'Alpha-Bêta soit efficace, il faut tester les meilleurs coups en premier. Nous utilisons une heuristique légère (rateMoveLight) pour trier les coups : les captures et les menaces d'alignement sont vérifiées avant les coups passifs.

    Dynamic Branching (Élagage Dynamique) : L'IA réduit le nombre de coups explorés à mesure qu'elle descend en profondeur.

        Racine (Depth 10) : On regarde 40 coups.

        Feuilles (Depth 1) : On ne regarde que les 5 meilleurs coups. Cela permet de concentrer la puissance de calcul sur les variantes principales.

    LMR (Late Move Reduction) : Si un coup semble mauvais (tard dans la liste triée) et ne présente pas de menace tactique immédiate, l'IA le recherche avec une profondeur réduite (ex: Depth - 2). S'il s'avère finalement bon, il est revérifié à pleine profondeur.

3. Mémoire & Cache

    Zobrist Hashing : Technique de hachage permettant de générer une signature unique (clé uint64_t) pour chaque configuration du plateau en temps constant, grâce à des opérations XOR incrémentales.

    Transposition Table (TT) : Une table de hachage qui stocke les positions déjà analysées (via la clé Zobrist). Si l'IA retombe sur une position connue (par interversion de coups), elle récupère instantanément le score stocké au lieu de recalculer tout l'arbre.

4. Heuristique d'Évaluation (evaluateBoard)

Fonction déterminant si une position est favorable ou non.

    Pattern Recognition : Détection des alignements (5 de suite, 4 ouverts, 3 ouverts, etc.) avec des poids exponentiels.

    Capture Priority : Une emphase massive est mise sur la capture de paires et la protection contre la capture. Un score de capture vaut plus qu'une construction simple.

    Bounding Box : L'évaluation ne scanne pas les 361 cases, mais uniquement le rectangle contenant les pierres actives (+ une marge), réduisant drastiquement le temps de calcul en début et milieu de partie.

5. Optimisations Techniques (C)

    Zero Malloc in Recursion : Aucune allocation dynamique (malloc/free) n'est effectuée dans la boucle de recherche. Toutes les structures sont allouées sur la pile (Stack) pour éviter les appels système coûteux.

    Sandbox Pattern : L'IA travaille sur une copie locale du jeu (sandbox_game) pour simuler ses coups. Cela garantit l'intégrité des données du jeu réel et évite les bugs graphiques ("ghost stones").

    Bitwise & Compiler Flags : Compilation avec -O3, -march=native et -flto pour vectoriser les boucles et utiliser les instructions processeur spécifiques.

🛠️ Compilation & Utilisation
    # Compiler le projet
    make

    # Lancer le jeu
    ./gomoku

🎮 Contrôles

    Clic Gauche : Placer une pierre.

    Échap : Quitter.