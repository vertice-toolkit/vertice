#pragma once
#include <cstdint>
#include <cmath>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

#ifdef _WIN32
#include <Windows.h>
#endif

// ===
// Runtime Detection
// ===
enum class Runtime : uint8_t { Unknown, IL2CPP, Mono };

// ===
// IL2CPP Forward Declarations
// ===
struct Il2CppClass;
struct FieldInfo {
    const char* name;
    void* type;
    Il2CppClass* parent;
    int32_t offset;
    uint8_t flags;
};
struct Il2CppObject;
struct Il2CppString;
struct Il2CppException;
struct Il2CppAssembly;
struct Il2CppDomain;
struct Il2CppImage {
    const char* name;
    void* assembly;
    int typeStart;
    int typeCount;
    int exportedTypeStart;
    int exportedTypeCount;
    int customAttributeStart;
    int customAttributeCount;
};
struct Il2CppField;
struct Il2CppDateTime;
struct Il2CppThread;
struct Il2CppAssemblyName;
typedef wchar_t Il2CppChar;

// MethodInfo needs methodPointer at offset 0
struct MethodInfo {
    void* methodPointer;
    void* invoker_method;
    const char* name;
    Il2CppClass* klass;
    const void* return_type;
    const void* parameters;
    int parameters_count;
    // ... more fields
};

// ===
// Mono Forward Declarations (opaque — all access via getter functions)
// ===
struct MonoClass;
struct MonoMethod;
struct MonoObject;
struct MonoString;
struct MonoException;
struct MonoAssembly;
struct MonoImage;
struct MonoClassField;
struct MonoDomain;
struct MonoThread;
typedef uint16_t MonoChar;

// ===
// Translate API — map obfuscated names to readable names
// ===
using translations = std::unordered_map<std::string, std::string>;
std::string translate(const translations& map, const std::string& value);

extern translations tClasses;
extern translations tFields;
extern translations tMethods;

namespace vertice {

// ===
// Core IL2CPP API
// ===

struct Core {
    static bool initialized;
    static Runtime current_runtime;

    // IL2CPP image references
    static Il2CppImage* assembly_csharp;
    static Il2CppImage* unity_engine;
    static Il2CppImage* assembly_mscorlib;

    // Mono image references
    static MonoImage* mono_assembly_csharp;
    static MonoImage* mono_unity_engine;
    static MonoImage* mono_mscorlib;

    static void init();
    static void shutdown();
    static const char* runtime_name();

    // Class resolution
    static Il2CppClass* find_class(const char* namespaze, const char* name);
    static Il2CppClass* find_class_unsafe(const char* namespaze, const char* name);

    // Method resolution  
    static MethodInfo* find_method(Il2CppClass* klass, const char* name, int args);
    static MethodInfo* find_method(const char* namespaze, const char* klass, const char* name, int args);
    static void* find_method_ptr(const char* namespaze, const char* klass, const char* name, int args);

    // Field resolution
    static FieldInfo* find_field(Il2CppClass* klass, const char* name);
    static FieldInfo* find_field(const char* namespaze, const char* klass, const char* name);
    static uint32_t get_field_offset(const char* namespaze, const char* klass, const char* field);
    static uint32_t get_static_field_offset(const char* namespaze, const char* klass, const char* field);
    static void* get_static_field_ptr(const char* namespaze, const char* klass, const char* field);
    static void field_static_get_value(FieldInfo* field, void* value);

    // Assembly / Image enumeration
    static int get_assembly_count();
    static Il2CppImage* get_assembly_image(int index);

    // Class enumeration
    static int get_image_class_count(Il2CppImage* img);
    static Il2CppClass* get_image_class(Il2CppImage* img, int index);

    // Field / Method enumeration (iterator pattern: call with iter=nullptr first, then reuse)
    static FieldInfo* get_class_next_field(Il2CppClass* klass, void** iter);
    static MethodInfo* get_class_next_method(Il2CppClass* klass, void** iter);

    // Field introspection
    static void* read_field_raw(Il2CppObject* obj, int32_t offset);
    static bool field_is_static(FieldInfo* field);

    // String operations
    static Il2CppString* string_new(const char* str);
    static Il2CppChar* string_get_chars(Il2CppString* str);
    static Il2CppString* string_concat(Il2CppString* a, Il2CppString* b);

    // Runtime invoke
    static Il2CppObject* runtime_invoke(MethodInfo* method, Il2CppObject* obj, void** parameters, Il2CppException** exc);
    static Il2CppObject* runtime_invoke(const char* namespaze, const char* klass, const char* method, int args, Il2CppObject* obj, void** parameters);

    // Field R/W
    template<typename T>
    static T read_field(Il2CppObject* obj, uint32_t offset) {
        if (!obj || offset == UINT32_MAX) return T();
        return *(T*)((uintptr_t)obj + offset);
    }

    template<typename T>
    static void write_field(Il2CppObject* obj, uint32_t offset, T value) {
        if (!obj || offset == UINT32_MAX) return;
        *(T*)((uintptr_t)obj + offset) = value;
    }

    // Static field R/W
    template<typename T>
    static T read_static_field(Il2CppClass* klass, const char* field) {
        void* ptr = get_static_field_ptr("", klass->namespaze, field);
        if (!ptr) return T();
        return *(T*)ptr;
    }

    template<typename T>
    static void write_static_field(Il2CppClass* klass, const char* field, T value) {
        void* ptr = get_static_field_ptr("", klass->namespaze, field);
        if (!ptr) return;
        *(T*)ptr = value;
    }

    // Component helpers
    static Il2CppObject* get_component(Il2CppObject* go, const char* component_name);
    static Il2CppObject* get_game_object(Il2CppObject* component);
    static Il2CppObject* get_transform(Il2CppObject* go);
    static Il2CppObject* get_main_camera();
    
    // Transform operations
    static Il2CppObject* transform_get_position(Il2CppObject* transform);
    static void transform_set_position(Il2CppObject* transform, float x, float y, float z);
    static Il2CppObject* transform_get_forward(Il2CppObject* transform);
    static Il2CppObject* transform_get_right(Il2CppObject* transform);
    static Il2CppObject* transform_get_up(Il2CppObject* transform);

    // Vector3 helpers
    struct Vec3 {
        float x, y, z;
        Vec3() : x(0), y(0), z(0) {}
        Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
        float length() const { return sqrtf(x*x + y*y + z*z); }
        float distance(const Vec3& other) const {
            float dx = x - other.x, dy = y - other.y, dz = z - other.z;
            return sqrtf(dx*dx + dy*dy + dz*dz);
        }
        Vec3 normalized() const {
            float len = length();
            if (len < 0.0001f) return Vec3();
            return Vec3(x/len, y/len, z/len);
        }
    };

    static Vec3 get_position(Il2CppObject* obj);
    static Vec3 world_to_screen(Il2CppObject* camera, Vec3 world_pos);
};

// Runtime-conditional helpers (use vertice::Core:: directly)
#define IF_IL2CPP(...) \
    if (vertice::Core::current_runtime == vertice::Runtime::IL2CPP) { __VA_ARGS__; }
#define IF_MONO(...) \
    if (vertice::Core::current_runtime == vertice::Runtime::Mono) { __VA_ARGS__; }

} // namespace vertice