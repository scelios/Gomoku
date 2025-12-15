Documentation Technique : Principal Variation Search (PVS)

Voici la documentation que vous m'avez demandée pour comprendre ce qui se passe sous le capot.
A. Objectif Fondamental

L'objectif du PVS est de prouver qu'un coup est mauvais le plus vite possible. Dans un arbre de jeu bien trié, le premier coup examiné est le meilleur (le coup "Principal"). Tous les autres coups sont des pertes de temps. Le PVS cherche à dépenser un minimum de CPU sur ces "autres coups" en faisant une vérification sommaire plutôt qu'un calcul complet.
B. La Différence Drastique (Alpha-Beta vs PVS)
Caractéristique	Alpha-Beta Classique	PVS (NegaScout)
Philosophie	"Je calcule tout précisément jusqu'à preuve du contraire."	"Je fais confiance au 1er coup, je survole les autres."
Fenêtre de recherche	Toujours [alpha, beta] (Large).	

1er coup : [alpha, beta]

Autres : [alpha, alpha+1] (Nulle).
Coût des nœuds frères	Cher. Chaque nœud peut explorer beaucoup de sous-nœuds.	Très bon marché. La fenêtre nulle force des cutoffs immédiats.
Risque	Aucun (mais lent).	Devoir recalculer (Re-search) si le tri était mauvais.
C. Les Gains Concrets

    Réduction des Nœuds : Sur un arbre parfaitement trié, PVS peut réduire le nombre de nœuds visités de 30% à 50% par rapport à un Alpha-Beta standard.

    Profondeur : Cette économie permet souvent de gagner +1 à +2 de profondeur supplémentaire dans le même temps imparti.

    Synergie : PVS ne fonctionne que si vous avez une bonne Transposition Table et un bon Move Ordering (ce que vous avez fait juste avant). Sans cela, il passerait son temps à faire des "Re-search" (recalculs), ce qui le rendrait plus lent que l'Alpha-Beta.

D. Détails de l'Implémentation (Comment ça marche ?)

La logique repose sur le concept de Null Window Search (Recherche à fenêtre nulle).

    L'Hypothèse : On assume que le premier coup moves[0] est le meilleur. On récupère son score exact (disons 100). Notre fenêtre devient [100, beta].

    Le Test Rapide : Pour le coup suivant moves[1], on ne veut pas savoir s'il vaut 80 ou 90 (ce qui est inférieur à 100). On veut juste savoir : "Est-il capable de battre 100 ?".

    La Fenêtre Nulle : On appelle minimax avec alpha=100 et beta=101.

        Comme alpha et beta sont collés, il est impossible pour l'algorithme de trouver une valeur "entre les deux".

        Il doit répondre par OUI (Fail High, score >= 101) ou NON (Fail Low, score <= 100).

    Le Verdict :

        Si NON (90% des cas) : Le coup est mauvais. On l'écarte instantanément. On a économisé le calcul de ses sous-branches complexes.

        Si OUI (Oups, le coup est génial) : Notre tri s'est trompé. On doit relancer une recherche normale [100, beta] pour connaître la vraie valeur de ce coup surprise.