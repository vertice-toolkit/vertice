#pragma once
#include "core.h"
#include "hooks.h"
#include "esp.h"

// ===
// Example: Using Vertice for a New Game
// ===

/*
USAGE EXAMPLE - How to use Vertice for a new IL2CPP Unity game:

1. FIRST: Set up your offsets file (offsets.h/cpp)
   - Define all game-specific field offsets
   - This makes features easier to write

2. Create your features like this:

--- features.h ---
#pragma once
#include "../vertice/hooks.h"

namespace game {
    // Player toggles
    FEATURE(godmode, VK_F1);
    FEATURE(infinite_stamina, VK_F2);
    FEATURE(speed_hack, VK_F3);
    FEATURE(no_clip, VK_F4);
    
    // Weapon toggles  
    FEATURE(infinite_ammo, VK_F5);
    FEATURE(no_recoil, 0);
    FEATURE(auto_aim, VK_F6);
    
    // ESP toggles
    ESP_FEATURE(esp_players, true);
    ESP_FEATURE(esp_items, false);
    ESP_FEATURE(esp_skeletons, false);
    ESP_FEATURE(esp_distance, false);
    
    // Config values
    static float speed_multiplier = 3.0f;
    static float aim_fov = 30.0f;
    
    // Functions
    void init();
    void update();
}

--- features.cpp ---
#include "features.h"

namespace game {
    
    // ===
    // Hooks
    // ===
    
    // God Mode - hook TakeDamage
    using TakeDamage_t = bool(*)(void* health, float amount, void* attacker, float minRegen);
    static TakeDamage_t orig_TakeDamage = nullptr;
    
    bool TakeDamage_hook(void* health, float amount, void* attacker, float minRegen) {
        if (godmode) amount = 0.0f;
        return orig_TakeDamage(health, amount, attacker, minRegen);
    }
    
    // Speed Hack - hook GetMoveSpeed
    using GetSpeed_t = float(*)(void* controller);
    static GetSpeed_t orig_GetSpeed = nullptr;
    
    float GetSpeed_hook(void* controller) {
        float base = orig_GetSpeed(controller);
        if (speed_hack) return base * speed_multiplier;
        return base;
    }
    
    // ===
    // Per-frame Updates
    // ===
    
    void update_infinite_stamina() {
        if (!infinite_stamina) return;
        
        auto stamina = Singleton<PlayerStamina>::get_local();
        if (!stamina) return;
        
        auto offset = OFFSET(PlayerStamina, currentStamina);
        write_field<float>(stamina, offset, 100.0f); // Max stamina
    }
    
    void update_no_clip() {
        static bool was_enabled = false;
        
        if (no_clip && !was_enabled) {
            // Enable fly cam
            auto fc = Singleton<PlayerFlyCamera>::get();
            if (fc) call_method(fc, "SetActive", true);
        }
        else if (!no_clip && was_enabled) {
            // Disable fly cam
            auto fc = Singleton<PlayerFlyCamera>::get();
            if (fc) call_method(fc, "SetActive", false);
        }
        
        was_enabled = no_clip;
    }
    
    void update_auto_aim() {
        if (!auto_aim) return;
        
        auto camera = get_main_camera();
        auto players = Objects::find_all<PlayerModeManager>();
        
        // Find nearest enemy
        float nearest_dist = aim_fov;
        void* target = nullptr;
        Vec3 target_pos;
        
        for (auto pmm : players) {
            if (pmm == get_local_player()) continue;
            if (pmm->isAgent == get_local_player()->isAgent) continue; // Same team
            
            Vec3 pos = get_position(pmm);
            float dist = get_position_local().distance(pos);
            
            if (dist < nearest_dist) {
                nearest_dist = dist;
                target = pmm;
                target_pos = pos;
            }
        }
        
        if (target) {
            // Set camera look at target
            set_camera_look_at(target_pos);
        }
    }
    
    // ===
    // Initialization
    // ===
    
    void init() {
        printf("[Game] Initializing features...\n");
        
        // Install hooks
        auto takeDamage = FIND_METHOD(PlayerHealth, TakeDamage, 3);
        if (takeDamage) Hook::create(takeDamage, TakeDamage_hook, &orig_TakeDamage);
        
        auto getSpeed = FIND_METHOD(PlayerController, GetMoveSpeed, 0);
        if (getSpeed) Hook::create(getSpeed, GetSpeed_hook, &orig_GetSpeed);
        
        // Register updates
        UPDATE(update_infinite_stamina());
        UPDATE(update_no_clip());
        UPDATE(update_auto_aim());
        
        printf("[Game] Features initialized\n");
    }
    
    void update() {
        FeatureManager::update();
    }
}

3. In your main.cpp/dllmain.cpp:

#include "features.h"

void main_loop() {
    while (g_running) {
        game::update();
        check_keys();
        Sleep(16); ~60fps
    }
}

4. Build and inject!

*/

// ===
// Vertice Usage Example
// ===

/*
QUICK START - Copy this pattern for new games:

// === offsets.h ===
#pragma once
#include "../vertice/core.h"

struct GameOffsets {
    // PlayerHealth
    uint32_t PlayerHealth_currentHealth = UINT32_MAX;
    uint32_t PlayerHealth_maxHealth = UINT32_MAX;
    
    // PlayerStamina  
    uint32_t PlayerStamina_currentStamina = UINT32_MAX;
    uint32_t PlayerStamina_maxStamina = UINT32_MAX;
    
    // PlayerController
    uint32_t PlayerController_speed = UINT32_MAX;
    
    // Weapon
    uint32_t Weapon_ammo = UINT32_MAX;
    uint32_t Weapon_cooldown = UINT32_MAX;
};

extern GameOffsets g_offsets;

// === offsets.cpp ===
#include "offsets.h"

GameOffsets g_offsets;

void resolve_offsets() {
    g_offsets.PlayerHealth_currentHealth = OFFSET(PlayerHealth, currentHealth);
    g_offsets.PlayerHealth_maxHealth = OFFSET(PlayerHealth, maxHealth);
    // ... etc
}

// === features.h ===
#pragma once
#include "../vertice/hooks.h"

FEATURE(godmode, VK_F1);
FEATURE(speed_hack, VK_F2);
FEATURE(infinite_ammo, VK_F3);

void init_features();
void update_features();

// === features.cpp ===
#include "features.h"

HOOK(takedamage, PlayerHealth, TakeDamage, 4);

void init_features() {
    // Hooks
    Hook::create(FIND(takedamage), takedamage_hook, &takedamage_orig);
    
    // Updates
    UPDATE(if (godmode) { /* custom logic */ });
}

void update_features() {
    if (speed_hack) { /* custom logic */ }
}

*/