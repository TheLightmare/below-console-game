# Bienvenue dans Below!

Le but pour ce TP 3 de C++ était de créer un RPG, et il se pourrait que je me soies lancé avant d'avoir pu avoir l'énoncé, il y a de celà Octobre. Certaines exigences basiques du TP ont donc été ajoutées au sparadrap par dessus un projet déjà assez défini.

# Compilation et Installation

Le jeu se compile pour linux et Windows:

COMPILATION LINUX : `$ make` -> l'exécutable est dans le root directory

COMPILATION WINDOWS : lancer "build.bat" -> l'exécutable est dans `build/Release/`

Dans les deux cas, le dossier "assets" est une dépendance pour le bon fonctionnement du projet, et l'exécutable doit être dans le même dossier pour une installation correcte.


# Dossiers et fichiers générés

En jouant, le jeu va générer un dossier "saves" et un fichier "keybindings.cfg" au besoin si jamais le joueur a sauvegardé la partie ou altéré les touches. 


# Cleanup du projet

POUR LINUX : `$ make clean`

POUR WINDOWS : supprimer manuellement le dossier "build"
