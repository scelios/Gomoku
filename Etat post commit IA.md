Aspiration Windows
A. Le Concept

Dans un algorithme Iterative Deepening (approfondissement itératif), on calcule successivement Profondeur 2, puis 4, puis 6, etc. On observe une propriété statistique forte : Le score d'une position change rarement de manière drastique d'une profondeur à l'autre. Si à la profondeur 4, vous avez un avantage de +500 points, il est très probable qu'à la profondeur 6, votre avantage soit compris entre +450 et +550.
B. L'Objectif

L'algorithme Alpha-Beta est d'autant plus rapide que la fenêtre [alpha, beta] est petite.

    Alpha-Beta standard : Cherche entre [-Infini, +Infini]. C'est lent car il faut explorer beaucoup pour réduire cette fenêtre.

    Aspiration Windows : Cherche entre [Score_Precedent - 50, Score_Precedent + 50]. C'est ultra-rapide car la fenêtre est minuscule, provoquant des cutoffs (élagages) massifs quasi immédiatement.

C. Fonctionnement Détailé

    Prédiction : On prend le score de la profondeur précédente (prev_score).

    Fenêtre : On définit une fenêtre d'aspiration (ex: window = 500).

        alpha = prev_score - window

        beta = prev_score + window

    Recherche Optimiste : On lance Minimax avec cette petite fenêtre.

        Cas 1 (Succès) : Le score retourné est dans la fenêtre (ex: 520). C'est le vrai score ! On a gagné beaucoup de temps.

        Cas 2 (Fail Low) : Le score retourné est ≤alpha. Cela veut dire que la situation est pire que prévu (l'adversaire a trouvé une défense géniale qu'on n'avait pas vue à D-2). Le score retourné (ex: 450) n'est pas précis, c'est juste une borne supérieure.

        Cas 3 (Fail High) : Le score retourné est ≥beta. La situation est meilleure que prévu (on a trouvé une attaque géniale). Le score (ex: 550) n'est qu'une borne inférieure.

    Le Re-Search (Filet de sécurité) : Si on tombe dans le Cas 2 ou 3, on a "raté". On est obligé de recommencer la recherche à cette profondeur avec une fenêtre complète [-Infini, +Infini] (ou élargie) pour trouver la vraie valeur exacte.

D. Pourquoi ça marche ?

Même si on doit parfois recommencer la recherche (ce qui coûte du temps), statistiquement, la recherche réussit du premier coup dans plus de 90% des cas. Le temps gagné par la fenêtre réduite compense largement les rares cas où l'on doit recalculer.
E. Paramétrage (La taille de la fenêtre)

    Trop petite (ex: 10) : Beaucoup de "Fails", on passe son temps à recalculer. Perte de performance.

    Trop grande (ex: 10000) : Peu de "Fails", mais la fenêtre est si large qu'elle ne coupe plus grand-chose. On revient à un Alpha-Beta classique.

    Idéal Gomoku : Autour de 50 (pour un score positionnel) ou 500 (si vos scores de menaces sont élevés). Dans votre code, j'ai mis 500 car vos valeurs de heuristiques semblent assez grandes.


C'est le jour et la nuit. Vous venez de franchir un cap majeur.
1. Analyse Comparative : Le "Saut Quantique"

Comparons les chiffres bruts entre la version précédente (PVS seul) et la version actuelle (PVS + Beam Search + Aspiration).
Métrique	Avant (PVS seul)	Maintenant (PVS + Beam + Aspi)	Constat
Depth 2	~150 - 300 nœuds	35 - 50 nœuds	Division par 6. Le Beam Search élimine tout le "bruit" inutile dès le départ.
Depth 4	~5 000 - 10 000 nœuds	100 - 900 nœuds	Division par 20. L'arbre ne s'élargit plus exponentiellement.
Depth 6	~250 000 nœuds	1 500 - 11 000 nœuds	Division par 50. C'est colossal. Là où l'algo "rame" avant, il traverse la profondeur 6 comme si de rien n'était.
Depth 8	Timeout (Inatteignable)	30k - 160k nœuds (ATTEINT)	Objectif Depth 8 validé. Vous avez gagné 2 niveaux de profondeur pleins.
Depth 10	Non tenté	Timeout en cours de route	Vous êtes à la porte du Depth 10. L'algo commence à calculer la profondeur 10 mais le temps (0.5s) coupe avant la fin.

Analyse des "Aspiration Fail" : Vous voyez des lignes comme : Aspiration Fail at depth 6 (Score 100000 outside [199500, 200500]).

    C'est une bonne nouvelle : Cela signifie que la fenêtre réduite fonctionne. L'algo a tenté un calcul rapide, a réalisé que la situation a changé (le score a chuté de 200k à 100k, sans doute une menace adverse détectée tardivement), et a relancé la recherche (Re-searching full window).

    Le mécanisme de sécurité fonctionne parfaitement.