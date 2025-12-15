1. Bitboards / Optimisation de l'évaluation : Est-ce encore utile ?

Réponse : NON (pour l'instant).

    Pourquoi ? Tu viens de faire l'optimisation "Table-Based" sans le savoir ! En passant de la lecture de toute la ligne (O(N)) à la lecture locale de rayon 4 (O(1)), tu as déjà gagné 90% de la performance possible sur l'évaluation.

    Le gain des Bitboards : Les Bitboards permettraient de traiter 64 cases d'un coup. Sur un Gomoku 19x19 ou 15x15, c'est complexe à implémenter (il faut plusieurs entiers de 64 bits). Le gain de performance serait maintenant marginal (peut-être x1.5 ou x2) comparé à l'effort de réécrire tout le moteur.

    Verdict : Garde ta fonction get_point_score actuelle. Elle est suffisante pour Depth 10 si l'élagage est bon.

2. Le Beam Search : Est-ce encore utile ?

Réponse : OUI, ABSOLUMENT.

C'est probablement L'ARME ULTIME pour atteindre artificiellement la "Profondeur 10" si l'Alpha-Beta standard (même avec PVS) plafonne à 6 ou 8.

    Le Concept : Au lieu de regarder tous les coups possibles (ou de compter sur l'Alpha-Beta pour les couper), on décide arbitrairement de ne garder que les X meilleurs coups à chaque nœud.

    La différence avec Alpha-Beta : Alpha-Beta peut tout regarder s'il n'a pas de chance. Beam Search garantit qu'il ne regardera jamais plus de X coups.

    Le Risque : Si le coup gagnant est le 6ème meilleur coup apparent et que ton Beam (largeur de faisceau) est de 5, tu perds la partie (l'IA ne le verra jamais). C'est une recherche "risquée" mais ultra-rapide.

Comparatif Stratégique pour ton Objectif

Pour faire Depth 10 en 0.5s, voici la hiérarchie des gains :

    PVS (Principal Variation Search) :

        Type : Algorithmique (Smart).

        Gain : Réduit le nombre de nœuds en vérifiant vite fait que les mauvais coups sont mauvais.

        Sécurité : 100% sûr (ne rate jamais rien).

        Recommandation : À faire en priorité.

    Beam Search :

        Type : Heuristique (Brutal).

        Gain : Force mathématiquement la profondeur. Si tu limites à 5 coups par nœud, tu iras très profond.

        Sécurité : Risqué (peut rater des tactiques subtiles).

        Recommandation : La roue de secours. Si avec PVS tu bloques encore à Depth 8, active le Beam Search pour forcer le Depth 10.

    VCF (Module spécialisé) :

        Type : Expert.

        Gain : Trouve des mats en 20 coups, mais ne compte pas vraiment comme de la "profondeur Minimax" classique.

        Recommandation : Pour gagner des tournois, pas forcément pour l'objectif "algo depth 10".