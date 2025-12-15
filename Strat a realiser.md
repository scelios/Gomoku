1. Transposition Table avec Zobrist Hashing (Impact : Critique)

Actuellement, votre algo recalcule des millions de fois les mêmes positions (ex: jouer A puis B revient au même que jouer B puis A).

    Le concept : On attribue une clé de hachage unique (64-bit int) à chaque position du plateau via Zobrist Hashing. On stocke le score et le "meilleur coup" trouvé pour cette position dans une table de hachage géante (Transposition Table - TT).

    Le gain : Si l'IA retombe sur une position déjà vue à une profondeur différente ou via une autre branche, elle récupère le résultat instantanément sans recalculer.

    Pourquoi c'est vital pour Depth 10 : Sans TT, Alpha-Beta ne pourra guère dépasser profondeur 6 ou 8 dans le temps imparti.

2. Optimisation de l'Évaluation (Bitboards ou Incremental Update)

Votre fonction get_point_score et update_score_impact sont vos goulots d'étranglement. Elles scannent des lignes avec des boucles while à chaque feuille de l'arbre. C'est trop lent (O(N)).

    Solution Bitboards (Avancé) : Représenter le plateau par des entiers de 64 bits (ou un tableau de 128 bits pour du 19x19). Les vérifications de lignes se font via des opérations bit à bit (AND, SHIFT) en 1 cycle CPU, sans aucune boucle.

    Solution "Table-Based" (Intermédiaire) : Au lieu de scanner, on maintient 4 tableaux (H, V, D1, D2) qui stockent l'état de chaque ligne. Quand on pose une pierre, on met à jour uniquement l'index concerné. La complexité passe de O(N) à O(1).

3. Move Ordering Avancé (Impact : Majeur)

Votre tri actuel est bon (quick_evaluate_move), mais coûteux. Pour élaguer l'arbre plus vite (avoir plus de "cutoffs" Alpha-Beta), il faut trier les coups encore mieux :

    Killer Heuristic : Si un coup X a provoqué une coupure (cutoff) à une profondeur donnée, on l'essaie en priorité dans les nœuds frères.

    History Heuristic : Si un coup (x,y) est souvent bon dans l'arbre de recherche, on lui donne un bonus permanent pour le tri, peu importe la position des autres pierres.

    Trier par la TT : Si la Transposition Table contient un "Best Move" pour cette position, on le joue toujours en premier. C'est souvent le meilleur coup à 90%.

4. Recherche de Victoire Forcée (VCF / VCT)

C'est le secret pour atteindre "Depth 10" ou "Depth 20".

    Le problème : Minimax s'épuise à calculer des coups positionnels calmes à profondeur 10.

    La solution : On lance un module séparé (VCF - Victory by Continuous Four) qui ne regarde que les coups d'attaque (4 alignés, ou 3 libres) et les réponses forcées de l'adversaire.

    Résultat : Ce module est très étroit mais très profond. Il peut trouver un mat en 15 coups en 0.01s, là où Minimax échouerait. L'IA n'appelle le Minimax lourd que si le VCF ne trouve rien.

5. Principal Variation Search (PVS) / Negascout

C'est une amélioration de l'Alpha-Beta.

    L'idée : On assume que le premier coup testé (le meilleur grâce au tri) est le bon. On cherche tous les autres coups avec une "fenêtre nulle" (alpha, alpha+1). Si un autre coup s'avère meilleur, on relance une recherche complète. Cela accélère considérablement la recherche quand le Move Ordering est bon.

Résumé de la stratégie pour votre code

Pour passer sous les 0.5s, je vous propose d'implémenter ces changements dans cet ordre précis :

    Immédiat (Low Hanging Fruit) : Optimiser get_point_score. Arrêtez de scanner toute la ligne. Scannez uniquement une fenêtre de 5 cases autour de la pierre posée. Cela divisera le temps de calcul par 2 ou 3.

    L'indispensable : Ajouter le Zobrist Hashing et la Transposition Table.

    L'accélérateur : Implémenter la Killer Heuristic dans votre générateur de coups.

Voulez-vous que je commence par vous générer le code pour le Zobrist Hashing (qui s'intègre facilement dans votre structure actuelle) ou préférez-vous l'optimisation immédiate de la fonction d'évaluation ?