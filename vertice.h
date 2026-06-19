#pragma once

// ===
// VERTICE
// Reusable IL2CPP Unity Cheat Framework
// ===
// 
// This framework provides:
// - Core IL2CPP API wrappers
// - MinHook integration
// - Feature toggle system
// - ESP rendering utilities  
// - Network/Command helpers
// - Singleton accessors
//
// USAGE:
// 1. Copy vertice/ folder to your project
// 2. Create game-specific offsets.h/cpp
// 3. Define features using FEATURE() macro
// 4. Implement hooks and updates
// 5. Build and inject!
//
// ===

#include "core.h"
#include "hooks.h"
#include "esp.h"
#include "example.h"

// ===
// Shortcut Macros for Easy Feature Definition
// ===

// Toggle a boolean on/off with key
#define TOGGLE(name, key) \
    static bool name = false; \
    vertice::FeatureManager::register_toggle(#name, &name, key);

// Simple toggle without key
#define FLAG(name) \
    static bool name = false; \
    vertice::FeatureManager::register_toggle(#name, &name, 0);

// Configurable float
#define CFG_FLOAT(name, default_val) \
    static float name = default_val;

// Configurable int  
#define CFG_INT(name, default_val) \
    static int name = default_val;

// Register per-frame update
#define ON_UPDATE(fn) \
    vertice::FeatureManager::register_update(fn);

// Create a MinHook wrapper
#define MAKE_HOOK(name, ret, klass, method, args, ...) \
    using name##_t = ret(*)(__VA_ARGS__); \
    static name##_t name##_orig = nullptr; \
    static ret name##_hook(__VA_ARGS__)

// Find a method and create hook
#define INSTALL_HOOK(name, klass, method, args) \
    do { \
        auto _ptr =             vertice::Core::find_method_ptr(#klass, #method, args); \
        if (_ptr) { \
            vertice::Hook::create(_ptr, name##_hook, &name##_orig); \
        } else { \
            printf("[Hook] WARN: " #klass "." #method " not found\n"); \
        } \
    } while(0)

// Call a network command
#define CALL_CMD(inst, klass, method, ...) \
    vertice::Network::call(inst, "", #klass, #method, sizeof((__VA_ARGS__))/sizeof(void*), (__VA_ARGS__))

// Get singleton instance
#define GET_SINGLETON(type) \
    vertice::Singleton<type>::get(#type)

#define GET_LOCAL(type) \
    vertice::Singleton<type>::get_local(#type)

// Find all objects of type
#define FIND_ALL(type) \
    vertice::Objects::find_all<type>()

// Read field with offset cache
#define READ(obj, type, offset) \
    vertice::Core::read_field<type>(obj, offset)

#define WRITE(obj, type, offset, value) \
    vertice::Core::write_field<type>(obj, offset, value)

// Get offset (call once at init)
#define GET_OFFSET(klass, field) \
    vertice::Core::get_field_offset("", #klass, #field)

// ===
// Version Info
// ===

#define VERTICE_VERSION "1.0.0"
#define VERTICE_NAME "Vertice Framework"

// ===
// End of Vertice
// ===