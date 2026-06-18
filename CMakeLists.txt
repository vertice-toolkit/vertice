# ============================================
# Vertice CMake
# Add this to your project's CMakeLists.txt
# ============================================

# Vertice source files
set(VERTICE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/vertice/core.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/vertice/hooks.cpp
)

# Vertice headers
set(VERTICE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/vertice/core.h
    ${CMAKE_CURRENT_SOURCE_DIR}/vertice/hooks.h
    ${CMAKE_CURRENT_SOURCE_DIR}/vertice/esp.h
    ${CMAKE_CURRENT_SOURCE_DIR}/vertice/example.h
    ${CMAKE_CURRENT_SOURCE_DIR}/vertice/vertice.h
)

# Add to your target
# add_library(your_cheat SHARED ${YOUR_SOURCES} ${VERTICE_SOURCES} ${VERTICE_HEADERS})

# ============================================
# Integration Guide
# ============================================
#
# 1. COPY VERTICE TO YOUR PROJECT
#    Copy the vertice/ folder to your project's src/ directory
#
# 2. INCLUDE IN CMAKE
#    Add the following to your CMakeLists.txt:
#
#    include_directories(${CMAKE_SOURCE_DIR}/src/vertice)
#    add_library(your_cheat SHARED your_main.cpp 
#        src/vertice/core.cpp 
#        src/vertice/hooks.cpp
#        src/vertice/vertice.cpp)
#
# 3. CREATE GAME OFFSETS
#    Create offsets.h/cpp in your game folder:
#
#    // game_offsets.h
#    struct GameOffsets {
#        uint32_t PlayerHealth_currentHealth = UINT32_MAX;
#        uint32_t PlayerHealth_maxHealth = UINT32_MAX;
#        // ... add more as needed
#    };
#    extern GameOffsets g_offsets;
#
#    // game_offsets.cpp
#    #include "game_offsets.h"
#    
#    bool resolve_all() {
#        g_offsets.PlayerHealth_currentHealth = GET_OFFSET(PlayerHealth, currentHealth);
#        g_offsets.PlayerHealth_maxHealth = GET_OFFSET(PlayerHealth, maxHealth);
#        // ... continue for all offsets
#        return true;
#    }
#
# 4. DEFINE FEATURES
#    Use the vertice macros:
#
#    // game_features.h
#    #pragma once
#    #include "vertice/vertice.h"
#    
#    TOGGLE(godmode, VK_F1);
#    TOGGLE(speed_hack, VK_F2);
#    TOGGLE(esp_enabled, VK_INSERT);
#    CFG_FLOAT(speed_multiplier, 3.0f);
#
#    void init_game_features();
#    void update_game_features();
#
#    // game_features.cpp
#    #include "game_features.h"
#    #include "game_offsets.h"
#    
#    MAKE_HOOK(take_damage, bool, PlayerHealth, TakeDamage, 3, void* _this, float amount, void* attacker, float delay) {
#        if (godmode) amount = 0.0f;
#        return take_damage_orig(_this, amount, attacker, delay);
#    }
#    
#    void init_game_features() {
#        INSTALL_HOOK(take_damage, PlayerHealth, TakeDamage, 3);
#    }
#    
#    void update_game_features() {
#        if (speed_hack) {
#            // Per-frame logic for speed hack
#        }
#    }
#
# 5. INTEGRATE WITH MAIN
#    Include in your dllmain.cpp or main entry:
#
#    #include "game_features.h"
#    
#    void main_loop() {
#        init_game_features();
#        resolve_all();
#        
#        while (g_running) {
#            update_game_features();
#            vertice::FeatureManager::check_keys();
#            Sleep(16);
#        }
#    }
#
# ============================================