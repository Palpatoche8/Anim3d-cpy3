# 09 - Rooftop storm

Un justicier encapuchonné et masqué, avec une cape (personnage original), se tient au bord d'un toit, de nuit, au-dessus de la ville.
Des éclairs illuminent brutalement la scène : sa silhouette se découpe sur le ciel pendant que sa cape claque dans le vent.

Toute la géométrie est générée en code (aucun asset externe). Le projet est basé sur la bibliothèque CGP du cours
(Glad + GLFW + ImGui, OpenGL 3.3 core, C++17).

## Compilation / lancement

```bash
cd scenes_ani3d/09_rooftop_storm
mkdir build && cd build
cmake ..
make -j$(nproc)
./project
```

Ou bien `cmake --preset relwithdebinfo && cmake --build build/relwithdebinfo` (voir le README racine).
Compiler en Release / RelWithDebInfo : la simulation de la cape est nettement plus lente en Debug.

## Contrôles

| Entrée | Action |
|---|---|
| Clic gauche + glisser | tourner la caméra |
| Clic droit + glisser / molette | zoom (mode orbite) / vitesse (mode libre) |
| Ctrl + clic gauche + glisser | déplacer la cible (mode orbite) |
| W A S D (= Z Q S D en AZERTY), flèches | se déplacer (mode libre) ou déplacer la cible (orbite) |
| E / Espace, C | monter / descendre ; Shift = plus rapide |
| TAB | basculer entre caméra orbitale et caméra libre |
| L | déclencher un éclair |
| P | pause / reprise de la simulation |
| R | réinitialiser la cape |
| H | masquer / afficher l'interface |
| Shift + F | plein écran |

## Architecture

```
src/
  main.cpp, application.*   boucle GLFW / ImGui (base CGP du cours)
  environment.*             uniforms globaux : caméra, lune, flash, brouillard, temps
  scene.*                   assemblage de la scène + panneau ImGui
  engine/
    camera_rig.*            caméra orbitale + libre (souris / clavier)
    gl_buffer.*             VAO/VBO dynamique minimal (pluie, éclair)
    mesh_builder.*          primitives procédurales (boîtes, tubes elliptiques, ellipsoïdes, cônes)
  world/
    city.*                  skyline procédurale (boîtes + données de fenêtres pour le shader)
    rooftop.*               toit, parapet, gargouille, château d'eau... + boîtes de collision
    character.*             silhouette low-poly + capsules de collision + courbe d'attache de la cape
  sim/
    wind.*                  vent : direction moyenne + rafales (bruit temporel) + turbulence (bruit 3D)
    cape.*                  tissu PBD : contraintes structure / cisaillement / flexion, forces aérodynamiques, collisions
  fx/
    lightning.*             éclair (déplacement du point milieu + branches) et enveloppe du flash
    rain.*                  particules de pluie autour de la caméra, inclinées par le vent
shaders/
  lit/        Blinn-Phong lune + flash + liseré (rim) + brouillard
  building/   idem + fenêtres procédurales, rues éclairées au sodium
  sky/        dégradé, nuages fbm, halo de lune, nuages illuminés par le flash
  rain/, bolt/  traînées de pluie, ruban lumineux additif de l'éclair
```

### Cape (Position Based Dynamics)

* grille `nu x nv` de particules, rangée du haut fixée sur un arc derrière les épaules ;
* prédiction : `v += h (g + F_air / m)`, amortissement, `p = x + h v` ;
* projection Gauss-Seidel de contraintes de distance (structure, cisaillement, flexion à distance 2),
  avec raideur corrigée selon le nombre d'itérations : `k' = 1 - (1 - k)^(1/n)` ;
* collisions résolues à chaque itération : capsules du corps, boîtes du toit et de la gargouille ;
* `v = (p - x) / h` ; pas de temps fixe (1/60 s divisé en sous-pas), indépendant du framerate ;
* force du vent par triangle : pression `0.5 ρ C |v_n| v_n n A` + frottement tangentiel, avec
  `v_rel = vent(p, t) - v_triangle` : c'est ce qui fait claquer la cape ;
* normales recalculées à chaque frame, rendu double face.
