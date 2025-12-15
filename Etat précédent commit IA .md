La Solution pour Depth 10 : Le "Beam Search" (Recherche en Faisceau)

C'est la méthode radicale. Actuellement, votre IA regarde tous les coups plausibles (voisins). Même avec l'élagage, cela fait trop de branches.

Le Concept : Au lieu de vérifier les 20 ou 30 coups possibles à chaque nœud, on décide arbitrairement de ne vérifier que les X meilleurs (selon votre tri actuel). Si votre fonction de tri (generate_moves) est bonne (et elle l'est : TT + Killer + History + Eval), le "meilleur coup réel" est presque toujours dans les 10 premiers.

Pourquoi "12" ?

Le facteur de branchement (b) détermine la complexité bd.

    Si b=20 (moyenne Gomoku) : 206=64,000,000.

    Si b=12 (Beam Search) : 126=2,900,000.

    Avec PVS qui coupe beaucoup de branches, b effectif descend encore plus bas.

Avec un Beam Width de 10 ou 12, vous devriez voir l'IA atteindre Depth 8 ou 10 assez facilement.

(Si l'IA joue bizarrement (rate des coups évidents), augmentez le Beam Width à 15 ou 20. Si elle est trop lente, baissez à 8 ou 10.)
