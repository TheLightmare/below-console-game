#include "../ecs/component.hpp"

// FactionManager implementation

FactionManager::FactionManager() {
    // Initialize all relationships to neutral
    for (size_t i = 0; i < static_cast<size_t>(FactionId::FACTION_COUNT); ++i) {
        for (size_t j = 0; j < static_cast<size_t>(FactionId::FACTION_COUNT); ++j) {
            relationships[i][j] = FactionRelation::NEUTRAL;
        }
        // Everyone is allied with themselves
        relationships[i][i] = FactionRelation::ALLY;
    }
    
    setup_default_relationships();
}

void FactionManager::setup_default_relationships() {
    // Player relationships - use mutual hostility so monsters also attack player
    set_relation(FactionId::PLAYER, FactionId::VILLAGER, FactionRelation::FRIENDLY);
    set_relation(FactionId::PLAYER, FactionId::GUARD, FactionRelation::FRIENDLY);
    set_relation(FactionId::PLAYER, FactionId::WILDLIFE, FactionRelation::NEUTRAL);
    set_mutual_hostility(FactionId::PLAYER, FactionId::PREDATOR);
    set_mutual_hostility(FactionId::PLAYER, FactionId::GOBLIN);
    set_mutual_hostility(FactionId::PLAYER, FactionId::UNDEAD);
    set_mutual_hostility(FactionId::PLAYER, FactionId::BANDIT);
    set_mutual_hostility(FactionId::PLAYER, FactionId::DEMON);
    set_mutual_hostility(FactionId::PLAYER, FactionId::DRAGON);
    set_mutual_hostility(FactionId::PLAYER, FactionId::SPIDER);
    set_mutual_hostility(FactionId::PLAYER, FactionId::ORC);
    set_mutual_hostility(FactionId::PLAYER, FactionId::BEAST);
    
    // Villagers - friendly with guards and player
    set_relation(FactionId::VILLAGER, FactionId::PLAYER, FactionRelation::FRIENDLY);
    set_relation(FactionId::VILLAGER, FactionId::GUARD, FactionRelation::ALLY);
    set_mutual_hostility(FactionId::VILLAGER, FactionId::GOBLIN);
    set_mutual_hostility(FactionId::VILLAGER, FactionId::UNDEAD);
    set_mutual_hostility(FactionId::VILLAGER, FactionId::BANDIT);
    set_mutual_hostility(FactionId::VILLAGER, FactionId::PREDATOR);
    
    // Guards - protect villagers and player
    set_relation(FactionId::GUARD, FactionId::PLAYER, FactionRelation::FRIENDLY);
    set_relation(FactionId::GUARD, FactionId::VILLAGER, FactionRelation::ALLY);
    set_mutual_hostility(FactionId::GUARD, FactionId::GOBLIN);
    set_mutual_hostility(FactionId::GUARD, FactionId::UNDEAD);
    set_mutual_hostility(FactionId::GUARD, FactionId::BANDIT);
    set_mutual_hostility(FactionId::GUARD, FactionId::ORC);
    set_mutual_hostility(FactionId::GUARD, FactionId::DEMON);
    set_mutual_hostility(FactionId::GUARD, FactionId::PREDATOR);
    
    // Wildlife - neutral to most, flees from predators
    set_relation(FactionId::WILDLIFE, FactionId::PREDATOR, FactionRelation::UNFRIENDLY);
    
    // Predators - hunt wildlife, hostile to player
    set_relation(FactionId::PREDATOR, FactionId::WILDLIFE, FactionRelation::HOSTILE);
    set_mutual_hostility(FactionId::PREDATOR, FactionId::PLAYER);
    
    // Monster faction hostilities
    set_mutual_hostility(FactionId::GOBLIN, FactionId::ORC); // Goblins and orcs don't like each other
    set_mutual_hostility(FactionId::UNDEAD, FactionId::DEMON); // Fight over dark powers
    
    // Spiders are hostile to almost everyone
    set_mutual_hostility(FactionId::SPIDER, FactionId::GOBLIN);
    set_mutual_hostility(FactionId::SPIDER, FactionId::ORC);
    set_mutual_hostility(FactionId::SPIDER, FactionId::WILDLIFE);
    
    // Dragons are apex predators - hostile to most
    set_mutual_hostility(FactionId::DRAGON, FactionId::GOBLIN);
    set_mutual_hostility(FactionId::DRAGON, FactionId::ORC);
    set_mutual_hostility(FactionId::DRAGON, FactionId::UNDEAD);
    
    // Bandits fight with goblins and orcs
    set_mutual_hostility(FactionId::BANDIT, FactionId::GOBLIN);
    set_mutual_hostility(FactionId::BANDIT, FactionId::ORC);
    set_mutual_hostility(FactionId::BANDIT, FactionId::UNDEAD);
}

void FactionManager::set_relation(FactionId from, FactionId to, FactionRelation relation) {
    relationships[static_cast<size_t>(from)][static_cast<size_t>(to)] = relation;
}

void FactionManager::set_mutual_hostility(FactionId a, FactionId b) {
    set_relation(a, b, FactionRelation::HOSTILE);
    set_relation(b, a, FactionRelation::HOSTILE);
}

void FactionManager::set_mutual_alliance(FactionId a, FactionId b) {
    set_relation(a, b, FactionRelation::ALLY);
    set_relation(b, a, FactionRelation::ALLY);
}

FactionRelation FactionManager::get_relation(FactionId from, FactionId to) const {
    return relationships[static_cast<size_t>(from)][static_cast<size_t>(to)];
}

bool FactionManager::is_hostile(FactionId from, FactionId to) const {
    return get_relation(from, to) == FactionRelation::HOSTILE;
}

bool FactionManager::is_allied(FactionId from, FactionId to) const {
    FactionRelation rel = get_relation(from, to);
    return rel == FactionRelation::ALLY;
}

bool FactionManager::is_friendly(FactionId from, FactionId to) const {
    FactionRelation rel = get_relation(from, to);
    return rel == FactionRelation::ALLY || rel == FactionRelation::FRIENDLY;
}

FactionRelation FactionManager::get_effective_relation(const FactionComponent* from_faction, 
                                                       const FactionComponent* to_faction,
                                                       EntityId to_id) const {
    if (!from_faction || !to_faction) return FactionRelation::NEUTRAL;
    
    // Check personal enemies first
    if (from_faction->is_personal_enemy(to_id)) {
        return FactionRelation::HOSTILE;
    }
    
    // Check personal allies
    if (from_faction->is_personal_ally(to_id)) {
        return FactionRelation::ALLY;
    }
    
    // Fall back to faction relationship
    return get_relation(from_faction->faction, to_faction->faction);
}

bool FactionManager::should_attack(const FactionComponent* attacker, const FactionComponent* target, EntityId target_id) const {
    FactionRelation rel = get_effective_relation(attacker, target, target_id);
    return rel == FactionRelation::HOSTILE;
}

bool FactionManager::should_defend(const FactionComponent* defender, const FactionComponent* ally, EntityId ally_id) const {
    FactionRelation rel = get_effective_relation(defender, ally, ally_id);
    return rel == FactionRelation::ALLY || defender->is_personal_ally(ally_id);
}
