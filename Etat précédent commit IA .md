Composant	Choix Technique	Conséquence (Avantage / Inconvénient)
Structure	Tableau 1D (int board[361])	

(+) Très rapide pour le cache CPU (L1/L2). Accès immédiat.

(-) Pas aussi rapide que des Bitboards (opérations bit à bit), mais plus simple à coder.
Algorithme	Minimax + Alpha-Beta	

(+) Coupe les mauvaises branches. Indispensable.

(-) Son efficacité dépend à 100% de l'ordre des coups (Move Ordering).
Gestion du Temps	Iterative Deepening	

(+) L'IA joue toujours dans les temps (0.5s). Elle fait prof 2, puis 4, puis 6... et s'arrête net.

(-) On perd un peu de temps à recalculer les profondeurs 2 et 4 avant d'attaquer la 6.
Génération de Coups	Voisinage (Radius 2)	

(+) Réduit l'arbre de 361 possibilités à ~20 coups pertinents. C'est l'optimisation majeure actuelle.

(-) Peut manquer des menaces très lointaines (rare).
Heuristique	Scan Global (O(N))	

(+) Précis, facile à comprendre.

(-) Trop lent. À chaque feuille de l'arbre, on relit tout le plateau (361 cases x 4 directions). C'est votre principal goulot d'étranglement.
Captures	Simulation Dynamique	

(+) L'IA "voit" les captures futures.

(-) Coûteux : on modifie le plateau et on lance checkPieceCapture à chaque nœud simulé.