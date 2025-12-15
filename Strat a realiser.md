omment franchir le dernier kilomètre (Depth 8 -> Depth 10) ?

Vous êtes actuellement bloqué à la fin de la profondeur 8 ou au début de la 10. Pour garantir le Depth 10 en 0.5s, il faut être encore plus agressif sur l'élagage.

Voici mes 3 propositions d'optimisation, de la plus simple à la plus complexe.
A. Dynamic Beam Width (Le réglage fin)

Actuellement, vous gardez probablement 12 coups partout (beam_width = 12). Mais à la profondeur 8, a-t-on vraiment besoin de regarder les 12 meilleures réponses ? Souvent, seules les 2 ou 3 meilleures comptent.

Proposition : Réduire la largeur du faisceau à mesure qu'on descend en profondeur.

    Depth 1-2 : Largeur 12 (On reste ouvert aux possibilités stratégiques).

    Depth 3-6 : Largeur 8.

    Depth 7+ : Largeur 4 ou 5 (On se concentre sur la tactique pure).

Dans generate_moves :
C

int beam_width = 12;
if (depth > 4) beam_width = 8;
if (depth > 6) beam_width = 5;

if (count > beam_width) count = beam_width;

Gain espéré : Cela divise par 2 ou 3 le nombre de nœuds finaux, ce qui devrait suffire pour finir la Depth 10.
B. Late Move Reduction (LMR)

C'est une technique standard des moteurs d'échecs. L'idée : Si un coup est trié tardivement (ex: c'est le 5ème ou 6ème coup généré) et qu'il n'est pas une capture ou une menace directe (Check), il est probablement mauvais. L'action : On le recherche avec une profondeur réduite (ex: depth - 2 au lieu de depth - 1).

    Si le coup s'avère mauvais (99% des cas) : On a gagné énormément de temps.

    Si le coup s'avère bon malgré la réduction : On le re-vérifie à pleine profondeur.

C. Le Module VCF (Le "Vrai" Depth 20)

Je reviens à la charge avec cette solution car c'est la "Secret Sauce" du Gomoku. Même si vous atteignez Depth 10 avec le Beam Search, l'IA risque de rater une victoire forcée en 15 coups (Depth 15) qui nécessite une séquence précise. Le VCF est un module qui ne regarde que les coups d'attaque (4, ou 3-libre). Comme le facteur de branchement est de 1 (l'adversaire doit répondre), il explore jusqu'à Depth 20 ou 30 quasi instantanément.
Ma recommandation immédiate

Pour valider votre objectif "Depth 10 en 0.5s" rapidement sans réécrire l'architecture :

Implémentez le "Dynamic Beam Width" (Proposition A).

Modifiez simplement generate_moves dans ai.c pour réduire le beam_width quand depth est élevé. Essayez :
C

int beam_width = 10; // Base un peu plus stricte
if (depth >= 4) beam_width = 6; // Très sélectif en profondeur

Avec ce réglage, vous devriez voir apparaître Depth 10 complete dans vos logs lors du prochain test.