#pragma once
#include "core.h"
#include <cstdint>
#include <functional>
#include <MinHook.h>

namespace vertice {

typedef Core::Vec3 Vec3;

// ===
// MinHook Wrapper
// ===

struct Hook {
    static bool init() {
        if (MH_Initialize() != MH_OK) {
            printf("[Hook] MinHook init failed\n");
            return false;
        }
        printf("[Hook] MinHook initialized\n");
        return true;
    }

    static void shutdown() {
        MH_Uninitialize();
    }

    template<typename T>
    static bool create(T target, T detour, T* original) {
        if (MH_CreateHook(target, detour, (void**)original) != MH_OK) {
            printf("[Hook] CreateHook failed for %p\n", target);
            return false;
        }
        if (MH_EnableHook(target) != MH_OK) {
            printf("[Hook] EnableHook failed for %p\n", target);
            return false;
        }
        printf("[Hook] Hook installed: %p -> %p\n", target, detour);
        return true;
    }

    template<typename T>
    static bool enable(T target) {
        return MH_EnableHook(target) == MH_OK;
    }

    template<typename T>
    static bool disable(T target) {
        return MH_DisableHook(target) == MH_OK;
    }

    static void disable_all() {
        MH_DisableHook(MH_ALL_HOOKS);
    }
};

// ===
// Feature Toggle System
// ===

struct Toggle {
    const char* name;
    bool* value;
    int key;
    
    Toggle(const char* n, bool* v, int k = 0) : name(n), value(v), key(k) {}
};

struct FeatureManager {
    static std::vector<Toggle> toggles;
    static std::vector<std::function<void()>> updates;
    static std::vector<std::function<void()>> on_enable;
    static std::vector<std::function<void()>> on_disable;
    static bool menu_open;
    
    static void register_toggle(const char* name, bool* value, int key = 0) {
        toggles.emplace_back(name, value, key);
    }
    
    static void register_update(std::function<void()> fn) {
        updates.push_back(fn);
    }
    
    static void register_on_enable(std::function<void()> fn) {
        on_enable.push_back(fn);
    }
    
    static void register_on_disable(std::function<void()> fn) {
        on_disable.push_back(fn);
    }
    
    static void update() {
        for (auto& fn : updates) {
            __try {
                fn();
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                printf("[Feature] Exception in update\n");
            }
        }
    }
    
    static void check_keys() {
        for (auto& t : toggles) {
            if (t.key && t.value) {
                bool down = (GetAsyncKeyState(t.key) & 0x8000) != 0;
                static bool prev[256] = {};
                if (down && !prev[t.key]) {
                    *t.value = !*t.value;
                    printf("[Feature] Toggle %s: %s\n", t.name, *t.value ? "ON" : "OFF");
                }
                prev[t.key] = down ? 1 : 0;
            }
        }
    }
    
    static void on_feature_enable() {
        for (auto& fn : on_enable) fn();
    }
    
    static void on_feature_disable() {
        for (auto& fn : on_disable) fn();
    }
};

// ===
// Cmd/Network Call Helper
// ===

struct Network {
    struct CmdCache {
        char key[128];
        void* ptr;
    };
    static CmdCache cache[32];
    static int cache_count;
    
    static void* find_cmd(const char* namespaze, const char* klass, const char* method, int args) {
        char key[128];
        sprintf_s(key, "%s.%s.%s.%d", namespaze, klass, method, args);
        
        for (int i = 0; i < cache_count; i++) {
            if (strcmp(cache[i].key, key) == 0) return cache[i].ptr;
        }
        
        void* ptr = IL2CPP::find_method_ptr(namespaze, klass, method, args);
        if (!ptr) {
            // Try with -1 args (wildcard)
            ptr = IL2CPP::find_method_ptr(namespaze, klass, method, -1);
        }
        
        if (ptr && cache_count < 32) {
            strncpy_s(cache[cache_count].key, key, _TRUNCATE);
            cache[cache_count].ptr = ptr;
            cache_count++;
        }
        
        return ptr;
    }
    
    template<typename T>
    static T get_cmd(const char* namespaze, const char* klass, const char* method, int args) {
        return (T)find_cmd(namespaze, klass, method, args);
    }
    
    static void call(void* instance, const char* namespaze, const char* klass, const char* method, int args, void** params = nullptr) {
        void* ptr = find_cmd(namespaze, klass, method, args);
        if (!ptr) {
            printf("[Network] Cmd not found: %s.%s.%s\n", namespaze, klass, method);
            return;
        }
        
        __try {
            switch (args) {
                case 0: {
                    using Fn = void(*)(void*);
                    ((Fn)ptr)(instance);
                    break;
                }
                case 1: {
                    using Fn = void(*)(void*, void*);
                    ((Fn)ptr)(instance, params[0]);
                    break;
                }
                case 2: {
                    using Fn = void(*)(void*, void*, void*);
                    ((Fn)ptr)(instance, params[0], params[1]);
                    break;
                }
                case 3: {
                    using Fn = void(*)(void*, void*, void*, void*);
                    ((Fn)ptr)(instance, params[0], params[1], params[2]);
                    break;
                }
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            printf("[Network] Exception calling %s.%s\n", klass, method);
        }
    }
};

// ===
// Singleton Helpers
// ===

template<typename T>
struct Singleton {
    static T* get(const char* class_name) {
        Il2CppClass* klass = IL2CPP::find_class("", class_name);
        if (!klass) return nullptr;
        
        FieldInfo* field = IL2CPP::find_field(klass, "Instance");
        if (!field) {
            char buf[128];
            sprintf_s(buf, "<Instance>k__BackingField");
            field = IL2CPP::find_field(klass, buf);
        }
        if (!field) return nullptr;
        
        T* instance = nullptr;
        IL2CPP::field_static_get_value(field, &instance);
        return instance;
    }
    
    static T* get_local(const char* class_name) {
        Il2CppClass* klass = IL2CPP::find_class("", class_name);
        if (!klass) return nullptr;
        
        FieldInfo* field = IL2CPP::find_field(klass, "LocalInstance");
        if (!field) {
            char buf[128];
            sprintf_s(buf, "<LocalInstance>k__BackingField");
            field = IL2CPP::find_field(klass, buf);
        }
        if (!field) return nullptr;
        
        T* instance = nullptr;
        IL2CPP::field_static_get_value(field, &instance);
        return instance;
    }
};

// ===
// Object Enumeration Helpers
// ===

struct Objects {
    template<typename T>
    static std::vector<T*> find_all(const char* class_name) {
        std::vector<T*> result;
        
        Il2CppClass* target_class = IL2CPP::find_class("", class_name);
        if (!target_class) return result;
        
        Il2CppClass* obj_class = IL2CPP::find_class("UnityEngine", "Object");
        if (!obj_class) return result;
        
        MethodInfo* find_method = IL2CPP::find_method(obj_class, "FindObjectsOfType", 1);
        if (!find_method) find_method = IL2CPP::find_method(obj_class, "FindObjectsOfType", 0);
        if (!find_method) return result;
        
        void* type_params[] = { &target_class };
        Il2CppException* exc = nullptr;
        Il2CppObject* arr = IL2CPP::runtime_invoke(find_method, nullptr, type_params, &exc);
        if (exc || !arr) return result;
        
        int count = 0;
        __try { count = *(int*)((uintptr_t)arr + 0x18); } __except (EXCEPTION_EXECUTE_HANDLER) { return result; }
        if (count > 200) count = 200;
        
        for (int i = 0; i < count; i++) {
            T* obj = nullptr;
            __try { obj = *(T**)((uintptr_t)arr + 0x20 + i * 8); } __except (EXCEPTION_EXECUTE_HANDLER) { break; }
            if (obj) result.push_back(obj);
        }
        
        return result;
    }
    
    template<typename T>
    static T* find_nearest(Vec3 from, float max_dist = 9999.0f) {
        auto all = find_all<T>();
        T* nearest = nullptr;
        float nearest_dist = max_dist;
        
        for (auto obj : all) {
            Vec3 pos = IL2CPP::get_position(obj);
            float dist = from.distance(pos);
            if (dist < nearest_dist) {
                nearest_dist = dist;
                nearest = obj;
            }
        }
        
        return nearest;
    }
};

} // namespace vertice