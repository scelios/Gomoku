🚀 Plan d'Amélioration (Du plus simple au changement de paradigme)

Pour passer de Depth 6 à Depth 10+ (ou équivalent en intelligence), voici les étapes.
Étape 1 : Optimisation Critique - Le "Move Ordering" (Gain x10)

C'est l'amélioration la plus rentable. Au lieu de trier par distance au centre, on trie par "potentiel immédiat".

Concept : Avant de lancer le Minimax récursif, on donne un score rapide à chaque coup candidat :

    Si je joue ici, est-ce que ça fait 5 alignés ? (Score max)

    Est-ce que ça bloque un 4 adverse ? (Score très haut)

    Est-ce que ça crée un 3 libre ? (Score moyen)

Si le premier coup testé est excellent, l'Alpha-Beta peut ignorer 90% des autres coups.

# Added !

Étape 2 : Réduire la largeur (Beam Search)

Au lieu de tester tous les coups voisins (disons 40 coups), on ne garde que les 10 meilleurs selon l'heuristique rapide de l'étape 1.

    Risque : Rater un coup de génie très subtil.

    Avantage : On passe de largeur 40 à largeur 10.

    106 (1 million) vs 406 (4 milliards). Tu atteindras Depth 10-12 facile.

Étape 3 : La Table de Transposition (Mémoire)

Dans le Gomoku, on retombe souvent sur les mêmes configurations (A puis B = B puis A).

    On utilise le Zobrist Hashing.

    On stocke le score des positions déjà vues dans une Hash Map.

    Si on recroise la position, on renvoie le score stocké instantanément.


🧠 Changement de Paradigme ? (VCF - Victory by Continuous Four)

Si après ça l'IA est toujours "lente" à trouver les victoires forcées, il existe une technique spécifique au Gomoku : le VCF (Victory by Continuous Four).

L'IA classique cherche "quel est le meilleur coup global". Le VCF cherche : "Est-ce que j'ai une suite de coups forcés qui mène au mat ?"

    Je fais un 4 (l'ennemi doit bloquer).

    Je fais un autre 4 (il doit bloquer).

    Je fais un 4 (il doit bloquer).

    Je fais un 5 (Gagné).

C'est un arbre de recherche très fin et très profond (peut aller à Depth 20+ facile) car il n'y a quasiment pas de branches (coups forcés).

---

1. Pourquoi la "Symétrie Locale" est un piège ?

Imaginons que tu poses une pierre X et que tu regardes un rayon de 2 cases autour.

Scénario A :
Plaintext

. . . . .
. O . O .
. . X . .  <-- Ton coup
. O . O .
. . . . .

Ici, c'est parfaitement symétrique. Si tu calcules le score en haut à gauche, tu pourrais théoriquement déduire le score en bas à droite.

Scénario B (La réalité du jeu) :
Plaintext

. . . . .
. A . B .  <-- A est une pierre ennemie, B est vide
. . X . .
. . C . .  <-- C est un bord de map
. . . . .

Pour savoir si la situation est symétrique, l'algorithme doit d'abord lire la case A, puis lire la case opposée (en bas à droite), et les comparer.

    Si A == Opposé, alors je gagne du temps ? Non, car j'ai déjà dû lire les deux cases pour vérifier l'égalité !

    L'opération de "vérifier si c'est symétrique" prend autant de cycles CPU que de "juste calculer le score".

Conclusion : Le coût de vérification de la symétrie annule le gain potentiel.
2. L'optimisation naturelle (On le fait déjà !)

En réalité, l'algorithme "divise déjà par 2" naturellement, non pas par symétrie de position, mais par axe.

Quand l'IA évalue une pierre X, elle ne lance pas 8 rayons (Haut, Bas, Gauche, Droite, 4 diagonales). Elle lance 4 axes :

    Horizontal (Gauche <-> Droite en un seul passage)

    Vertical (Haut <-> Bas en un seul passage)

    Diagonale 1

    Diagonale 2

Elle traite Gauche et Droite ensemble comme une seule ligne. Donc, elle ne fait pas le travail en double.
3. La VRAIE application de ton idée : Le "Pattern Matching" instantané

Ton intuition est : "Au lieu de scanner case par case, ne peut-on pas reconnaître le motif immédiatement ?"

C'est possible et c'est une optimisation redoutable appelée Table de Pré-calcul (Lookup Table).
Le principe

Au lieu de faire une boucle for qui regarde case par case (if case[i] == ...), on transforme la ligne locale en un nombre unique (un code) et on regarde le résultat dans un tableau géant pré-rempli.

Exemple concret sur un rayon de 4 cases (9 cases au total) : Imagine une ligne : _ _ O X X _ _ _ _ On peut encoder cela en binaire (00=vide, 01=IA, 10=Ennemi) : 00 00 10 01 01 00 00 00 00

Cela donne un nombre entier (un index). L'IA fait alors simplement :
C

score = SCORE_TABLE[ index ];

C'est instantané (accès mémoire direct O(1)). Pas de boucles, pas de if, pas de scan.
Où intervient la symétrie ici ?

C'est DANS la construction de ce tableau SCORE_TABLE que tu utilises la symétrie pour réduire la taille du tableau en mémoire (mais pas le temps de calcul).

    L'index pour O X X donnera le même score que l'index pour X X O.

    Mais pendant le jeu, l'IA se contente de lire la valeur.