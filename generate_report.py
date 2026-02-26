#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Generates the TP3 Hexagone 2026 report PDF for the Console RPG project.
Author: Mathias Kowalski
"""

from fpdf import FPDF
import os

class RPGReport(FPDF):
    """Custom PDF class with header/footer."""

    def header(self):
        self.set_font("Helvetica", "I", 8)
        self.set_text_color(120, 120, 120)
        self.cell(0, 5, "TP3 - Hexagone 2026 | Mathias Kowalski", align="L")
        self.cell(0, 5, f"Page {self.page_no()}/{{nb}}", align="R", new_x="LMARGIN", new_y="NEXT")
        self.line(10, 13, 200, 13)
        self.ln(4)

    def footer(self):
        self.set_y(-15)
        self.set_font("Helvetica", "I", 8)
        self.set_text_color(120, 120, 120)
        self.line(10, self.get_y() - 2, 200, self.get_y() - 2)
        self.cell(0, 10, "Below - Console RPG | C++17 / ECS Architecture", align="C")

    def chapter_title(self, title, level=1):
        if level == 1:
            self.set_font("Helvetica", "B", 16)
            self.set_text_color(25, 55, 120)
            self.ln(4)
            self.cell(0, 10, title, new_x="LMARGIN", new_y="NEXT")
            self.set_draw_color(25, 55, 120)
            self.line(10, self.get_y(), 200, self.get_y())
            self.ln(4)
        elif level == 2:
            self.set_font("Helvetica", "B", 13)
            self.set_text_color(40, 80, 150)
            self.ln(3)
            self.cell(0, 8, title, new_x="LMARGIN", new_y="NEXT")
            self.ln(2)
        elif level == 3:
            self.set_font("Helvetica", "B", 11)
            self.set_text_color(60, 100, 170)
            self.ln(2)
            self.cell(0, 7, title, new_x="LMARGIN", new_y="NEXT")
            self.ln(1)

    def body_text(self, text):
        self.set_font("Helvetica", "", 10)
        self.set_text_color(30, 30, 30)
        self.multi_cell(0, 5.5, text)
        self.ln(1)

    def bullet_point(self, text, indent=10):
        self.set_font("Helvetica", "", 10)
        self.set_text_color(30, 30, 30)
        self.set_x(self.l_margin + indent)
        self.multi_cell(self.w - self.r_margin - self.l_margin - indent, 5.5, "- " + text)

    def code_block(self, code, lang="C++"):
        self.set_font("Courier", "", 8)
        self.set_fill_color(240, 240, 245)
        self.set_text_color(40, 40, 40)
        self.set_draw_color(200, 200, 210)
        x = self.get_x() + 5
        y = self.get_y()
        lines = code.split("\n")
        h = len(lines) * 4.2 + 4
        # Check if we need a page break
        if y + h > 270:
            self.add_page()
            y = self.get_y()
        self.rect(x, y, 180, h, style="DF")
        self.set_xy(x + 3, y + 2)
        for line in lines:
            self.cell(0, 4.2, line, new_x="LMARGIN", new_y="NEXT")
            self.set_x(x + 3)
        self.ln(3)

    def bold_text(self, label, text):
        self.set_font("Helvetica", "B", 10)
        self.set_text_color(30, 30, 30)
        self.cell(self.get_string_width(label) + 1, 5.5, label)
        self.set_font("Helvetica", "", 10)
        self.multi_cell(0, 5.5, text)


def generate_report():
    pdf = RPGReport()
    pdf.alias_nb_pages()
    pdf.set_auto_page_break(auto=True, margin=20)

    # ===================== TITLE PAGE =====================
    pdf.add_page()
    pdf.ln(30)
    pdf.set_font("Helvetica", "B", 28)
    pdf.set_text_color(25, 55, 120)
    pdf.cell(0, 15, "TP3 : Hexagone 2026", align="C", new_x="LMARGIN", new_y="NEXT")
    pdf.ln(5)
    pdf.set_font("Helvetica", "", 18)
    pdf.set_text_color(60, 60, 60)
    pdf.cell(0, 10, 'Modelisation, Heritage, Surcharge', align="C", new_x="LMARGIN", new_y="NEXT")
    pdf.ln(3)
    pdf.set_font("Helvetica", "I", 16)
    pdf.set_text_color(80, 80, 80)
    pdf.cell(0, 10, '"Below" - Un RPG en console', align="C", new_x="LMARGIN", new_y="NEXT")
    pdf.ln(20)

    pdf.set_draw_color(25, 55, 120)
    pdf.line(50, pdf.get_y(), 160, pdf.get_y())
    pdf.ln(15)

    pdf.set_font("Helvetica", "", 13)
    pdf.set_text_color(40, 40, 40)
    pdf.cell(0, 8, "Auteur : Mathias Kowalski", align="C", new_x="LMARGIN", new_y="NEXT")
    pdf.ln(3)
    pdf.set_font("Helvetica", "", 12)
    pdf.set_text_color(80, 80, 80)
    pdf.cell(0, 8, "Langage : C++17", align="C", new_x="LMARGIN", new_y="NEXT")
    pdf.cell(0, 8, "Architecture : Entity-Component-System (ECS)", align="C", new_x="LMARGIN", new_y="NEXT")
    pdf.cell(0, 8, "Build : CMake 3.10+ (Windows & Linux)", align="C", new_x="LMARGIN", new_y="NEXT")
    pdf.ln(20)
    pdf.set_font("Helvetica", "I", 10)
    pdf.set_text_color(120, 120, 120)
    pdf.cell(0, 8, "Fevrier 2026", align="C", new_x="LMARGIN", new_y="NEXT")

    # ===================== TABLE OF CONTENTS =====================
    pdf.add_page()
    pdf.chapter_title("Table des matieres")
    pdf.set_font("Helvetica", "", 11)
    pdf.set_text_color(30, 30, 30)

    toc = [
        ("1.", "Introduction", 3),
        ("2.", "Technologies employees", 4),
        ("3.", "Architecture du projet", 5),
        ("  3.1", "Structure des dossiers", 5),
        ("  3.2", "Le systeme ECS (Entity-Component-System)", 6),
        ("  3.3", "Separation Moteur / Jeu", 7),
        ("4.", "Modelisation UML", 8),
        ("  4.1", "Diagramme de classes (composants)", 8),
        ("  4.2", "Heritage et polymorphisme", 9),
        ("5.", "Les personnages", 10),
        ("  5.1", "Le joueur", 10),
        ("  5.2", "Les ennemis", 10),
        ("  5.3", "Les allies et PNJ", 11),
        ("6.", "Les objets et l'inventaire", 12),
        ("  6.1", "Types d'objets", 12),
        ("  6.2", "Potions et effets", 12),
        ("  6.3", "Armes et armures", 13),
        ("  6.4", "Equipement et anatomie", 13),
        ("7.", "Le systeme de combat", 14),
        ("8.", "Rencontres et interactions", 15),
        ("9.", "Surcharge d'operateurs", 16),
        ("10.", "Systeme de sauvegarde et chargement", 17),
        ("11.", "Generation procedurale du monde", 18),
        ("12.", "Intelligence artificielle (Utility AI)", 19),
        ("13.", "Conception data-driven (JSON)", 20),
        ("14.", "Compilation et execution", 21),
        ("15.", "Conclusion", 22),
    ]
    for num, title, page in toc:
        pdf.set_font("Helvetica", "B" if not num.startswith(" ") else "", 11)
        dots = "." * (70 - len(num) - len(title))
        pdf.cell(0, 6.5, f"{num}  {title} {dots} {page}", new_x="LMARGIN", new_y="NEXT")
    pdf.ln(5)

    # ===================== 1. INTRODUCTION =====================
    pdf.add_page()
    pdf.chapter_title("1. Introduction")
    pdf.body_text(
        "Ce rapport presente le projet \"Below\", un jeu de role (RPG) developpe en C++17 dans le cadre "
        "du TP3 Hexagone 2026. Le sujet demande de concevoir et implementer un RPG comportant des personnages "
        "(joueur, ennemis, allies) avec des points de vie, de l'attaque, de la defense et un inventaire, "
        "ainsi qu'un systeme de combat tour par tour, des objets (potions, armes, armures), un systeme de "
        "sauvegarde/chargement et au moins une surcharge d'operateur utile."
    )
    pdf.body_text(
        "Notre implementation va bien au-dela du cahier des charges minimal : le projet utilise une "
        "architecture Entity-Component-System (ECS) complete, un monde infini genere proceduralement par "
        "bruit de Perlin, un systeme d'intelligence artificielle par utilite (Utility AI), un systeme "
        "d'anatomie avec membres equipables, des factions avec relations dynamiques, et un pipeline "
        "entierement data-driven base sur des fichiers JSON."
    )
    pdf.body_text(
        "Le choix d'utiliser l'ECS plutot qu'un heritage classique Personnage -> Joueur/Ennemi/Allie "
        "est motive par la flexibilite et l'extensibilite : dans un ECS, un personnage n'est qu'un "
        "identifiant numerique auquel on attache des composants (Stats, Inventaire, Faction, IA, etc.). "
        "Cela permet d'ajouter de nouveaux types d'entites sans modifier l'architecture existante."
    )

    # ===================== 2. TECHNOLOGIES =====================
    pdf.add_page()
    pdf.chapter_title("2. Technologies employees")

    pdf.chapter_title("Langage et standard", level=2)
    pdf.body_text(
        "Le projet est entierement developpe en C++17 (ISO/IEC 14882:2017). Ce standard apporte des "
        "fonctionnalites cles utilisees dans le projet :"
    )
    pdf.bullet_point("std::optional, structured bindings (auto& [key, value])")
    pdf.bullet_point("std::filesystem pour la gestion des sauvegardes")
    pdf.bullet_point("if constexpr pour le code template conditionnel")
    pdf.bullet_point("Fold expressions et deduction de template amelioree")
    pdf.ln(2)

    pdf.chapter_title("Bibliotheques et dependances", level=2)
    pdf.body_text(
        "Le projet n'utilise aucune bibliotheque externe. Tout est code en C++17 pur avec la "
        "bibliotheque standard uniquement :"
    )
    pdf.bullet_point("Parseur JSON custom (engine/util/json.hpp) - zero dependance externe")
    pdf.bullet_point("Console rendering avec API systeme (Windows: Win32 Console API, Linux: ANSI escape codes)")
    pdf.bullet_point("Bruit de Perlin implemente nativement (engine/world/perlin_noise.hpp)")
    pdf.bullet_point("Aucune bibliotheque graphique (SDL, SFML, ncurses, etc.)")
    pdf.ln(2)

    pdf.chapter_title("Systeme de build", level=2)
    pdf.body_text(
        "La compilation est geree par CMake 3.10+ de maniere multiplateforme :"
    )
    pdf.bullet_point("Windows : build.bat -> CMake + MSBuild (Visual Studio) -> build/Release/game.exe")
    pdf.bullet_point("Linux : Makefile avec g++ -std=c++17 -Wall -Wextra -O2")
    pdf.bullet_point("Flags MSVC : /W4 /EHsc /utf-8 (niveau d'avertissement eleve)")
    pdf.bullet_point("Assets automatiquement copies dans le repertoire de build par CMake")
    pdf.ln(2)

    pdf.chapter_title("Outils de developpement", level=2)
    pdf.bullet_point("Editeur : Visual Studio Code avec extensions C/C++")
    pdf.bullet_point("Compilateur : MSVC (Windows), g++ (Linux)")
    pdf.bullet_point("Controle de version : Git + GitHub")
    pdf.bullet_point("Debogage : Visual Studio Debugger / GDB")

    # ===================== 3. ARCHITECTURE =====================
    pdf.add_page()
    pdf.chapter_title("3. Architecture du projet")

    pdf.chapter_title("3.1 Structure des dossiers", level=2)
    pdf.body_text("Le projet est organise selon une separation claire moteur/jeu :")
    pdf.code_block(
        "consoleGame/\n"
        "  main.cpp              # Point d'entree, boucle de jeu\n"
        "  CMakeLists.txt        # Configuration de build\n"
        "  engine/               # Moteur reutilisable\n"
        "    ecs/                # Entity-Component-System\n"
        "      components/       # 15 composants (stats, inventaire, item...)\n"
        "      component_manager.hpp  # Gestion des entites/composants\n"
        "      component_serialization.hpp  # Serialisation JSON\n"
        "    systems/            # Systemes (combat, IA, inventaire...)\n"
        "    render/             # Console, Renderer, UI\n"
        "    scene/              # Scenes et menus\n"
        "    world/              # Generation procedurale, chunks\n"
        "    loaders/            # Chargement JSON (entites, structures)\n"
        "    input/              # Gestion des entrees clavier\n"
        "    util/               # JSON parser, save system\n"
        "  game/                 # Code specifique au jeu\n"
        "    scenes/             # Scenes gameplay (exploration, donjon)\n"
        "    entities/           # Factory, registre d'effets\n"
        "  assets/               # Donnees JSON\n"
        "    entities/           # Definitions des entites (joueur, ennemis, PNJ)\n"
        "    structures/         # Structures (villages, temples...)\n"
        "    worldgen/           # Parametres de generation du monde\n"
        "    effects/            # Definitions des effets"
    )

    pdf.add_page()
    pdf.chapter_title("3.2 Le systeme ECS (Entity-Component-System)", level=2)
    pdf.body_text(
        "L'architecture Entity-Component-System est le coeur du projet. Au lieu de l'heritage classique "
        "orientee-objet (Personnage -> Joueur, Ennemi, Allie), nous utilisons une approche par composition :"
    )
    pdf.ln(1)
    pdf.bold_text("Entity (Entite) : ", "Un simple identifiant numerique (EntityId = uint32_t). "
                   "Aucune donnee, aucun comportement.")
    pdf.ln(1)
    pdf.bold_text("Component (Composant) : ", "Structure de donnees pure attachee a une entite. "
                   "Exemples : StatsComponent, InventoryComponent, ItemComponent, FactionComponent.")
    pdf.ln(1)
    pdf.bold_text("System (Systeme) : ", "Logique qui opere sur les entites possedant certains composants. "
                   "Exemples : CombatSystem, AISystem, InventorySystem.")
    pdf.ln(3)
    pdf.body_text("Le ComponentManager gere la creation/destruction d'entites et le stockage des composants "
                  "via une map indexee par type (std::type_index) :")
    pdf.code_block(
        "class ComponentManager {\n"
        "    // Stockage: entity_id -> (type_index -> component)\n"
        "    unordered_map<EntityId,\n"
        "        unordered_map<type_index, unique_ptr<Component>>> components;\n"
        "\n"
        "    EntityId create_entity();\n"
        "    void destroy_entity(EntityId id);\n"
        "\n"
        "    template<typename T> void add_component(EntityId, T comp);\n"
        "    template<typename T> T* get_component(EntityId id);\n"
        "    template<typename T> bool has_component(EntityId id);\n"
        "    template<typename T> vector<EntityId> get_entities_with_component();\n"
        "};"
    )

    pdf.add_page()
    pdf.chapter_title("3.3 Separation Moteur / Jeu", level=2)
    pdf.body_text(
        "Le dossier engine/ contient le code reutilisable (ECS, rendu, systemes, scenes) tandis que "
        "game/ contient les implementations specifiques au jeu (scenes de gameplay, factory d'entites). "
        "Toutes les scenes recoivent Console*, ComponentManager*, Renderer*, UI* par constructeur, "
        "garantissant l'injection de dependance et evitant les singletons pour l'etat partage."
    )
    pdf.body_text(
        "Le SceneManager gere les transitions entre scenes (menu, exploration, donjon, game over) "
        "via des identifiants constants (SceneId::MENU, SceneId::EXPLORATION, etc.). Les scenes de "
        "gameplay sont marquees comme persistantes pour survivre aux pauses/reprises."
    )
    pdf.body_text(
        "La boucle principale dans main.cpp est simple : input -> update -> render a ~60 FPS :"
    )
    pdf.code_block(
        "void run() {\n"
        "    while (running && scene_manager.has_active_scene()) {\n"
        "        Key key = Input::get_key();\n"
        "        scene_manager.handle_input(key);\n"
        "        scene_manager.update();\n"
        "        scene_manager.render();\n"
        "        SLEEP_MS(16); // ~60 FPS\n"
        "    }\n"
        "}"
    )

    # ===================== 4. UML =====================
    pdf.add_page()
    pdf.chapter_title("4. Modelisation UML")

    pdf.chapter_title("4.1 Diagramme de classes (composants)", level=2)
    pdf.body_text(
        "Dans une architecture ECS, le diagramme de classes se concentre sur les composants "
        "(donnees) et les systemes (comportements). Voici les composants principaux :"
    )
    pdf.code_block(
        "                    +-------------------+\n"
        "                    |    Component      |  (classe abstraite)\n"
        "                    |-------------------|\n"
        "                    | +clone(): unique_ptr<Component>\n"
        "                    +-------------------+\n"
        "                             ^\n"
        "           __________________|____________________\n"
        "          |         |         |         |         |\n"
        "  +-------+--+ +---+------+ ++---------++ +------+----+\n"
        "  |StatsComp | |NameComp  | |ItemComp   | |RenderComp |\n"
        "  |----------| |----------| |-----------| |-----------|\n"
        "  |level     | |name      | |type: enum | |ch: char   |\n"
        "  |max_hp    | |descr.    | |equip_slot | |color      |\n"
        "  |current_hp| +----------+ |attack_bon.| +-----------+\n"
        "  |attack    |              |armor_bonus|\n"
        "  |defense   |              |value      |\n"
        "  +----------+              +-----------+\n"
        "\n"
        "  +------------+ +------------+ +----------------+\n"
        "  |InventoryC. | |FactionComp | |UtilityAIComp   |\n"
        "  |------------| |------------| |----------------|\n"
        "  |items: vec  | |faction: Id | |actions: vec    |\n"
        "  |max_items   | |reputation  | |aggro_range     |\n"
        "  |gold        | |enemies     | |home_x, home_y  |\n"
        "  +------------+ +------------+ +----------------+\n"
        "\n"
        "  +-------------+ +-----------+ +------------------+\n"
        "  |PositionComp | |NPCComp    | |StatusEffectsComp |\n"
        "  |-------------| |-----------| |------------------|\n"
        "  |x, y, z      | |disposition| |active_effects    |\n"
        "  |operator+,-  | |role       | |immunities        |\n"
        "  |operator<<   | |can_trade  | +------------------|\n"
        "  +-------------+ +-----------+"
    )

    pdf.add_page()
    pdf.chapter_title("4.2 Heritage et polymorphisme", level=2)
    pdf.body_text(
        "Bien que l'architecture ECS favorise la composition, l'heritage est utilise de maniere strategique :"
    )
    pdf.ln(1)
    pdf.bold_text("1) Composants : ", "Tous les composants heritent de la classe abstraite Component, "
                   "qui definit la methode virtuelle pure clone(). Le macro IMPLEMENT_CLONE genere "
                   "l'implementation pour chaque sous-classe concrete.")
    pdf.ln(1)
    pdf.bold_text("2) Scenes : ", "Toutes les scenes heritent de Scene (classe de base). Les scenes "
                   "de gameplay utilisent le template BaseGameplayScene<WorldType, FactoryType> qui "
                   "fournit des fonctionnalites communes (combat, inventaire, IA, rendu).")
    pdf.ln(1)
    pdf.bold_text("3) Polymorphisme de sous-typage : ", "L'enum ItemComponent::Type (CONSUMABLE, WEAPON, "
                   "ARMOR, HELMET, BOOTS, RING, SHIELD, QUEST, MISC) differencie les types d'objets. "
                   "L'enum FactionId differencie joueur, villageois, gardes, gobelins, morts-vivants, etc.")
    pdf.ln(1)
    pdf.bold_text("4) Templates : ", "Le systeme utilise intensivement les templates C++ : "
                   "ComponentManager::get_component<T>(), BaseGameplayScene<WorldType, FactoryType>, "
                   "et les fonctions de serialisation parametrees par type.")

    # ===================== 5. PERSONNAGES =====================
    pdf.add_page()
    pdf.chapter_title("5. Les personnages")
    pdf.body_text(
        "Dans l'ECS, il n'y a pas de classe \"Personnage\". Un personnage est une entite possedant "
        "les composants StatsComponent (PV, attaque, defense), NameComponent (nom), PositionComponent "
        "(position sur la carte), RenderComponent (affichage) et FactionComponent (faction/camp)."
    )

    pdf.chapter_title("5.1 Le joueur", level=2)
    pdf.body_text(
        "Le joueur est defini dans assets/entities/player.json avec les composants suivants :"
    )
    pdf.bullet_point("StatsComponent : 30 PV, 8 attaque, 5 defense, niveau 1, 5 points d'attributs")
    pdf.bullet_point("InventoryComponent : 20 emplacements, 100 or de depart")
    pdf.bullet_point("AnatomyComponent : preset humanoide (tete, torse, 2 bras, 2 mains, 2 jambes, 2 pieds) + queue personnalisee")
    pdf.bullet_point("FactionComponent : faction PLAYER")
    pdf.bullet_point("RenderComponent : caractere '@' en jaune, priorite 10 (affiche au-dessus)")
    pdf.bullet_point("CollisionComponent : bloque le mouvement")
    pdf.ln(2)

    pdf.chapter_title("5.2 Les ennemis", level=2)
    pdf.body_text(
        "Les ennemis sont des entites avec une FactionComponent hostile au joueur (GOBLIN, UNDEAD, "
        "BANDIT, DEMON, SPIDER, ORC, etc.). Ils possedent en plus un UtilityAIComponent qui definit "
        "leur comportement via des presets IA (aggressive_melee, berserker, ambush_predator, boss...). "
        "Les ennemis sont definis en JSON dans assets/entities/enemies/ et instancies par l'EntityLoader."
    )
    pdf.body_text(
        "Exemples d'ennemis : gobelin, squelette, loup, bandit, dragon. Chacun a ses propres statistiques "
        "scalees par niveau (hp_base + hp_per_level * (niveau-1)), son propre comportement IA, et son "
        "inventaire de butin que le joueur peut recuperer apres combat."
    )

    pdf.add_page()
    pdf.chapter_title("5.3 Les allies et PNJ", level=2)
    pdf.body_text(
        "Les PNJ amicaux ont un NPCComponent avec disposition FRIENDLY et un role (VILLAGER, MERCHANT, "
        "GUARD, QUEST_GIVER, TRAINER, INNKEEPER, WANDERER). Ils offrent differentes interactions :"
    )
    pdf.bullet_point("Parler (TALK) : Declenche un dialogue defini en JSON (DialogueComponent)")
    pdf.bullet_point("Commercer (TRADE) : Les marchands et aubergistes ont un ShopComponent avec inventaire")
    pdf.bullet_point("Voler (STEAL) : Tentative de pickpocket (les gardes sont plus vigilants)")
    pdf.bullet_point("Examiner (EXAMINE) : Affiche les informations detaillees sur le PNJ")
    pdf.ln(1)
    pdf.body_text(
        "Les gardes (faction GUARD) defendent les villageois et attaquent les entites hostiles via "
        "le preset IA town_guard. Les villageois avec une maison (preset villager_with_home) retournent "
        "chez eux la nuit. Les factions definissent des relations (ALLY, FRIENDLY, NEUTRAL, UNFRIENDLY, "
        "HOSTILE) via la FactionRelationTable."
    )

    # ===================== 6. OBJETS =====================
    pdf.add_page()
    pdf.chapter_title("6. Les objets et l'inventaire")

    pdf.chapter_title("6.1 Types d'objets", level=2)
    pdf.body_text("Les objets sont des entites avec un ItemComponent. L'enum ItemComponent::Type definit :")
    pdf.bullet_point("CONSUMABLE : Potions et consommables (utilisation unique)")
    pdf.bullet_point("WEAPON : Armes de melee (epees, haches, dagues)")
    pdf.bullet_point("ARMOR : Armures pour le torse")
    pdf.bullet_point("HELMET : Casques pour la tete")
    pdf.bullet_point("BOOTS : Bottes pour les pieds")
    pdf.bullet_point("GLOVES : Gants pour les mains")
    pdf.bullet_point("RING : Anneaux (doigt)")
    pdf.bullet_point("SHIELD : Boucliers (main secondaire)")
    pdf.bullet_point("QUEST : Objets de quete (non-consommables)")
    pdf.bullet_point("MISC : Divers")
    pdf.ln(2)

    pdf.chapter_title("6.2 Potions et effets", level=2)
    pdf.body_text(
        "Les potions sont des objets de type CONSUMABLE avec un ItemEffectComponent definissant des effets "
        "au declenchement ON_USE. Les types d'effets incluent HEAL_HP, RESTORE_HP_PERCENT, BUFF_ATTACK, "
        "BUFF_DEFENSE, POISON, REGENERATION, TELEPORT, GAIN_XP, et CUSTOM (effet enregistre dans "
        "l'EffectRegistry). Le systeme supporte les effets instantanes et les effets dans la duree "
        "via le StatusEffectSystem."
    )
    pdf.body_text(
        "Le sujet mentionne que les potions soignent automatiquement le personnage s'il tombe a 0 PV. "
        "Cette mecanique est implementee dans le CombatSystem : lorsqu'une entite subit des degats la "
        "reduisant a 0 PV, le systeme verifie son inventaire pour une potion de soin et l'utilise "
        "automatiquement avant de declarer la mort."
    )

    pdf.add_page()
    pdf.chapter_title("6.3 Armes et armures", level=2)
    pdf.body_text(
        "Les armes (WEAPON) et armures (ARMOR, HELMET, BOOTS, GLOVES, SHIELD) ont des bonus de statistiques "
        "dans l'ItemComponent :"
    )
    pdf.bullet_point("attack_bonus : Bonus d'attaque lorsque l'arme est equipee")
    pdf.bullet_point("base_damage : Degats de base de l'arme (les degats de melee dependent de l'arme)")
    pdf.bullet_point("armor_bonus : Bonus de defense lorsque l'armure est equipee")
    pdf.bullet_point("defense_bonus : Bonus de defense supplementaire")
    pdf.bullet_point("cutting_chance : Probabilite de trancher un membre (systeme d'anatomie)")
    pdf.ln(1)
    pdf.body_text(
        "Le systeme supporte aussi les armes a distance (RangedWeaponComponent) avec types de munitions "
        "(fleches, carreaux, pierres), portee, et verification de ligne de vue."
    )

    pdf.chapter_title("6.4 Equipement et anatomie", level=2)
    pdf.body_text(
        "Le systeme d'equipement est lie a l'AnatomyComponent qui definit les membres du corps de l'entite. "
        "Chaque membre (Limb) a un type (HEAD, TORSO, ARM, HAND, LEG, FOOT), un emplacement d'equipement, "
        "et une resistance au tranchage. Un personnage peut equiper au maximum ce que son anatomie permet : "
        "typiquement une arme par main, une armure au torse, un casque, des bottes, etc."
    )
    pdf.body_text(
        "L'InventorySystem gere l'equipement/desequipement avec application automatique des bonus de "
        "statistiques. Lorsqu'on retire une arme, le bonus d'attaque est supprime ; lorsqu'on enleve "
        "une armure, le bonus de defense est retire."
    )

    # ===================== 7. COMBAT =====================
    pdf.add_page()
    pdf.chapter_title("7. Le systeme de combat")
    pdf.body_text(
        "Le CombatSystem gere toute la logique de combat. Le fonctionnement correspond a la specification "
        "du sujet, avec des extensions significatives :"
    )
    pdf.ln(1)
    pdf.bold_text("Calcul des degats : ", "degats = attaque_assaillant - defense_defenseur (minimum 1). "
                   "Les armes equipees ajoutent leur base_damage et attack_bonus.")
    pdf.ln(1)
    pdf.bold_text("Combat multi-armes : ", "Le systeme resout l'attaque pour chaque arme equipee. "
                   "Les mains libres permettent des frappes a mains nues supplementaires.")
    pdf.ln(1)
    pdf.bold_text("Effets au toucher : ", "Les armes avec des effets ON_HIT (poison, drain de vie, etc.) "
                   "se declenchent selon leur probabilite (chance).")
    pdf.ln(1)
    pdf.bold_text("Tranchage de membres : ", "Les armes avec cutting_chance > 0 peuvent sevrer un membre "
                   "de l'anatomie de la cible (en fonction de la resistance du membre).")
    pdf.ln(1)
    pdf.bold_text("Mort et butin : ", "Quand une entite meurt, handle_entity_death() cree un cadavre "
                   "(CorpseComponent) et transfere les objets. Le joueur peut recuperer le butin.")
    pdf.ln(1)
    pdf.bold_text("Factions : ", "L'hostilite est determinee par les factions. Attaquer un garde "
                   "provoque la retaliation de tous les gardes proches (alert_allies).")
    pdf.ln(1)
    pdf.bold_text("Armes a distance : ", "Le systeme supporte tir a distance avec verification de "
                   "portee, ligne de vue, consommation de munitions et animation de projectile.")
    pdf.ln(1)
    pdf.body_text(
        "Le premier attaquant n'est pas aleatoire dans notre implementation (contrairement au sujet), "
        "mais determine par l'initiative : le joueur attaque en premier s'il initie l'action, sinon "
        "c'est l'ennemi (via le systeme IA) qui frappe d'abord."
    )

    # ===================== 8. RENCONTRES =====================
    pdf.add_page()
    pdf.chapter_title("8. Rencontres et interactions")
    pdf.body_text(
        "L'InteractionSystem gere les rencontres du joueur :"
    )
    pdf.ln(1)
    pdf.bold_text("Rencontre d'un ennemi : ", "Lorsque le joueur se deplace vers une case occupee par "
                   "une entite hostile, le CombatSystem est automatiquement declenche. Le joueur frappe "
                   "avec toutes ses armes equipees, puis l'ennemi contre-attaque s'il survit.")
    pdf.ln(1)
    pdf.bold_text("Rencontre d'un allie/PNJ : ", "Le joueur peut interagir via un menu contextuel "
                   "proposant Parler, Commercer, Voler ou Examiner selon le role du PNJ. "
                   "Le DialogueSystem affiche les dialogues definis en JSON avec des choix.")
    pdf.ln(1)
    pdf.bold_text("Rencontre d'un objet : ", "Le joueur peut ramasser les objets au sol via le systeme "
                   "de pickup (InventorySystem::pickup_items). Il est libre de les prendre ou non. "
                   "Si l'inventaire est plein, le ramassage echoue avec un message.")
    pdf.ln(1)
    pdf.bold_text("Coffres et conteneurs : ", "Les coffres (InteractableComponent de type CHEST) "
                   "peuvent etre ouverts pour reveler leur contenu. Les portes peuvent etre "
                   "ouvertes/fermees, modifiant la collision et le rendu.")
    pdf.ln(1)
    pdf.bold_text("Commerce : ", "Les marchands ont un ShopComponent avec un inventaire d'articles "
                   "a vendre. Le joueur peut acheter et vendre des objets contre de l'or.")

    # ===================== 9. SURCHARGE D'OPERATEURS =====================
    pdf.add_page()
    pdf.chapter_title("9. Surcharge d'operateurs")
    pdf.body_text(
        "Conformement a la demande du sujet, plusieurs operateurs sont surcharges de maniere utile "
        "dans le projet. Le composant PositionComponent (engine/ecs/components/position_component.hpp) "
        "definit les surcharges suivantes :"
    )
    pdf.ln(2)
    pdf.code_block(
        "struct PositionComponent : Component {\n"
        "    int x = 0, y = 0, z = 0;\n"
        "\n"
        "    // Egalite / inegalite\n"
        "    bool operator==(const PositionComponent& other) const {\n"
        "        return x == other.x && y == other.y && z == other.z;\n"
        "    }\n"
        "    bool operator!=(const PositionComponent& other) const {\n"
        "        return !(*this == other);\n"
        "    }\n"
        "\n"
        "    // Arithmetique vectorielle (addition/soustraction)\n"
        "    PositionComponent operator+(const PositionComponent& o) const {\n"
        "        return PositionComponent(x+o.x, y+o.y, z+o.z);\n"
        "    }\n"
        "    PositionComponent operator-(const PositionComponent& o) const {\n"
        "        return PositionComponent(x-o.x, y-o.y, z-o.z);\n"
        "    }\n"
        "\n"
        "    // Affectation composee\n"
        "    PositionComponent& operator+=(const PositionComponent& o) {\n"
        "        x += o.x; y += o.y; z += o.z; return *this;\n"
        "    }\n"
        "    PositionComponent& operator-=(const PositionComponent& o) {\n"
        "        x -= o.x; y -= o.y; z -= o.z; return *this;\n"
        "    }\n"
        "\n"
        "    // Insertion dans un flux (debug)\n"
        "    friend ostream& operator<<(ostream& os, const PositionComponent& p) {\n"
        "        return os << \"(\" << p.x << \", \" << p.y << \", \" << p.z << \")\";\n"
        "    }\n"
        "};"
    )
    pdf.ln(2)
    pdf.body_text("Utilite de ces surcharges :")
    pdf.bullet_point("operator== / != : Utilise partout pour comparer des positions (collision, detection d'ennemis, pathfinding)")
    pdf.bullet_point("operator+ / - : Permet le calcul vectoriel de deplacement (position + direction = nouvelle_position)")
    pdf.bullet_point("operator+= / -= : Deplacements in-place sans allocation de nouvel objet")
    pdf.bullet_point("operator<< : Facilite l'affichage de debug (cout << position)")
    pdf.ln(1)
    pdf.body_text(
        "De plus, l'operateur operator[] est surcharge dans la classe json::Value (engine/util/json.hpp) "
        "pour acceder aux champs JSON de maniere intuitive, et l'operateur operator== est surcharge dans "
        "ChunkCoord (engine/world/chunk_coord.hpp) avec un foncteur de hachage operator() pour "
        "l'utilisation comme cle dans les unordered_map."
    )

    # ===================== 10. SAUVEGARDE =====================
    pdf.add_page()
    pdf.chapter_title("10. Systeme de sauvegarde et chargement")
    pdf.body_text(
        "Le systeme de sauvegarde (engine/util/save_system.hpp) permet une persistance complete de "
        "l'etat du jeu. Il sérialise toutes les donnees en JSON et les ecrit dans des fichiers "
        "(saves/save_slot_N.json). Le systeme supporte 5 emplacements de sauvegarde."
    )
    pdf.ln(1)
    pdf.chapter_title("Donnees sauvegardees", level=3)
    pdf.bullet_point("Etat du joueur : PV, attaque, defense, niveau, XP, or, position")
    pdf.bullet_point("Composants complets du joueur : anatomie, equipement, effets de statut (JSON)")
    pdf.bullet_point("Inventaire : tous les objets avec leurs composants serialises (effets, stats)")
    pdf.bullet_point("Etat du monde : graine de generation, chunks visites, entites par chunk")
    pdf.bullet_point("Etat du donjon : type, profondeur, graine, position de retour")
    pdf.bullet_point("Entites du monde : PNJ, ennemis, objets au sol (stats, IA, faction, dialogue)")
    pdf.bullet_point("Metadonnees : nom du joueur, temps de jeu, horodatage, scene en cours")
    pdf.ln(1)
    pdf.chapter_title("Serialisation des composants", level=3)
    pdf.body_text(
        "Chaque composant a des fonctions serialize_xxx() et deserialize_xxx() dans "
        "component_serialization.hpp. Les enums sont convertis en chaines (enum_converters.hpp) pour "
        "la lisibilite humaine du JSON. Les entites imbriquees (objets d'inventaire) sont serialisees "
        "recursivement avec remappage d'identifiants (id_remap)."
    )
    pdf.code_block(
        "// Exemple : serialisation de StatsComponent\n"
        "json::Object serialize_stats(const StatsComponent& comp) {\n"
        "    json::Object obj;\n"
        "    obj[\"_type\"] = \"stats\";\n"
        "    obj[\"level\"] = comp.level;\n"
        "    obj[\"max_hp\"] = comp.max_hp;\n"
        "    obj[\"current_hp\"] = comp.current_hp;\n"
        "    obj[\"attack\"] = comp.attack;\n"
        "    obj[\"defense\"] = comp.defense;\n"
        "    return obj;\n"
        "}"
    )

    # ===================== 11. WORLD GEN =====================
    pdf.add_page()
    pdf.chapter_title("11. Generation procedurale du monde")
    pdf.body_text(
        "Le monde est infini et genere proceduralement par chunks de 32x32 tuiles. La generation utilise "
        "5 couches de bruit de Perlin (continent, biome, humidite, temperature, detail) pour produire "
        "un terrain coherent avec des biomes distincts (plaines, forets, desert, montagnes, marais, neige...)."
    )
    pdf.ln(1)
    pdf.chapter_title("Biomes", level=3)
    pdf.body_text(
        "Les biomes sont definis dans assets/worldgen/biomes.json avec leurs tuiles de terrain, "
        "seuils de vegetation, modificateurs de deplacement et de visibilite, et textes d'ambiance. "
        "La selection du biome est determinee par l'elevation et l'humidite a chaque position."
    )
    pdf.chapter_title("Structures", level=3)
    pdf.body_text(
        "Le ChunkGenerator place des structures (villages, temples, donjons, fermes, tavernes...) "
        "definies en JSON (assets/structures/). L'enum StructureType definit 27+ types de structures. "
        "Les structures sont composees de formes (filled_rect, hollow_rect, circle, scatter) et de "
        "spawns d'entites (PNJ, ennemis, objets) avec positions, rayon, nombre et probabilite."
    )
    pdf.chapter_title("Donjons", level=3)
    pdf.body_text(
        "Les donjons sont generes par le ProceduralGenerator avec deux techniques : automates cellulaires "
        "(grottes organiques) et salles+couloirs (donjons classiques). Chaque donjon a une profondeur, "
        "une difficulte, et des decorations procedurales (torches, coffres, spawns d'ennemis)."
    )

    # ===================== 12. IA =====================
    pdf.add_page()
    pdf.chapter_title("12. Intelligence artificielle (Utility AI)")
    pdf.body_text(
        "L'IA des PNJ et ennemis utilise un systeme de decision par utilite (Utility AI) ou chaque "
        "action recoit un score de 0.0 a 1.0 base sur des considerations ponderees. L'action avec "
        "le score le plus eleve est executee chaque tour."
    )
    pdf.ln(1)
    pdf.chapter_title("Concepts cles", level=3)
    pdf.bullet_point("Action (UtilityAction) : Comportement possible (ATTACK_MELEE, FLEE, WANDER, DEFEND_ALLY, HEAL_SELF, RETURN_HOME, SLEEP...)")
    pdf.bullet_point("Consideration : Entree qui affecte le score (sante %, distance a la cible, nombre d'allies...)")
    pdf.bullet_point("Courbe de reponse (ResponseCurve) : Transforme l'entree en score (LINEAR, INVERSE, STEP, QUADRATIC, BELL_CURVE...)")
    pdf.bullet_point("Contexte : Donnees de l'environnement (PV, position, factions, jour/nuit...)")
    pdf.ln(2)
    pdf.chapter_title("Presets de comportement", level=3)
    pdf.body_text("Le systeme fournit 16+ presets predefinies dans UtilityPresets:: :")
    pdf.bullet_point("Combat : aggressive_melee, defensive_guard, berserker, boss, pack_hunter, cowardly")
    pdf.bullet_point("Animaux : wildlife, predator, ambush_predator, undead")
    pdf.bullet_point("PNJ : peaceful_villager, villager_with_home, town_guard, merchant, wanderer")
    pdf.ln(1)
    pdf.body_text(
        "Exemple : un garde utilise le preset town_guard qui priorise DEFEND_ALLY quand un allie est "
        "attaque, ATTACK_MELEE quand un ennemi est en portee, et PATROL sinon. Un gobelin berserker "
        "attaque en priorite quand ses PV sont bas (courbe inverse sur la sante)."
    )

    # ===================== 13. DATA-DRIVEN =====================
    pdf.add_page()
    pdf.chapter_title("13. Conception data-driven (JSON)")
    pdf.body_text(
        "Toutes les entites, structures et parametres du monde sont definis dans des fichiers JSON, "
        "pas en code C++. Cela permet d'ajouter de nouveaux contenus sans recompilation. "
        "L'EntityLoader charge les templates au demarrage depuis assets/entities/ et les instancie "
        "a la volee via create_from_template(id, x, y)."
    )
    pdf.ln(1)
    pdf.chapter_title("Exemple : definition d'une entite en JSON", level=3)
    pdf.code_block(
        "// assets/entities/player.json\n"
        "{\n"
        "    \"id\": \"player\",\n"
        "    \"name\": \"Explorer\",\n"
        "    \"category\": \"player\",\n"
        "    \"components\": {\n"
        "        \"render\": { \"char\": \"@\", \"color\": \"yellow\", \"priority\": 10 },\n"
        "        \"stats\": { \"hp_base\": 30, \"attack_base\": 8, \"defense_base\": 5 },\n"
        "        \"inventory\": { \"max_items\": 20, \"gold\": 100 },\n"
        "        \"anatomy\": { \"preset\": \"humanoid\" },\n"
        "        \"faction\": { \"faction\": \"player\" },\n"
        "        \"collision\": { \"blocks_movement\": true }\n"
        "    }\n"
        "}"
    )
    pdf.ln(1)
    pdf.body_text(
        "Les effets des objets sont aussi data-driven (assets/effects/item_effects.json) et enregistres "
        "dans l'EffectRegistry au demarrage. Les structures (villages, temples, etc.) definissent leurs "
        "formes, tuiles et spawns d'entites en JSON. Les biomes sont configures dans "
        "assets/worldgen/biomes.json."
    )

    # ===================== 14. COMPILATION =====================
    pdf.add_page()
    pdf.chapter_title("14. Compilation et execution")

    pdf.chapter_title("Windows", level=2)
    pdf.code_block(
        "# Prerequis : Visual Studio 2019+ avec toolset C++\n"
        "# et CMake >= 3.10 dans le PATH\n"
        "\n"
        "> build.bat\n"
        "# Genere build/Release/game.exe\n"
        "\n"
        "> cd build\\Release\n"
        "> game.exe"
    )

    pdf.chapter_title("Linux", level=2)
    pdf.code_block(
        "# Prerequis : g++ >= 8 (support C++17)\n"
        "\n"
        "$ make\n"
        "# Genere ./game\n"
        "\n"
        "$ ./game"
    )

    pdf.ln(2)
    pdf.chapter_title("Fichiers compiles", level=3)
    pdf.body_text("Le projet compile 8 fichiers source :")
    pdf.bullet_point("main.cpp - Point d'entree et classe Game")
    pdf.bullet_point("engine/src/console.cpp - Rendering console multiplateforme")
    pdf.bullet_point("engine/src/ui.cpp - Interface utilisateur (menus, HUD)")
    pdf.bullet_point("engine/src/component_manager.cpp - Gestion des composants ECS")
    pdf.bullet_point("engine/src/ai_system.cpp - Systeme d'intelligence artificielle")
    pdf.bullet_point("engine/src/combat_system.cpp - Systeme de combat (1123 lignes)")
    pdf.bullet_point("engine/src/inventory_system.cpp - Gestion de l'inventaire et equipement")
    pdf.bullet_point("engine/src/interaction_system.cpp - Interactions (portes, coffres, PNJ)")

    # ===================== 15. CONCLUSION =====================
    pdf.add_page()
    pdf.chapter_title("15. Conclusion")
    pdf.body_text(
        "Ce projet repond a toutes les exigences du TP3 Hexagone 2026 tout en les depassant "
        "significativement :"
    )
    pdf.ln(1)
    pdf.bullet_point("Personnages avec PV, attaque, defense, inventaire : StatsComponent + InventoryComponent dans l'ECS")
    pdf.bullet_point("Joueur, ennemis, allies : Differencies par FactionComponent et UtilityAIComponent")
    pdf.bullet_point("Potions, armes, armures : ItemComponent avec types enum + ItemEffectComponent pour les effets")
    pdf.bullet_point("Potions auto-soin a 0 PV : Verifie dans le CombatSystem avant la declaration de mort")
    pdf.bullet_point("Equipement arme/armure : AnatomyComponent avec Limb pour des equipements anatomiques realistes")
    pdf.bullet_point("Combat tour par tour : CombatSystem avec degats = attaque - defense, multi-armes, effets au toucher")
    pdf.bullet_point("Butin apres victoire : Cadavre cree avec transfert d'inventaire, ramassage par le joueur")
    pdf.bullet_point("Interaction avec allies : Menu contextuel (parler, commercer, voler, examiner)")
    pdf.bullet_point("Ramassage d'objets : Libre, base sur la taille de l'inventaire")
    pdf.bullet_point("Surcharge d'operateurs : 7 operateurs surcharges sur PositionComponent (+, -, +=, -=, ==, !=, <<)")
    pdf.bullet_point("Sauvegarde/chargement : Serialisation JSON complete avec 5 emplacements et entites persistantes")
    pdf.ln(2)
    pdf.body_text(
        "Au-dela du cahier des charges, le projet integre : un monde infini procedural par bruit de "
        "Perlin, une IA par utilite avec 16+ presets comportementaux, un systeme de factions avec "
        "relations dynamiques, un systeme d'anatomie avec membre tranchables, 27+ types de structures, "
        "des effets de statut (poison, saignement, buffs), des armes a distance avec munitions, et "
        "une architecture entierement data-driven en JSON."
    )
    pdf.ln(5)
    pdf.set_font("Helvetica", "I", 12)
    pdf.set_text_color(60, 60, 60)
    pdf.cell(0, 10, "Mathias Kowalski", align="R", new_x="LMARGIN", new_y="NEXT")
    pdf.set_font("Helvetica", "", 10)
    pdf.cell(0, 8, "Fevrier 2026", align="R", new_x="LMARGIN", new_y="NEXT")

    # Save
    output_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "rapport_TP3_Hexagone_2026.pdf")
    pdf.output(output_path)
    print(f"Report generated: {output_path}")
    return output_path


if __name__ == "__main__":
    generate_report()
