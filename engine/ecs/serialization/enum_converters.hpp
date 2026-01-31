#pragma once
// Centralized enum-to-string and string-to-enum converters
// Include this file when you need to serialize/deserialize game enums.

#include "../components/faction_component.hpp"
#include "../components/inventory_component.hpp"
#include "../components/item_component.hpp"
#include "../components/npc_component.hpp"
#include "../../systems/utility_ai.hpp"
#include <string>
#include <unordered_map>

namespace EnumConverters {

// ==================== LimbType ====================

inline std::string limb_type_to_string(LimbType type) {
    switch (type) {
        case LimbType::HEAD: return "head";
        case LimbType::TORSO: return "torso";
        case LimbType::ARM: return "arm";
        case LimbType::HAND: return "hand";
        case LimbType::LEG: return "leg";
        case LimbType::FOOT: return "foot";
        case LimbType::OTHER: return "other";
        default: return "other";
    }
}

inline LimbType string_to_limb_type(const std::string& str) {
    static const std::unordered_map<std::string, LimbType> map = {
        {"head", LimbType::HEAD},
        {"torso", LimbType::TORSO},
        {"arm", LimbType::ARM},
        {"hand", LimbType::HAND},
        {"leg", LimbType::LEG},
        {"foot", LimbType::FOOT},
        {"other", LimbType::OTHER}
    };
    auto it = map.find(str);
    return (it != map.end()) ? it->second : LimbType::OTHER;
}

// ==================== ItemComponent::Type ====================

inline std::string item_type_to_string(ItemComponent::Type type) {
    switch (type) {
        case ItemComponent::Type::WEAPON: return "weapon";
        case ItemComponent::Type::ARMOR: return "armor";
        case ItemComponent::Type::CONSUMABLE: return "consumable";
        case ItemComponent::Type::QUEST: return "quest";
        case ItemComponent::Type::HELMET: return "helmet";
        case ItemComponent::Type::BOOTS: return "boots";
        case ItemComponent::Type::GLOVES: return "gloves";
        case ItemComponent::Type::RING: return "ring";
        case ItemComponent::Type::SHIELD: return "shield";
        case ItemComponent::Type::MISC: return "misc";
        default: return "misc";
    }
}

inline ItemComponent::Type string_to_item_type(const std::string& str) {
    static const std::unordered_map<std::string, ItemComponent::Type> map = {
        {"weapon", ItemComponent::Type::WEAPON},
        {"armor", ItemComponent::Type::ARMOR},
        {"consumable", ItemComponent::Type::CONSUMABLE},
        {"quest", ItemComponent::Type::QUEST},
        {"helmet", ItemComponent::Type::HELMET},
        {"boots", ItemComponent::Type::BOOTS},
        {"gloves", ItemComponent::Type::GLOVES},
        {"ring", ItemComponent::Type::RING},
        {"shield", ItemComponent::Type::SHIELD},
        {"misc", ItemComponent::Type::MISC}
    };
    auto it = map.find(str);
    return (it != map.end()) ? it->second : ItemComponent::Type::MISC;
}

// ==================== ItemComponent::EquipSlot ====================

inline std::string equip_slot_to_string(ItemComponent::EquipSlot slot) {
    switch (slot) {
        case ItemComponent::EquipSlot::HEAD: return "head";
        case ItemComponent::EquipSlot::TORSO: return "torso";
        case ItemComponent::EquipSlot::HAND: return "hand";
        case ItemComponent::EquipSlot::TWO_HAND: return "two_hand";
        case ItemComponent::EquipSlot::FOOT: return "foot";
        case ItemComponent::EquipSlot::FINGER: return "finger";
        case ItemComponent::EquipSlot::BACK: return "back";
        case ItemComponent::EquipSlot::NECK: return "neck";
        case ItemComponent::EquipSlot::WAIST: return "waist";
        case ItemComponent::EquipSlot::NONE: return "none";
        default: return "none";
    }
}

inline ItemComponent::EquipSlot string_to_equip_slot(const std::string& str) {
    static const std::unordered_map<std::string, ItemComponent::EquipSlot> map = {
        {"head", ItemComponent::EquipSlot::HEAD},
        {"torso", ItemComponent::EquipSlot::TORSO},
        {"hand", ItemComponent::EquipSlot::HAND},
        {"two_hand", ItemComponent::EquipSlot::TWO_HAND},
        {"foot", ItemComponent::EquipSlot::FOOT},
        {"finger", ItemComponent::EquipSlot::FINGER},
        {"back", ItemComponent::EquipSlot::BACK},
        {"neck", ItemComponent::EquipSlot::NECK},
        {"waist", ItemComponent::EquipSlot::WAIST},
        {"none", ItemComponent::EquipSlot::NONE}
    };
    auto it = map.find(str);
    return (it != map.end()) ? it->second : ItemComponent::EquipSlot::NONE;
}

// ==================== NPCComponent::Disposition ====================

inline std::string disposition_to_string(NPCComponent::Disposition d) {
    switch (d) {
        case NPCComponent::Disposition::FRIENDLY: return "friendly";
        case NPCComponent::Disposition::NEUTRAL: return "neutral";
        case NPCComponent::Disposition::HOSTILE: return "hostile";
        case NPCComponent::Disposition::FEARFUL: return "fearful";
        default: return "neutral";
    }
}

inline NPCComponent::Disposition string_to_disposition(const std::string& str) {
    static const std::unordered_map<std::string, NPCComponent::Disposition> map = {
        {"friendly", NPCComponent::Disposition::FRIENDLY},
        {"neutral", NPCComponent::Disposition::NEUTRAL},
        {"hostile", NPCComponent::Disposition::HOSTILE},
        {"fearful", NPCComponent::Disposition::FEARFUL}
    };
    auto it = map.find(str);
    return (it != map.end()) ? it->second : NPCComponent::Disposition::NEUTRAL;
}

// ==================== NPCComponent::Role ====================

inline std::string role_to_string(NPCComponent::Role r) {
    switch (r) {
        case NPCComponent::Role::VILLAGER: return "villager";
        case NPCComponent::Role::MERCHANT: return "merchant";
        case NPCComponent::Role::GUARD: return "guard";
        case NPCComponent::Role::QUEST_GIVER: return "quest_giver";
        case NPCComponent::Role::TRAINER: return "trainer";
        case NPCComponent::Role::INNKEEPER: return "innkeeper";
        case NPCComponent::Role::WANDERER: return "wanderer";
        default: return "villager";
    }
}

inline NPCComponent::Role string_to_role(const std::string& str) {
    static const std::unordered_map<std::string, NPCComponent::Role> map = {
        {"villager", NPCComponent::Role::VILLAGER},
        {"merchant", NPCComponent::Role::MERCHANT},
        {"guard", NPCComponent::Role::GUARD},
        {"quest_giver", NPCComponent::Role::QUEST_GIVER},
        {"trainer", NPCComponent::Role::TRAINER},
        {"innkeeper", NPCComponent::Role::INNKEEPER},
        {"wanderer", NPCComponent::Role::WANDERER}
    };
    auto it = map.find(str);
    return (it != map.end()) ? it->second : NPCComponent::Role::VILLAGER;
}

// ==================== RangedWeaponComponent::AmmoType ====================

inline std::string ammo_type_to_string(RangedWeaponComponent::AmmoType type) {
    switch (type) {
        case RangedWeaponComponent::AmmoType::NONE: return "none";
        case RangedWeaponComponent::AmmoType::ARROW: return "arrow";
        case RangedWeaponComponent::AmmoType::BOLT: return "bolt";
        case RangedWeaponComponent::AmmoType::STONE: return "stone";
        case RangedWeaponComponent::AmmoType::BULLET: return "bullet";
        case RangedWeaponComponent::AmmoType::MAGIC: return "magic";
        default: return "none";
    }
}

inline RangedWeaponComponent::AmmoType string_to_ammo_type(const std::string& str) {
    static const std::unordered_map<std::string, RangedWeaponComponent::AmmoType> map = {
        {"none", RangedWeaponComponent::AmmoType::NONE},
        {"arrow", RangedWeaponComponent::AmmoType::ARROW},
        {"bolt", RangedWeaponComponent::AmmoType::BOLT},
        {"stone", RangedWeaponComponent::AmmoType::STONE},
        {"bullet", RangedWeaponComponent::AmmoType::BULLET},
        {"magic", RangedWeaponComponent::AmmoType::MAGIC}
    };
    auto it = map.find(str);
    return (it != map.end()) ? it->second : RangedWeaponComponent::AmmoType::NONE;
}

// ==================== FactionId ====================

inline std::string faction_to_string(FactionId id) {
    switch (id) {
        case FactionId::NONE: return "none";
        case FactionId::PLAYER: return "player";
        case FactionId::VILLAGER: return "villager";
        case FactionId::GUARD: return "guard";
        case FactionId::WILDLIFE: return "wildlife";
        case FactionId::PREDATOR: return "predator";
        case FactionId::GOBLIN: return "goblin";
        case FactionId::UNDEAD: return "undead";
        case FactionId::BANDIT: return "bandit";
        case FactionId::DEMON: return "demon";
        case FactionId::DRAGON: return "dragon";
        case FactionId::SPIDER: return "spider";
        case FactionId::ORC: return "orc";
        case FactionId::ELEMENTAL: return "elemental";
        case FactionId::BEAST: return "beast";
        default: return "none";
    }
}

inline FactionId string_to_faction(const std::string& str) {
    static const std::unordered_map<std::string, FactionId> map = {
        {"none", FactionId::NONE},
        {"player", FactionId::PLAYER},
        {"villager", FactionId::VILLAGER},
        {"guard", FactionId::GUARD},
        {"wildlife", FactionId::WILDLIFE},
        {"predator", FactionId::PREDATOR},
        {"goblin", FactionId::GOBLIN},
        {"undead", FactionId::UNDEAD},
        {"bandit", FactionId::BANDIT},
        {"demon", FactionId::DEMON},
        {"dragon", FactionId::DRAGON},
        {"spider", FactionId::SPIDER},
        {"orc", FactionId::ORC},
        {"elemental", FactionId::ELEMENTAL},
        {"beast", FactionId::BEAST}
    };
    auto it = map.find(str);
    return (it != map.end()) ? it->second : FactionId::NONE;
}

// ==================== ItemEffectComponent::EffectType ====================

inline std::string effect_type_to_string(ItemEffectComponent::EffectType type) {
    switch (type) {
        case ItemEffectComponent::EffectType::HEAL_HP: return "heal_hp";
        case ItemEffectComponent::EffectType::RESTORE_HP_PERCENT: return "restore_hp_percent";
        case ItemEffectComponent::EffectType::DAMAGE_HP: return "damage_hp";
        case ItemEffectComponent::EffectType::BUFF_ATTACK: return "buff_attack";
        case ItemEffectComponent::EffectType::BUFF_DEFENSE: return "buff_defense";
        case ItemEffectComponent::EffectType::BUFF_ARMOR: return "buff_armor";
        case ItemEffectComponent::EffectType::DEBUFF_ATTACK: return "debuff_attack";
        case ItemEffectComponent::EffectType::DEBUFF_DEFENSE: return "debuff_defense";
        case ItemEffectComponent::EffectType::GAIN_XP: return "gain_xp";
        case ItemEffectComponent::EffectType::TELEPORT: return "teleport";
        case ItemEffectComponent::EffectType::REVEAL_MAP: return "reveal_map";
        case ItemEffectComponent::EffectType::POISON: return "poison";
        case ItemEffectComponent::EffectType::REGENERATION: return "regeneration";
        case ItemEffectComponent::EffectType::CUSTOM: return "custom";
        default: return "custom";
    }
}

inline ItemEffectComponent::EffectType string_to_effect_type(const std::string& str) {
    static const std::unordered_map<std::string, ItemEffectComponent::EffectType> map = {
        {"heal_hp", ItemEffectComponent::EffectType::HEAL_HP},
        {"restore_hp_percent", ItemEffectComponent::EffectType::RESTORE_HP_PERCENT},
        {"damage_hp", ItemEffectComponent::EffectType::DAMAGE_HP},
        {"buff_attack", ItemEffectComponent::EffectType::BUFF_ATTACK},
        {"buff_defense", ItemEffectComponent::EffectType::BUFF_DEFENSE},
        {"buff_armor", ItemEffectComponent::EffectType::BUFF_ARMOR},
        {"debuff_attack", ItemEffectComponent::EffectType::DEBUFF_ATTACK},
        {"debuff_defense", ItemEffectComponent::EffectType::DEBUFF_DEFENSE},
        {"gain_xp", ItemEffectComponent::EffectType::GAIN_XP},
        {"teleport", ItemEffectComponent::EffectType::TELEPORT},
        {"reveal_map", ItemEffectComponent::EffectType::REVEAL_MAP},
        {"poison", ItemEffectComponent::EffectType::POISON},
        {"regeneration", ItemEffectComponent::EffectType::REGENERATION},
        {"custom", ItemEffectComponent::EffectType::CUSTOM}
    };
    auto it = map.find(str);
    return (it != map.end()) ? it->second : ItemEffectComponent::EffectType::CUSTOM;
}

// ==================== ItemEffectComponent::Trigger ====================

inline std::string effect_trigger_to_string(ItemEffectComponent::Trigger trigger) {
    switch (trigger) {
        case ItemEffectComponent::Trigger::ON_USE: return "on_use";
        case ItemEffectComponent::Trigger::ON_EQUIP: return "on_equip";
        case ItemEffectComponent::Trigger::ON_UNEQUIP: return "on_unequip";
        case ItemEffectComponent::Trigger::ON_HIT: return "on_hit";
        case ItemEffectComponent::Trigger::PASSIVE: return "passive";
        default: return "on_use";
    }
}

inline ItemEffectComponent::Trigger string_to_effect_trigger(const std::string& str) {
    static const std::unordered_map<std::string, ItemEffectComponent::Trigger> map = {
        {"on_use", ItemEffectComponent::Trigger::ON_USE},
        {"on_equip", ItemEffectComponent::Trigger::ON_EQUIP},
        {"on_unequip", ItemEffectComponent::Trigger::ON_UNEQUIP},
        {"on_hit", ItemEffectComponent::Trigger::ON_HIT},
        {"passive", ItemEffectComponent::Trigger::PASSIVE}
    };
    auto it = map.find(str);
    return (it != map.end()) ? it->second : ItemEffectComponent::Trigger::ON_USE;
}

// ==================== UtilityActionType ====================

inline std::string utility_action_type_to_string(UtilityActionType type) {
    switch (type) {
        case UtilityActionType::IDLE: return "idle";
        case UtilityActionType::WANDER: return "wander";
        case UtilityActionType::PATROL: return "patrol";
        case UtilityActionType::ATTACK_MELEE: return "attack_melee";
        case UtilityActionType::ATTACK_RANGED: return "attack_ranged";
        case UtilityActionType::CHASE: return "chase";
        case UtilityActionType::FLEE: return "flee";
        case UtilityActionType::RETREAT: return "retreat";
        case UtilityActionType::DEFEND_ALLY: return "defend_ally";
        case UtilityActionType::HEAL_SELF: return "heal_self";
        case UtilityActionType::BUFF_ALLY: return "buff_ally";
        case UtilityActionType::INVESTIGATE: return "investigate";
        case UtilityActionType::GUARD_POSITION: return "guard_position";
        case UtilityActionType::CALL_FOR_HELP: return "call_for_help";
        case UtilityActionType::RETURN_HOME: return "return_home";
        case UtilityActionType::SLEEP: return "sleep";
        case UtilityActionType::LOOT: return "loot";
        case UtilityActionType::USE_ENVIRONMENT: return "use_environment";
        case UtilityActionType::CUSTOM: return "custom";
        default: return "idle";
    }
}

inline UtilityActionType string_to_utility_action_type(const std::string& str) {
    static const std::unordered_map<std::string, UtilityActionType> map = {
        {"idle", UtilityActionType::IDLE},
        {"wander", UtilityActionType::WANDER},
        {"patrol", UtilityActionType::PATROL},
        {"attack_melee", UtilityActionType::ATTACK_MELEE},
        {"attack_ranged", UtilityActionType::ATTACK_RANGED},
        {"chase", UtilityActionType::CHASE},
        {"flee", UtilityActionType::FLEE},
        {"retreat", UtilityActionType::RETREAT},
        {"defend_ally", UtilityActionType::DEFEND_ALLY},
        {"heal_self", UtilityActionType::HEAL_SELF},
        {"buff_ally", UtilityActionType::BUFF_ALLY},
        {"investigate", UtilityActionType::INVESTIGATE},
        {"guard_position", UtilityActionType::GUARD_POSITION},
        {"call_for_help", UtilityActionType::CALL_FOR_HELP},
        {"return_home", UtilityActionType::RETURN_HOME},
        {"sleep", UtilityActionType::SLEEP},
        {"loot", UtilityActionType::LOOT},
        {"use_environment", UtilityActionType::USE_ENVIRONMENT},
        {"custom", UtilityActionType::CUSTOM}
    };
    auto it = map.find(str);
    return (it != map.end()) ? it->second : UtilityActionType::IDLE;
}

// ==================== CurveType ====================

inline std::string curve_type_to_string(CurveType type) {
    switch (type) {
        case CurveType::LINEAR: return "linear";
        case CurveType::QUADRATIC: return "quadratic";
        case CurveType::LOGISTIC: return "logistic";
        case CurveType::LOGIT: return "logit";
        case CurveType::EXPONENTIAL: return "exponential";
        case CurveType::STEP: return "step";
        case CurveType::INVERSE: return "inverse";
        case CurveType::SINE: return "sine";
        case CurveType::PARABOLIC: return "parabolic";
        default: return "linear";
    }
}

inline CurveType string_to_curve_type(const std::string& str) {
    static const std::unordered_map<std::string, CurveType> map = {
        {"linear", CurveType::LINEAR},
        {"quadratic", CurveType::QUADRATIC},
        {"logistic", CurveType::LOGISTIC},
        {"logit", CurveType::LOGIT},
        {"exponential", CurveType::EXPONENTIAL},
        {"step", CurveType::STEP},
        {"inverse", CurveType::INVERSE},
        {"sine", CurveType::SINE},
        {"parabolic", CurveType::PARABOLIC}
    };
    auto it = map.find(str);
    return (it != map.end()) ? it->second : CurveType::LINEAR;
}

// ==================== InputType ====================

inline std::string input_type_to_string(InputType type) {
    switch (type) {
        case InputType::HEALTH_PERCENT: return "health_percent";
        case InputType::HEALTH_ABSOLUTE: return "health_absolute";
        case InputType::LEVEL: return "level";
        case InputType::ATTACK_STAT: return "attack_stat";
        case InputType::DEFENSE_STAT: return "defense_stat";
        case InputType::DISTANCE_TO_TARGET: return "distance_to_target";
        case InputType::TARGET_HEALTH_PERCENT: return "target_health_percent";
        case InputType::TARGET_THREAT_LEVEL: return "target_threat_level";
        case InputType::IS_TARGET_HOSTILE: return "is_target_hostile";
        case InputType::IS_TARGET_ALLY: return "is_target_ally";
        case InputType::NEARBY_ENEMIES_COUNT: return "nearby_enemies_count";
        case InputType::NEARBY_ALLIES_COUNT: return "nearby_allies_count";
        case InputType::ALLIES_UNDER_ATTACK: return "allies_under_attack";
        case InputType::IN_COMBAT: return "in_combat";
        case InputType::TIME_SINCE_LAST_ATTACK: return "time_since_last_attack";
        case InputType::HAS_TARGET: return "has_target";
        case InputType::TARGET_IN_MELEE_RANGE: return "target_in_melee_range";
        case InputType::HAS_HEALING_ITEM: return "has_healing_item";
        case InputType::HAS_RANGED_WEAPON: return "has_ranged_weapon";
        case InputType::DISTANCE_TO_HOME: return "distance_to_home";
        case InputType::IS_AT_HOME: return "is_at_home";
        case InputType::IS_NIGHT_TIME: return "is_night_time";
        case InputType::CUSTOM: return "custom";
        default: return "health_percent";
    }
}

inline InputType string_to_input_type(const std::string& str) {
    static const std::unordered_map<std::string, InputType> map = {
        {"health_percent", InputType::HEALTH_PERCENT},
        {"health_absolute", InputType::HEALTH_ABSOLUTE},
        {"level", InputType::LEVEL},
        {"attack_stat", InputType::ATTACK_STAT},
        {"defense_stat", InputType::DEFENSE_STAT},
        {"distance_to_target", InputType::DISTANCE_TO_TARGET},
        {"target_health_percent", InputType::TARGET_HEALTH_PERCENT},
        {"target_threat_level", InputType::TARGET_THREAT_LEVEL},
        {"is_target_hostile", InputType::IS_TARGET_HOSTILE},
        {"is_target_ally", InputType::IS_TARGET_ALLY},
        {"nearby_enemies_count", InputType::NEARBY_ENEMIES_COUNT},
        {"nearby_allies_count", InputType::NEARBY_ALLIES_COUNT},
        {"allies_under_attack", InputType::ALLIES_UNDER_ATTACK},
        {"in_combat", InputType::IN_COMBAT},
        {"time_since_last_attack", InputType::TIME_SINCE_LAST_ATTACK},
        {"has_target", InputType::HAS_TARGET},
        {"target_in_melee_range", InputType::TARGET_IN_MELEE_RANGE},
        {"has_healing_item", InputType::HAS_HEALING_ITEM},
        {"has_ranged_weapon", InputType::HAS_RANGED_WEAPON},
        {"distance_to_home", InputType::DISTANCE_TO_HOME},
        {"is_at_home", InputType::IS_AT_HOME},
        {"is_night_time", InputType::IS_NIGHT_TIME},
        {"custom", InputType::CUSTOM}
    };
    auto it = map.find(str);
    return (it != map.end()) ? it->second : InputType::HEALTH_PERCENT;
}

} // namespace EnumConverters
