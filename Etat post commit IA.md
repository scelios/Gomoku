## Mise à jour Heuristique (Stratégie Heuristique Positionnelle - Implémentée)

Nous avons affiné l'évaluation pour détecter les menaces cachées (trous) et rendre l'IA plus agressive sur les "Open 3".

**Nouvelle Pondération :**
*   **Open 3 (`_ X X X _`)** : Score passé de 5 000 à **8 000**. (Presque équivalent à un 4 bloqué).
*   **Broken 3 (`X X . X`)** : Score passé de ~150 à **800**. (L'IA ne l'ignore plus).
*   **Broken 4 (`X X X . X`)** : Score passé de ~1000 à **6 000**. (Considéré comme une menace mortelle immédiate).

## Comparatif de Performance (Avant vs Après Heuristique)

L'ajout de la logique de "Peek" (regarder après le trou) a un coût CPU léger, mais visible sur la profondeur maximale atteinte en 0.5s.

| Profondeur | Nœuds (Avant) | Nœuds (Après - Moyenne) | Observation |
| :--- | :--- | :--- | :--- |
| **Depth 4** | 281 | ~270 | Identique. Le tri des coups reste excellent. |
| **Depth 6** | 2 140 | ~1 700 | **Amélioration**. L'IA coupe plus vite car elle identifie mieux les menaces. |
| **Depth 8** | 14 674 | ~10 000 | **Amélioration**. Moins de nœuds explorés pour atteindre la même profondeur. |
| **Depth 10** | 75 223 (Fini) | Timeout (Cut) | **Régression apparente**. Le calcul par nœud étant plus lourd, on dépasse les 0.5s. |

**Conclusion :**
L'IA explore **moins de nœuds** (le pruning est plus efficace grâce aux scores plus tranchés), mais chaque nœud prend un peu plus de temps à calculer.
*   **Résultat :** On s'arrête à Depth 8 solide au lieu de Depth 10 limite.
*   **Qualité :** La qualité du jeu à Depth 8 "New Gen" est supérieure à Depth 10 "Old Gen" car elle ne rate plus les coups vicieux à trous.