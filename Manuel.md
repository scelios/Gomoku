# Documentation Technique Approfondie du Moteur Gomoku

Ce document détaille l'architecture interne du moteur d'IA. Il ne s'agit pas d'un simple algorithme récursif, mais d'une architecture complexe inspirée des moteurs d'échecs modernes, conçue pour contourner l'explosion combinatoire inhérente au jeu de Gomoku (facteur de branchement ~200 coups possibles par tour).

---

## 0. Min Max Alpha Béta (L'Élagage) 

L'Alpha-Beta n'est pas un algorithme différent du Minimax. C'est le Minimax, mais qui arrête de calculer dès qu'il a assez d'informations.

### Les deux variables magiques

Imaginez une fenêtre de négociation `[Alpha, Beta]`.

1.  **Alpha : Le "Plancher" (Mon minimum garanti)**
    *   C'est le meilleur score que l'IA (Max) a **déjà trouvé** ailleurs dans l'arbre.
    *   *Phrase clé :* "Quoi qu'il arrive, je sais que je peux avoir au moins ce score là."
    *   Si l'IA trouve une nouvelle variante qui rapporte moins que Alpha, elle la jette (car elle a déjà mieux).

2.  **Beta : Le "Plafond" (Le maximum que l'adversaire tolère)**
    *   C'est le meilleur score pour l'Adversaire (Min) qu'il a trouvé ailleurs.
    *   *Phrase clé :* "Si je tente d'obtenir plus que ça, l'adversaire a un coup pour m'en empêcher."
    *   Si l'IA trouve une variante qui rapporte plus que Beta, elle arrête de chercher (car l'adversaire ne la laissera jamais jouer ce coup).

L'algorithme cherche toujours à prouver :
`Alpha < Score < Beta`

*   Si le score descend sous Alpha -> **C'est nul pour moi** (J'ai mieux ailleurs).
*   Si le score monte au-dessus de Beta -> **C'est trop beau** (L'adversaire ne me laissera pas faire).

Dans les deux cas, on coupe la branche de l'arbre (Pruning). On ne calcule pas les milliers de descendants de ce nœud. C'est ce qui permet d'aller profond.

Résumé Technique
    Mise à jour : On resserre la fenêtre [Alpha, Beta] à chaque bon coup trouvé.
    Condition d'arrêt (break) : Dès que Alpha croise Beta (Alpha >= Beta), la fenêtre se ferme. Cela signifie qu'il n'y a plus de solution viable dans cette branche pour les deux joueurs qui jouent parfaitement. On coupe.

---

## 1. Le Cœur Algorithmique : PVS (Principal Variation Search)

L'algorithme de base est une variante sophistiquée de l'Alpha-Beta appelée **PVS** (ou NegaScout).

### Le Concept Technique
Dans un arbre Minimax standard, on cherche la valeur exacte de chaque nœud. Le PVS part du principe que si le tri des coups est efficace, le premier coup testé (le nœud PV) sera le meilleur.
Pour tous les coups suivants, on ne cherche pas leur valeur exacte, mais on pose une question binaire : *"Ce coup est-il meilleur que le premier ?"*.

### La "Null Window" (Fenêtre Nulle) expliquée
C'est l'outil mathématique du PVS.
*   **Fenêtre standard :** `[Alpha, Beta]`. On cherche un score `v` tel que `Alpha < v < Beta`.
*   **Null Window :** `[Alpha, Alpha + 1]`. La fenêtre a une largeur de zéro (ou 1 en entiers).
    *   Si la recherche renvoie une valeur `<= Alpha`, le coup est mauvais (Fail Low). On a économisé du temps.
    *   Si la recherche renvoie une valeur `>= Alpha + 1`, le coup est "trop bon" (Fail High). C'est seulement dans ce cas rare que l'on relance une recherche coûteuse avec la fenêtre pleine pour connaître le score exact.

### Traduction dans le Code (`ai.c`)
La logique se matérialise par cette structure de contrôle spécifique dans la boucle des coups :

```c
if (i == 0) {
    // 1. Le "Champion" actuel (PV-Node)
    // On le cherche avec une fenêtre pleine pour avoir son score exact.
    val = minimax(..., alpha, beta, ...);
} else {
    // 2. Les "Challengers"
    // On les teste avec une Null Window (alpha, alpha+1).
    // L'appel est très rapide car il provoque énormément de coupures (cutoffs).
    val = minimax(..., alpha, alpha + 1, ...);

    if (val > alpha && val < beta) {
        // 3. Le Re-Search (Cas rare)
        // Le challenger a battu le champion ! La Null Window a échoué (Fail High).
        // On doit maintenant calculer son score précis.
        val = minimax(..., alpha, beta, ...);
    }
}
```

---

## 2. Ordonnancement des Coups (Move Ordering)

C'est la partie la plus critique du moteur. Pour que le PVS (ci-dessus) fonctionne, il est impératif que le meilleur coup soit testé en premier dans 90% des cas. Si les coups sont testés dans le désordre, l'IA s'effondre.

Nous utilisons un tri en 5 étapes successives :

1.  **Hash Move (Transposition Table) :**
    *   Si nous avons déjà analysé cette position (lors d'une recherche précédente ou via l'Iterative Deepening), nous savons quel était le meilleur coup. Nous le jouons immédiatement. C'est le "VIP" des coups.
2.  **Captures & Victoires :**
    *   Les coups qui gagnent la partie ou capturent des pierres adverses sont prioritaires car ils forcent la réponse de l'adversaire (coups "forcing").
3.  **Killer Moves (Heuristique du Tueur) :**
    *   *Principe :* Un "Killer Move" est un coup qui a provoqué une coupure Beta (Beta Cutoff) à la même profondeur mais dans une autre branche de l'arbre.
    *   *Logique :*  Au Gomoku, les menaces sont souvent indépendantes de la position exacte des autres pierres éloignées. Si le coup (10,10) réfute une attaque adverse dans la variation A, il y a 90% de chances qu'il réfute aussi l'attaque dans la variation B (frère).
    *   *Implémentation :*  Tableau killer_moves[MAX_DEPTH][2].
    Lorsqu'un cutoff se produit, on décale les slots : le nouveau tueur prend la place 0, l'ancien passe en place 1. Cela garde les tueurs les plus récents.
    Ces coups sont testés juste après les captures, avant l'évaluation statique coûteuse.
4.  **History Heuristic :**
    *   *Principe :* Contrairement aux Killer Moves qui sont liés à la profondeur, l'historique est global. Si le coup `(8,8)` a été bon tout au long de la partie, il reçoit un score bonus permanent. C'est la "mémoire à long terme" de la partie.
5.  **Évaluation Statique :**
    *   Contrairement aux Killer Moves (éphémères et liés à la profondeur), l'History Heuristic est persistante.

    Tableau history[BOARD_SIZE].
    À chaque fois qu'un coup provoque un cutoff ou est le meilleur coup, on incrémente sa valeur dans ce tableau (souvent + depth^2).
    Cela permet de prioriser les coups "naturellement bons" sur le long terme.

---

## 3. Optimisations de Recherche (Le "Turbo")

Pour atteindre la Depth 10 en moins de 0.5s, nous utilisons des techniques de réduction agressives.

### A. Transposition Table (Zobrist Hashing)
Le Gomoku permet de nombreuses transpositions (arriver à la même position par un ordre de coups différent).
*   Chaque position du plateau possède une signature unique de 64 bits (Hash Zobrist).
*   Avant de calculer un nœud, on vérifie dans une table de hachage (`TT_SIZE = 1 Million`) 
    si on ne connaît pas déjà la réponse.
*   L'Astuce de l'Optimisation (Mise à jour Incrémentale) :
    On ne recalcule jamais le Hash depuis zéro (ce qui serait en O(N)).
    Il n'y a aucune boucle. L'ordinateur saute directement à la case mémoire correspondante.
*   La distinction Clé vs Valeur
    - Le Zobrist Hash (La Clé) : C'est juste un grand nombre (ex: 0x8F3A2...). Il sert d'identifiant unique. Il ne contient aucune information sur qui gagne ou perd.
    - La Transposition Table (Le Casier) : C'est un tableau géant de structures. C'est là que l'information de calcul est stockée.
*   typedef struct {
        uint64_t key;   // La signature (pour vérifier qu'on a le bon ticket)
        int depth;      // A quelle profondeur on a calculé (ex: Depth 8)
        int value;      // LE RÉSULTAT DU CALCUL (ex: Score 5000)
        int best_move;  // Le meilleur coup à jouer ici
        int flag;       // Type de score (Exact, Borne inf, Borne sup)
    } TTEntry;
*   **Résumé du flux :**
    Mise à jour : L'IA joue un coup -> current_hash est mis à jour (1 opération XOR).
    Calcul d'Index : index = current_hash % TAILLE_TABLE (1 opération Math).
    Vérification : Si Table[index] est remplie... (1 accès Mémoire).
    Alors : On a déjà vu cette position ! On prend le score stocké.
    Sinon : C'est une nouvelle position, on doit la calculer.
    C'est une opération binaire ultra-rapide (1 cycle CPU).
    Cela permet de détecter les transpositions instantanément.
*   **Gain :** Évite de recalculer des milliers de sous-arbres identiques.
*   **Le processus complet :**
    Quand la phrase dit "on vérifie si on ne connaît pas déjà la réponse", voici ce qui se passe en millisecondes :

    Calcul du Hash : L'IA regarde le plateau et obtient la signature 987654.
    Accès au Casier : Elle va voir la case n° 987654 du tableau géant.
    Vérification : Elle regarde le ticket dans le casier.
    "Est-ce que le ticket stocké ici est bien 987654 ?" (Pour éviter les collisions).
    Récupération : Si oui, elle lit la variable value (le score) et best_move qui sont stockés dans le casier.

    Conclusion : Le Hash sert uniquement d'adresse GPS pour aller chercher le résultat que l'IA a calculé et sauvegardé lors d'un passage précédent (ou lors d'une recherche précédente).

### B. Iterative Deepening (Approfondissement Itératif)
L'IA ne lance pas directement une recherche à Depth 10. Elle fait Depth 1, puis 2, puis 3...
*   **Avantage 1 :** Gestion du temps. Si on dépasse les 0.5s, on s'arrête et on renvoie le meilleur coup de la profondeur précédente (Depth 8 par exemple).
*   **Avantage 2 :** Le meilleur coup trouvé à Depth `N-1` sert de premier coup à tester pour Depth `N`, optimisant ainsi le tri.

### C. Aspiration Windows
Au lieu de chercher un score entre `-Infini` et `+Infini`, on suppose que le score de la profondeur actuelle sera proche de celui de la profondeur précédente.
*   On cherche dans une fenêtre étroite : `[ScorePrécédent - 50, ScorePrécédent + 50]`.
*   Si le score sort de cette fenêtre (Fail Low ou Fail High), on relance une recherche complète.
*   **Gain :** Accélère les coupures Alpha-Beta.

### D. Late Move Reduction (LMR) - *L'optimisation clé*
C'est la technique qui nous a permis de passer de Depth 8 à Depth 10.
*   **Logique :** Si nous avons bien trié nos coups, les coups situés à la fin de la liste (après les Hash, Killer, et History moves) sont probablement mauvais.
*   **Action :** On recherche ces coups "tardifs" avec une **profondeur réduite** (`Depth - 2` voire plus).
*   **Sécurité :** Si la recherche réduite renvoie un score étonnamment bon (supérieur à Alpha), on admet qu'on s'est trompé et on relance la recherche à pleine profondeur (Re-search).
*   **Résultat :** On "survole" les mauvaises branches au lieu de les calculer à fond.

---

## 4. Analyse des Performances (Avant vs Après LMR)

L'impact de l'intégration du LMR et des Killer Moves est visible dans les logs :

### Comparatif de Profondeur
*   **Avant LMR :** Plafond à **Depth 8** (~10 000 nœuds). La Depth 10 provoquait systématiquement un Timeout (> 0.5s).
*   **Après LMR :** **Depth 10 atteinte** de manière stable (entre 25 000 et 40 000 nœuds explorés).

### Efficacité du Pruning (Élagage)
Sans LMR, une recherche à Depth 10 nécessiterait théoriquement plus de 100 000 nœuds. Le fait de n'en visiter que ~30 000 signifie que nous avons réussi à **ignorer environ 70% de l'arbre de recherche** sans perdre en qualité de jeu tactique.

### Stabilité Tactique
Les logs montrent que l'IA trouve toujours les mats (`Score 100000`) et les défenses forcées (`Score 200000`). Cela prouve que le LMR ne "réduit" pas les coups critiques, car l'heuristique de tri (Killer/History) les place correctement en début de liste, là où le LMR n'est pas appliqué.

---

## 5. Heuristique d'Évaluation (La "Vision")

L'IA ne se contente pas de compter les pierres. Elle utilise une détection de motifs avancée (`heuristics.c`).

*   **Le "Peek" (Coup d'œil) :** L'IA regarde au-delà des cases vides.
    *   Exemple : `X X . X`
    *   Une IA naïve voit "Deux pierres" (faible valeur).
    *   Notre IA voit "Un 3 brisé" (menace forte), car elle saute par-dessus le trou.
*   **Pondération Agressive :** 
    * Un "Open 3" (_ X X X _) vaut désormais 8000 points, presque autant qu'un "4 bloqué". 
    * Cela force l'IA à construire du jeu ouvert plutôt que de tenter des pièges fermés       
    * faciles à contrer.

---

## 6. Parsing du Plateu et Gestion Mémoire  en C

L'efficacité du moteur repose aussi sur la manière dont le langage C accède à la mémoire.

*   **Représentation 1D vs 2D**
*   Bien que le plateau soit visuellement une grille 2D (19x19), nous utilisons un 
*   tableau unidimensionnel int board[361].

*   **Accès Mémoire (Cache Locality) :**
*   Les processeurs modernes chargent la mémoire par "lignes de cache" (64 octets).
*   Dans un tableau 1D, les cases (x, y) et (x+1, y) sont contiguës en mémoire RAM.
*   Parcourir le tableau linéairement (for i=0 to 361) est extrêmement rapide car le CPU 
*   précharge les données suivantes avant même qu'on les demande (Prefetching).

* **Macro d'Accès :**
*   L'accès se fait via la macro GET_INDEX(x, y) ((y) * WIDTH + (x)).
*   Cela évite les doubles indirections de pointeurs (board[y][x]) qui provoquent souvent des 
*   "Cache Miss" coûteux.

*   **Heuristique de "Scan" (heuristics.c)**
*   L'évaluation ne parcourt pas tout le plateau à chaque fois.

*   Elle utilise des vecteurs de direction dx, dy pour scanner uniquement des segments de 9 
*   cases autour du dernier coup joué.
*   Le "Segment de 9" (C'est la Ligne)
*   Quand on vérifie une seule direction (par exemple, l'Horizontale), on regarde :
*    La pierre qu'on vient de poser (Centre).
*   Les 4 cases à gauche.
*   Les 4 cases à droite.
*   Total : 
*   4+1+4=9 cases.
*   Les "32 cases" (C'est le Travail Total)
*   Comme on doit vérifier les 4 directions (Horizontale, Verticale, Diagonale 1, Diagonale 
*   2), on répète l'opération 4 fois.

*   Cependant, la pierre centrale (celle qu'on vient de poser) est la même pour les 4 lignes.
*   L'algorithme va donc concrètement aller lire en mémoire les voisins :

*   Horizontale : 8 voisins (4 avant + 4 après)
*   Verticale : 8 voisins
*   Diagonale 1 : 8 voisins
*   Diagonale 2 : 8 voisins
*   Total des lectures mémoire : 
*   8×4=32 cases voisines consultées.
*   Cela transforme une complexité O(N) (tout le plateau) en O(1) (local), rendant 
*   l'évaluation quasi-instantanée.