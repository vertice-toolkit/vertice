#include "core.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <MinHook.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#define FIELD_ATTRIBUTE_STATIC 0x10

namespace vertice {

bool Core::initialized = false;
Il2CppImage* Core::assembly_csharp = nullptr;
Il2CppImage* Core::unity_engine = nullptr;

// ===
// Internal IL2CPP Types (from il2cp.h)
// ===

typedef Il2CppClass* (*il2cpp_class_from_name_t)(Il2CppImage* image, const char* namespaze, const char* name);
typedef Il2CppClass* (*il2cpp_class_get_element_class_t)(Il2CppClass* klass);
typedef const char* (*il2cpp_class_get_name_t)(Il2CppClass* klass);
typedef const char* (*il2cpp_class_get_namespace_t)(Il2CppClass* klass);
typedef Il2CppClass* (*il2cpp_class_get_parent_t)(Il2CppClass* klass);
typedef int (*il2cpp_class_get_flags_t)(Il2CppClass* klass);
typedef bool (*il2cpp_class_is_enum_t)(Il2CppClass* klass);
typedef Il2CppField* (*il2cpp_class_get_fields_t)(Il2CppClass* klass, void** iter);
typedef FieldInfo* (*il2cpp_class_get_field_t)(Il2CppClass* klass, const char* name);
typedef MethodInfo* (*il2cpp_class_get_methods_t)(Il2CppClass* klass, void** iter);
typedef MethodInfo* (*il2cpp_class_get_method_from_name_t)(Il2CppClass* klass, const char* name, int argsCount);
typedef Il2CppClass* (*il2cpp_class_get_nested_types_t)(Il2CppClass* klass, void** iter);
typedef int (*il2cpp_class_instance_size_t)(Il2CppClass* klass);
typedef int (*il2cpp_class_value_size_t)(Il2CppClass* klass, int* align);
typedef void* (*il2cpp_class_static_box_t)(Il2CppClass* klass, void* data);

typedef Il2CppString* (*il2cpp_string_new_t)(const char* str);
typedef Il2CppString* (*il2cpp_string_new_from_length_t)(const char* str, int length);
typedef int (*il2cpp_string_length_t)(Il2CppString* str);
typedef Il2CppChar* (*il2cpp_string_chars_t)(Il2CppString* str);

typedef Il2CppObject* (*il2cpp_value_box_t)(Il2CppClass* klass, void* data);

typedef void (*il2cpp_method_get_name_t)(MethodInfo* method, char* buf);
typedef MethodInfo* (*il2cpp_method_get_declaring_type_t)(MethodInfo* method);
typedef Il2CppClass* (*il2cpp_method_get_return_type_t)(MethodInfo* method);
typedef int (*il2cpp_method_get_param_count_t)(MethodInfo* method);
typedef void (*il2cpp_method_get_params_t)(MethodInfo* method, void** params);

typedef FieldInfo* (*il2cpp_field_get_offset_t)(FieldInfo* field);
typedef const char* (*il2cpp_field_get_name_t)(FieldInfo* field);
typedef int (*il2cpp_field_get_flags_t)(FieldInfo* field);

typedef Il2CppObject* (*il2cpp_runtime_invoke_t)(MethodInfo* method, Il2CppObject* obj, void** params, Il2CppException** exc);

typedef void (*il2cpp_field_static_get_value_t)(FieldInfo* field, void* value);
typedef void (*il2cpp_field_static_set_value_t)(FieldInfo* field, void* value);

typedef void (*il2cpp_gc_foreach_t)(void(*callback)(Il2CppObject*, void*), void* user_data);

typedef void* (*il2cpp_assembly_get_image_t)(Il2CppAssembly* assembly);

typedef uint64_t(*il2cpp_datetime_to_tick_count_t)(Il2CppDateTime* dt);
typedef Il2CppDateTime* (*il2cpp_datetime_from_tick_count_t)(Il2CppDateTime* dt);

typedef void (*il2cpp_thread_attach_t)(Il2CppDomain* domain);
typedef void (*il2cpp_thread_detach_t)(Il2CppThread* thread);

typedef Il2CppDomain* (*il2cpp_domain_get_t)();
typedef Il2CppAssemblyName* (*il2cpp_domain_get_assemblies_t)(size_t* size);
typedef int (*il2cpp_image_get_class_count_t)(Il2CppImage*);
typedef Il2CppClass* (*il2cpp_image_get_class_t)(Il2CppImage*, int);

// Global function pointers
static il2cpp_class_from_name_t f_class_from_name = nullptr;
static il2cpp_class_get_fields_t f_class_get_fields = nullptr;
static il2cpp_class_get_field_t f_class_get_field = nullptr;
static il2cpp_class_get_methods_t f_class_get_methods = nullptr;
static il2cpp_class_get_method_from_name_t f_class_get_method_from_name = nullptr;
static il2cpp_string_new_t f_string_new = nullptr;
static il2cpp_string_length_t f_string_length = nullptr;
static il2cpp_string_chars_t f_string_chars = nullptr;
static il2cpp_runtime_invoke_t f_runtime_invoke = nullptr;
static il2cpp_field_static_get_value_t f_field_static_get_value = nullptr;
static il2cpp_field_static_set_value_t f_field_static_set_value = nullptr;
static il2cpp_value_box_t f_value_box = nullptr;
static il2cpp_assembly_get_image_t f_assembly_get_image = nullptr;
static il2cpp_domain_get_t f_domain_get = nullptr;
static il2cpp_domain_get_assemblies_t f_domain_get_assemblies = nullptr;
static il2cpp_thread_attach_t f_thread_attach = nullptr;
static il2cpp_image_get_class_count_t f_image_get_class_count = nullptr;
static il2cpp_image_get_class_t f_image_get_class = nullptr;

// ===
// Helper Functions
// ===

static void add_offset(uint32_t base, const char* name, uint32_t offset) {
    // Store in global offset table
}

struct ExportEntry {
    const char* name;
    void** ptr;
};

static bool scan_for_il2cpp() {
    HMODULE module = GetModuleHandleA("GameAssembly.dll");
    if (!module) {
        module = GetModuleHandleA("unityplayer.dll");
    }
    if (!module) {
        printf("[Core] Could not find GameAssembly.dll or unityplayer.dll\n");
        return false;
    }

    ExportEntry exports[] = {
        { "il2cpp_class_from_name",             (void**)&f_class_from_name },
        { "il2cpp_class_get_fields",            (void**)&f_class_get_fields },
        { "il2cpp_class_get_field",             (void**)&f_class_get_field },
        { "il2cpp_class_get_methods",           (void**)&f_class_get_methods },
        { "il2cpp_class_get_method_from_name",  (void**)&f_class_get_method_from_name },
        { "il2cpp_string_new",                  (void**)&f_string_new },
        { "il2cpp_string_length",               (void**)&f_string_length },
        { "il2cpp_string_chars",                (void**)&f_string_chars },
        { "il2cpp_runtime_invoke",              (void**)&f_runtime_invoke },
        { "il2cpp_field_static_get_value",      (void**)&f_field_static_get_value },
        { "il2cpp_field_static_set_value",      (void**)&f_field_static_set_value },
        { "il2cpp_assembly_get_image",          (void**)&f_assembly_get_image },
        { "il2cpp_domain_get",                  (void**)&f_domain_get },
        { "il2cpp_domain_get_assemblies",       (void**)&f_domain_get_assemblies },
        { "il2cpp_thread_attach",               (void**)&f_thread_attach },
        { "il2cpp_value_box",                   (void**)&f_value_box },
        { "il2cpp_image_get_class_count",       (void**)&f_image_get_class_count },
        { "il2cpp_image_get_class",             (void**)&f_image_get_class },
    };

    int resolved = 0;
    for (auto& exp : exports) {
        *exp.ptr = (void*)GetProcAddress(module, exp.name);
        if (*exp.ptr) {
            resolved++;
        } else {
            printf("[Core] WARN: Export not found: %s\n", exp.name);
        }
    }

    printf("[Core] Resolved %d/%zu IL2CPP exports\n", resolved, sizeof(exports)/sizeof(exports[0]));
    return resolved > 0;
}

// ===
// Core Implementation  
// ===

void Core::init() {
    if (initialized) return;
    
    printf("[Vertice] Initializing IL2CPP core...\n");

    if (!scan_for_il2cpp()) {
        printf("[Vertice] FATAL: Could not resolve IL2CPP exports\n");
        return;
    }

    if (f_domain_get) {
        Il2CppDomain* domain = f_domain_get();
        if (domain && f_domain_get_assemblies) {
            size_t assembly_count = 0;
            Il2CppAssembly** assemblies = (Il2CppAssembly**)f_domain_get_assemblies(&assembly_count);
            if (assemblies) {
                for (size_t i = 0; i < assembly_count; i++) {
                    if (!assemblies[i]) continue;
                    Il2CppImage* img = (Il2CppImage*)f_assembly_get_image(assemblies[i]);
                    if (!img) continue;
                    // Store known assemblies by heuristic
                    printf("[Core] Assembly: %p\n", img);
                    if (!assembly_csharp) assembly_csharp = img;
                }
            }
        }
    }

    initialized = true;
    printf("[Vertice] IL2CPP core initialized\n");
}

void Core::shutdown() {
    if (!initialized) return;
    initialized = false;
    assembly_csharp = nullptr;
    unity_engine = nullptr;
    printf("[Vertice] IL2CPP core shutdown\n");
}

Il2CppClass* Core::find_class(const char* namespaze, const char* name) {
    if (!f_class_from_name) return nullptr;
    return f_class_from_name(assembly_csharp, namespaze, name);
}

Il2CppClass* Core::find_class_unsafe(const char* namespaze, const char* name) {
    return find_class(namespaze, name);
}

MethodInfo* Core::find_method(Il2CppClass* klass, const char* name, int args) {
    if (!f_class_get_method_from_name || !klass) return nullptr;
    return f_class_get_method_from_name(klass, name, args);
}

MethodInfo* Core::find_method(const char* namespaze, const char* klass, const char* name, int args) {
    Il2CppClass* k = find_class(namespaze, klass);
    if (!k) return nullptr;
    return find_method(k, name, args);
}

void* Core::find_method_ptr(const char* namespaze, const char* klass, const char* name, int args) {
    MethodInfo* m = find_method(namespaze, klass, name, args);
    if (!m) return nullptr;
    __try {
        return *(void**)m; // MethodInfo->methodPointer
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

FieldInfo* Core::find_field(Il2CppClass* klass, const char* name) {
    if (!f_class_get_field || !klass) return nullptr;
    return f_class_get_field(klass, name);
}

FieldInfo* Core::find_field(const char* namespaze, const char* klass, const char* name) {
    Il2CppClass* k = find_class(namespaze, klass);
    return find_field(k, name);
}

uint32_t Core::get_field_offset(const char* namespaze, const char* klass, const char* field) {
    FieldInfo* f = find_field(namespaze, klass, field);
    if (!f) return UINT32_MAX;
    return f->offset;
}

uint32_t Core::get_static_field_offset(const char* namespaze, const char* klass, const char* field) {
    Il2CppClass* k = find_class(namespaze, klass);
    if (!k) return UINT32_MAX;
    
    // Iterate static fields
    void* iter = nullptr;
    FieldInfo* f = f_class_get_fields(k, &iter);
    while (f) {
        if (strcmp(f->name, field) == 0 && (f->flags & FIELD_ATTRIBUTE_STATIC)) {
            return f->offset;
        }
        f = f_class_get_fields(k, &iter);
    }
    return UINT32_MAX;
}

void* Core::get_static_field_ptr(const char* namespaze, const char* klass, const char* field) {
    Il2CppClass* k = find_class(namespaze, klass);
    if (!k) return nullptr;
    
    // Get static field pointer - this is typically in the class data
    uint32_t off = get_static_field_offset(namespaze, klass, field);
    if (off == UINT32_MAX) return nullptr;
    
    // Static fields are at negative offsets from class
    return (void*)((uintptr_t)k - off);
}

int Core::get_assembly_count() {
    if (!f_domain_get || !f_domain_get_assemblies) return 0;
    Il2CppDomain* domain = f_domain_get();
    if (!domain) return 0;
    size_t count = 0;
    f_domain_get_assemblies(&count);
    return (int)count;
}

Il2CppImage* Core::get_assembly_image(int index) {
    if (!f_domain_get || !f_domain_get_assemblies || index < 0) return nullptr;
    Il2CppDomain* domain = f_domain_get();
    if (!domain) return nullptr;
    size_t count = 0;
    auto assemblies = (Il2CppAssembly**)f_domain_get_assemblies(&count);
    if (!assemblies || (size_t)index >= count) return nullptr;
    if (!f_assembly_get_image) return nullptr;
    return (Il2CppImage*)f_assembly_get_image(assemblies[index]);
}

int Core::get_image_class_count(Il2CppImage* img) {
    if (!f_image_get_class_count || !img) return 0;
    return f_image_get_class_count(img);
}

Il2CppClass* Core::get_image_class(Il2CppImage* img, int index) {
    if (!f_image_get_class || !img || index < 0) return nullptr;
    return f_image_get_class(img, index);
}

void* Core::read_field_raw(Il2CppObject* obj, int32_t offset) {
    if (!obj || offset < 0) return nullptr;
    return (void*)((uintptr_t)obj + offset);
}

bool Core::field_is_static(FieldInfo* field) {
    if (!field) return false;
    return (field->flags & FIELD_ATTRIBUTE_STATIC) != 0;
}

FieldInfo* Core::get_class_next_field(Il2CppClass* klass, void** iter) {
    if (!f_class_get_fields || !klass || !iter) return nullptr;
    return (FieldInfo*)f_class_get_fields(klass, iter);
}

MethodInfo* Core::get_class_next_method(Il2CppClass* klass, void** iter) {
    if (!f_class_get_methods || !klass || !iter) return nullptr;
    return f_class_get_methods(klass, iter);
}

Il2CppString* Core::string_new(const char* str) {
    if (!f_string_new) return nullptr;
    return f_string_new(str);
}

Il2CppChar* Core::string_get_chars(Il2CppString* str) {
    if (!str || !f_string_chars) return nullptr;
    return f_string_chars(str);
}

Il2CppString* Core::string_concat(Il2CppString* a, Il2CppString* b) {
    if (!a || !b || !f_string_length || !f_string_chars || !f_string_new) return a;
    
    int len_a = f_string_length(a);
    int len_b = f_string_length(b);
    
    Il2CppChar* chars_a = f_string_chars(a);
    Il2CppChar* chars_b = f_string_chars(b);
    if (!chars_a || !chars_b) return a;
    
    // Build UTF-16 buffer then convert to UTF-8 for il2cpp_string_new
    int total_len = len_a + len_b;
    Il2CppChar* wide_buf = (Il2CppChar*)malloc((total_len + 1) * sizeof(Il2CppChar));
    if (!wide_buf) return a;
    
    memcpy(wide_buf, chars_a, len_a * sizeof(Il2CppChar));
    memcpy(wide_buf + len_a, chars_b, len_b * sizeof(Il2CppChar));
    wide_buf[total_len] = L'\0';
    
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wide_buf, total_len, nullptr, 0, nullptr, nullptr);
    char* utf8_buf = (char*)malloc(utf8_len + 1);
    if (!utf8_buf) {
        free(wide_buf);
        return a;
    }
    WideCharToMultiByte(CP_UTF8, 0, wide_buf, total_len, utf8_buf, utf8_len, nullptr, nullptr);
    utf8_buf[utf8_len] = '\0';
    
    Il2CppString* result = f_string_new(utf8_buf);
    free(wide_buf);
    free(utf8_buf);
    return result ? result : a;
}

void Core::field_static_get_value(FieldInfo* field, void* value) {
    if (!f_field_static_get_value || !field || !value) return;
    f_field_static_get_value(field, value);
}

Il2CppObject* Core::runtime_invoke(MethodInfo* method, Il2CppObject* obj, void** parameters, Il2CppException** exc) {
    if (!f_runtime_invoke) return nullptr;
    return f_runtime_invoke(method, obj, parameters, exc);
}

Il2CppObject* Core::runtime_invoke(const char* namespaze, const char* klass, const char* method, int args, Il2CppObject* obj, void** parameters) {
    MethodInfo* m = find_method(namespaze, klass, method, args);
    if (!m) return nullptr;
    return runtime_invoke(m, obj, parameters, nullptr);
}

Il2CppObject* Core::get_component(Il2CppObject* go, const char* component_name) {
    if (!go) return nullptr;
    
    Il2CppString* type_str = string_new(component_name);
    if (!type_str) return nullptr;
    void* params[] = { type_str };
    
    return runtime_invoke("UnityEngine", "GameObject", "GetComponent", 1, go, params);
}

Il2CppObject* Core::get_game_object(Il2CppObject* component) {
    if (!component) return nullptr;
    return runtime_invoke("UnityEngine", "Component", "get_gameObject", 0, component, nullptr);
}

Il2CppObject* Core::get_transform(Il2CppObject* go) {
    if (!go) return nullptr;
    // Try GameObject.get_transform() or GetComponent(Transform)
    Il2CppObject* tr = runtime_invoke("UnityEngine", "GameObject", "get_transform", 0, go, nullptr);
    if (tr) return tr;
    // Fallback: get_transform via Component cast
    return get_component(go, "Transform");
}

Il2CppObject* Core::get_main_camera() {
    // Camera.get_main static property
    return runtime_invoke("UnityEngine", "Camera", "get_main", 0, nullptr, nullptr);
}

Il2CppObject* Core::transform_get_position(Il2CppObject* transform) {
    if (!transform) return nullptr;
    return runtime_invoke("UnityEngine", "Transform", "get_position", 0, transform, nullptr);
}

void Core::transform_set_position(Il2CppObject* transform, float x, float y, float z) {
    if (!transform) return;
    
    Il2CppClass* vec3_class = find_class("UnityEngine", "Vector3");
    if (!vec3_class) return;
    
    // Box the Vector3 value
    float vec3_data[3] = { x, y, z };
    Il2CppObject* vec3_obj = nullptr;
    if (f_value_box) {
        vec3_obj = f_value_box(vec3_class, vec3_data);
    }
    if (!vec3_obj) return;
    
    void* params[] = { vec3_obj };
    runtime_invoke("UnityEngine", "Transform", "set_position", 1, transform, params);
}

Il2CppObject* Core::transform_get_forward(Il2CppObject* transform) {
    if (!transform) return nullptr;
    return runtime_invoke("UnityEngine", "Transform", "get_forward", 0, transform, nullptr);
}

Il2CppObject* Core::transform_get_right(Il2CppObject* transform) {
    if (!transform) return nullptr;
    return runtime_invoke("UnityEngine", "Transform", "get_right", 0, transform, nullptr);
}

Il2CppObject* Core::transform_get_up(Il2CppObject* transform) {
    if (!transform) return nullptr;
    return runtime_invoke("UnityEngine", "Transform", "get_up", 0, transform, nullptr);
}

Core::Vec3 Core::get_position(Il2CppObject* obj) {
    Il2CppObject* tr = get_transform(obj);
    if (!tr) return Vec3();
    
    Il2CppObject* pos = transform_get_position(tr);
    if (!pos) return Vec3();
    
    uint32_t ox = get_field_offset("UnityEngine", "Vector3", "x");
    uint32_t oy = get_field_offset("UnityEngine", "Vector3", "y");
    uint32_t oz = get_field_offset("UnityEngine", "Vector3", "z");
    
    return Vec3(
        read_field<float>(pos, ox),
        read_field<float>(pos, oy),
        read_field<float>(pos, oz)
    );
}

Core::Vec3 Core::world_to_screen(Il2CppObject* camera, Vec3 world_pos) {
    if (!camera) return Vec3();
    
    Il2CppClass* vec3_class = find_class("UnityEngine", "Vector3");
    if (!vec3_class) return Vec3();
    
    // Box the world position as a Vector3
    float vec3_data[3] = { world_pos.x, world_pos.y, world_pos.z };
    Il2CppObject* vec3_obj = nullptr;
    if (f_value_box) {
        vec3_obj = f_value_box(vec3_class, vec3_data);
    }
    if (!vec3_obj) return Vec3();
    
    void* params[] = { vec3_obj };
    Il2CppObject* screen_point = runtime_invoke("UnityEngine", "Camera", "WorldToScreenPoint", 1, camera, params);
    if (!screen_point) return Vec3();
    
    uint32_t ox = get_field_offset("UnityEngine", "Vector3", "x");
    uint32_t oy = get_field_offset("UnityEngine", "Vector3", "y");
    uint32_t oz = get_field_offset("UnityEngine", "Vector3", "z");
    
    return Vec3(
        read_field<float>(screen_point, ox),
        read_field<float>(screen_point, oy),
        read_field<float>(screen_point, oz)
    );
}

} // namespace vertice