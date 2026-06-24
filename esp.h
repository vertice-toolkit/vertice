#pragma once
#include "core.h"
#include <vector>
#include <imgui.h>

namespace vertice {

// ===
// ESP System
// ===

struct Color {
    static const ImU32 RED = IM_COL32(255, 50, 50, 255);
    static const ImU32 BLUE = IM_COL32(0, 150, 255, 255);
    static const ImU32 GREEN = IM_COL32(0, 255, 0, 255);
    static const ImU32 YELLOW = IM_COL32(255, 255, 0, 255);
    static const ImU32 WHITE = IM_COL32(255, 255, 255, 255);
    static const ImU32 ORANGE = IM_COL32(255, 165, 0, 255);
    static const ImU32 PURPLE = IM_COL32(180, 50, 255, 255);
    static const ImU32 CYAN = IM_COL32(0, 255, 255, 255);
};

struct ESP {
    static bool enabled;
    static bool show_players;
    static bool show_items;
    static bool show_npcs;
    static bool show_containers;
    
    static ImU32 player_color_ally;
    static ImU32 player_color_enemy;
    static ImU32 item_color;
    static ImU32 npc_color;
    
    struct PlayerInfo {
        void* pmm;
        void* go;
        Vec3 position;
        bool is_local;
        bool is_agent;
        float health;
        float max_health;
        const char* name;
    };
    
    static std::vector<PlayerInfo> get_players() {
        std::vector<PlayerInfo> result;
        
        auto pmm_class = IL2CPP::find_class("", "PlayerModeManager");
        if (!pmm_class) return result;
        
        auto field = IL2CPP::find_field(pmm_class, "Instances");
        if (!field) return result;
        
        void* list = nullptr;
        IL2CPP::field_static_get_value(field, &list);
        if (!list) return result;
        
        int size = 0;
        __try { size = *(int*)((uintptr_t)list + 0x18); } __except (EXCEPTION_EXECUTE_HANDLER) { return result; }
        void* arr = nullptr;
        __try { arr = *(void**)((uintptr_t)list + 0x10); } __except (EXCEPTION_EXECUTE_HANDLER) { return result; }
        if (!arr) return result;
        
        auto local_pmm = Singleton<PlayerModeManager>::get_local("PlayerModeManager");
        
        for (int i = 0; i < size; i++) {
            void* pmm = nullptr;
            __try { pmm = *(void**)((uintptr_t)arr + 0x20 + i * 8); } __except (EXCEPTION_EXECUTE_HANDLER) { break; }
            if (!pmm) continue;
            
            PlayerInfo info;
            info.pmm = pmm;
            info.go = IL2CPP::get_game_object(pmm);
            if (!info.go) continue;
            
            info.is_local = (pmm == local_pmm);
            
            auto is_agent_offset = IL2CPP::get_field_offset("", "PlayerModeManager", "isAgent");
            info.is_agent = IL2CPP::read_field<bool>(pmm, is_agent_offset);
            
            info.position = IL2CPP::get_position(info.go);
            
            result.push_back(info);
        }
        
        return result;
    }
    
    static void render_player(ImDrawList* dl, PlayerInfo& info, Vec3 screen) {
        if (screen.z < 0.01f) return;
        
        ImU32 color = info.is_agent ? player_color_ally : player_color_enemy;
        
        float box_h = 60.0f;
        float box_w = 30.0f;
        
        ImVec2 s_pos(screen.x, screen.y);
        ImVec2 box_min(s_pos.x - box_w/2, s_pos.y - box_h);
        ImVec2 box_max(s_pos.x + box_w/2, s_pos.y);
        
        dl->AddRect(box_min, box_max, color, 0, 0, 2.0f);
        
        const char* label = info.is_agent ? "Agent" : "Smuggler";
        if (info.is_local) label = "YOU";
        
        dl->AddText(ImVec2(s_pos.x - 20, s_pos.y - box_h - 14), color, label);
    }
    
    static void render_snapline(ImDrawList* dl, Vec3 screen) {
        if (screen.z < 0.01f) return;
        
        auto io = ImGui::GetIO();
        ImVec2 center(io.DisplaySize.x * 0.5f, io.DisplaySize.y);
        dl->AddLine(center, ImVec2(screen.x, screen.y), Color::WHITE, 1.5f);
    }
    
    static void render() {
        if (!enabled || !show_players) return;
        
        auto camera = IL2CPP::get_main_camera();
        if (!camera) return;
        
        auto players = get_players();
        auto dl = ImGui::GetBackgroundDrawList();
        
        for (auto& info : players) {
            auto screen = IL2CPP::world_to_screen(camera, info.position);
            render_player(dl, info, screen);
        }
    }
    
    static void render_hud() {
        if (!enabled) return;
        
        auto dl = ImGui::GetBackgroundDrawList();
        float y = 50.0f;
        
        dl->AddText(ImVec2(10, y), IM_COL32(255, 255, 255, 255), "[Vertice ESP]"); y += 20;
        
        if (show_players) {
            dl->AddText(ImVec2(10, y), IM_COL32(0, 255, 0, 255), "+ Players"); y += 16;
        }
    }
};

// ===
// Menu System
// ===

struct Menu {
    static bool open;
    static bool toggle_key;
    
    static void toggle() {
        open = !open;
    }
    
    static void render() {
        if (!open) return;
        
        ImGui::SetNextWindowSize(ImVec2(400, 500));
        ImGui::Begin("Vertice", &open, ImGuiWindowFlags_NoCollapse);
        
        if (ImGui::CollapsingHeader("ESP", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Enable", &ESP::enabled);
            ImGui::Checkbox("Show Players", &ESP::show_players);
            ImGui::Checkbox("Show Items", &ESP::show_items);
            ImGui::Checkbox("Show NPCs", &ESP::show_npcs);
            ImGui::Checkbox("Show Containers", &ESP::show_containers);
        }
        
        if (ImGui::CollapsingHeader("Player", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Player cheats go here");
        }
        
        if (ImGui::CollapsingHeader("Visuals", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Visual cheats go here");
        }
        
        ImGui::Separator();
        ImGui::Text("Press INS to toggle menu");
        
        ImGui::End();
    }
};

} // namespace vertice