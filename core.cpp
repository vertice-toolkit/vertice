#include "core.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <MinHook.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#define FIELD_ATTRIBUTE_STATIC 0x10

// ===
// Translation tables — populate these in your game init/offsets file
// e.g. tClasses[u8"ႣႣႤႡႤႠႦႛႝႨႣ"] = "DiffuseAudio";
// ===
translations tClasses;
translations tFields;
translations tMethods;

std::string translate(const translations& map, const std::string& value)
{
    const auto pos = map.find(value);
    if (pos == map.end())
        return value;
    return pos->second;
}

namespace vertice {

bool Core::initialized = false;
Runtime Core::current_runtime = Runtime::Unknown;

Il2CppImage* Core::assembly_csharp = nullptr;
Il2CppImage* Core::unity_engine = nullptr;
Il2CppImage* Core::assembly_mscorlib = nullptr;

MonoImage* Core::mono_assembly_csharp = nullptr;
MonoImage* Core::mono_unity_engine = nullptr;
MonoImage* Core::mono_mscorlib = nullptr;

static std::vector<Il2CppImage*> g_all_images;
static std::vector<MonoImage*> g_all_mono_images;

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
typedef FieldInfo* (*il2cpp_class_get_fields_t)(Il2CppClass* klass, void** iter);
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

// ===
// Internal Mono Types
// ===

typedef MonoClass* (*mono_class_from_name_t)(MonoImage* image, const char* namespaze, const char* name);
typedef MonoClass* (*mono_class_get_t)(MonoImage* image, uint32_t type_index);
typedef const char* (*mono_class_get_name_t)(MonoClass* klass);
typedef const char* (*mono_class_get_namespace_t)(MonoClass* klass);
typedef MonoClass* (*mono_class_get_parent_t)(MonoClass* klass);
typedef int (*mono_class_get_flags_t)(MonoClass* klass);
typedef int (*mono_class_is_enum_t)(MonoClass* klass);
typedef int (*mono_class_instance_size_t)(MonoClass* klass);
typedef int (*mono_class_value_size_t)(MonoClass* klass, int* align);
typedef MonoClassField* (*mono_class_get_fields_t)(MonoClass* klass, void** iter);
typedef MonoClassField* (*mono_class_get_field_from_name_t)(MonoClass* klass, const char* name);
typedef MonoMethod* (*mono_class_get_methods_t)(MonoClass* klass, void** iter);
typedef MonoMethod* (*mono_class_get_method_from_name_t)(MonoClass* klass, const char* name, int argsCount);
typedef int (*mono_class_num_fields_t)(MonoClass* klass);
typedef int (*mono_class_num_methods_t)(MonoClass* klass);

typedef MonoString* (*mono_string_new_t)(MonoDomain* domain, const char* str);
typedef int (*mono_string_length_t)(MonoString* str);
typedef MonoChar* (*mono_string_chars_t)(MonoString* str);
typedef char* (*mono_string_to_utf8_t)(MonoString* str);

typedef MonoObject* (*mono_value_box_t)(MonoDomain* domain, MonoClass* klass, void* data);

typedef const char* (*mono_method_get_name_t)(MonoMethod* method);
typedef int (*mono_method_get_param_count_t)(MonoMethod* method);
typedef MonoMethod* (*mono_method_get_declaring_type_t)(MonoMethod* method);
typedef void* (*mono_compile_method_t)(MonoMethod* method);

typedef uint32_t (*mono_field_get_offset_t)(MonoClassField* field);
typedef const char* (*mono_field_get_name_t)(MonoClassField* field);
typedef uint32_t (*mono_field_get_flags_t)(MonoClassField* field);
typedef void (*mono_field_get_value_t)(MonoObject* obj, MonoClassField* field, void* value);
typedef void (*mono_field_set_value_t)(MonoObject* obj, MonoClassField* field, void* value);
typedef void (*mono_field_static_get_value_t)(MonoClass* klass, MonoClassField* field, void* value);
typedef void (*mono_field_static_set_value_t)(MonoClass* klass, MonoClassField* field, void* value);
typedef MonoClass* (*mono_field_get_parent_t)(MonoClassField* field);

typedef MonoObject* (*mono_runtime_invoke_t)(MonoMethod* method, void* obj, void** params, MonoObject** exc);

typedef MonoDomain* (*mono_get_root_domain_t)();
typedef MonoDomain* (*mono_domain_get_t)();
typedef void (*mono_domain_foreach_t)(void(*callback)(MonoDomain*, void*), void* user_data);
typedef void (*mono_domain_get_assemblies_t)(MonoDomain* domain, void** iter, MonoAssembly*** assemblies, int* count);
typedef MonoImage* (*mono_assembly_get_image_t)(MonoAssembly* assembly);
typedef const char* (*mono_image_get_name_t)(MonoImage* image);
typedef int (*mono_image_get_table_rows_t)(MonoImage* image, int table_id);

typedef void (*mono_thread_attach_t)(MonoDomain* domain);
typedef MonoImage* (*mono_image_loaded_t)(const char* name);

// Mono table IDs (for mono_image_get_table_rows)
#define MONO_TABLE_TYPEDEF 2

// Global IL2CPP function pointers
static il2cpp_class_from_name_t f_class_from_name = nullptr;
static il2cpp_class_get_fields_t f_class_get_fields = nullptr;
static il2cpp_class_get_field_t f_class_get_field = nullptr;
static il2cpp_class_get_methods_t f_class_get_methods = nullptr;
static il2cpp_class_get_method_from_name_t f_class_get_method_from_name = nullptr;
static il2cpp_class_get_name_t f_class_get_name = nullptr;
static il2cpp_class_get_namespace_t f_class_get_namespace = nullptr;
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

// Global Mono function pointers
static mono_class_from_name_t f_mono_class_from_name = nullptr;
static mono_class_get_t f_mono_class_get = nullptr;
static mono_class_get_name_t f_mono_class_get_name = nullptr;
static mono_class_get_namespace_t f_mono_class_get_namespace = nullptr;
static mono_class_get_fields_t f_mono_class_get_fields = nullptr;
static mono_class_get_field_from_name_t f_mono_class_get_field_from_name = nullptr;
static mono_class_get_methods_t f_mono_class_get_methods = nullptr;
static mono_class_get_method_from_name_t f_mono_class_get_method_from_name = nullptr;
static mono_class_num_fields_t f_mono_class_num_fields = nullptr;
static mono_class_num_methods_t f_mono_class_num_methods = nullptr;
static mono_string_new_t f_mono_string_new = nullptr;
static mono_string_length_t f_mono_string_length = nullptr;
static mono_string_chars_t f_mono_string_chars = nullptr;
static mono_string_to_utf8_t f_mono_string_to_utf8 = nullptr;
static mono_runtime_invoke_t f_mono_runtime_invoke = nullptr;
static mono_field_static_get_value_t f_mono_field_static_get_value = nullptr;
static mono_field_static_set_value_t f_mono_field_static_set_value = nullptr;
static mono_field_get_offset_t f_mono_field_get_offset = nullptr;
static mono_field_get_name_t f_mono_field_get_name = nullptr;
static mono_field_get_flags_t f_mono_field_get_flags = nullptr;
static mono_field_get_value_t f_mono_field_get_value = nullptr;
static mono_field_set_value_t f_mono_field_set_value = nullptr;
static mono_value_box_t f_mono_value_box = nullptr;
static mono_assembly_get_image_t f_mono_assembly_get_image = nullptr;
static mono_get_root_domain_t f_mono_get_root_domain = nullptr;
static mono_domain_get_t f_mono_domain_get = nullptr;
static mono_domain_get_assemblies_t f_mono_domain_get_assemblies = nullptr;
static mono_thread_attach_t f_mono_thread_attach = nullptr;
static mono_image_get_name_t f_mono_image_get_name = nullptr;
static mono_image_get_table_rows_t f_mono_image_get_table_rows = nullptr;
static mono_image_loaded_t f_mono_image_loaded = nullptr;
static mono_method_get_name_t f_mono_method_get_name = nullptr;
static mono_method_get_param_count_t f_mono_method_get_param_count = nullptr;
static mono_compile_method_t f_mono_compile_method = nullptr;
static mono_field_get_parent_t f_mono_field_get_parent = nullptr;

static HMODULE g_mono_module = nullptr;
static MonoDomain* g_mono_root_domain = nullptr;

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
        { "il2cpp_class_get_name",              (void**)&f_class_get_name },
        { "il2cpp_class_get_namespace",         (void**)&f_class_get_namespace },
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

static bool scan_for_mono() {
    const char* modules[] = {
        "mono-2.0-bdwgc.dll",
        "mono.dll",
        "mono-2.0-sgen.dll",
        "libmono.dll",
    };
    
    g_mono_module = nullptr;
    for (auto mod : modules) {
        g_mono_module = GetModuleHandleA(mod);
        if (g_mono_module) break;
    }
    if (!g_mono_module) {
        // Also try unityplayer.dll (some builds embed Mono)
        g_mono_module = GetModuleHandleA("unityplayer.dll");
    }
    if (!g_mono_module) {
        printf("[Core] Could not find Mono module\n");
        return false;
    }

    struct MonoExport {
        const char* name;
        void** ptr;
    };

    MonoExport exports[] = {
        { "mono_class_from_name",               (void**)&f_mono_class_from_name },
        { "mono_class_get",                     (void**)&f_mono_class_get },
        { "mono_class_get_name",                (void**)&f_mono_class_get_name },
        { "mono_class_get_namespace",           (void**)&f_mono_class_get_namespace },
        { "mono_class_get_fields",              (void**)&f_mono_class_get_fields },
        { "mono_class_get_field_from_name",     (void**)&f_mono_class_get_field_from_name },
        { "mono_class_get_methods",             (void**)&f_mono_class_get_methods },
        { "mono_class_get_method_from_name",    (void**)&f_mono_class_get_method_from_name },
        { "mono_class_num_fields",              (void**)&f_mono_class_num_fields },
        { "mono_class_num_methods",             (void**)&f_mono_class_num_methods },
        { "mono_string_new",                    (void**)&f_mono_string_new },
        { "mono_string_length",                 (void**)&f_mono_string_length },
        { "mono_string_chars",                  (void**)&f_mono_string_chars },
        { "mono_string_to_utf8",                (void**)&f_mono_string_to_utf8 },
        { "mono_runtime_invoke",                (void**)&f_mono_runtime_invoke },
        { "mono_field_static_get_value",        (void**)&f_mono_field_static_get_value },
        { "mono_field_static_set_value",        (void**)&f_mono_field_static_set_value },
        { "mono_field_get_offset",              (void**)&f_mono_field_get_offset },
        { "mono_field_get_name",                (void**)&f_mono_field_get_name },
        { "mono_field_get_flags",               (void**)&f_mono_field_get_flags },
        { "mono_field_get_value",               (void**)&f_mono_field_get_value },
        { "mono_field_set_value",               (void**)&f_mono_field_set_value },
        { "mono_value_box",                     (void**)&f_mono_value_box },
        { "mono_assembly_get_image",            (void**)&f_mono_assembly_get_image },
        { "mono_get_root_domain",               (void**)&f_mono_get_root_domain },
        { "mono_domain_get",                    (void**)&f_mono_domain_get },
        { "mono_domain_get_assemblies",         (void**)&f_mono_domain_get_assemblies },
        { "mono_thread_attach",                 (void**)&f_mono_thread_attach },
        { "mono_image_get_name",                (void**)&f_mono_image_get_name },
        { "mono_image_get_table_rows",          (void**)&f_mono_image_get_table_rows },
        { "mono_image_loaded",                  (void**)&f_mono_image_loaded },
        { "mono_method_get_name",               (void**)&f_mono_method_get_name },
        { "mono_method_get_param_count",        (void**)&f_mono_method_get_param_count },
        { "mono_compile_method",                (void**)&f_mono_compile_method },
        { "mono_field_get_parent",              (void**)&f_mono_field_get_parent },
    };

    int resolved = 0;
    for (auto& exp : exports) {
        *exp.ptr = (void*)GetProcAddress(g_mono_module, exp.name);
        if (*exp.ptr) {
            resolved++;
        }
    }

    // Fallback: if mono_class_get_field_from_name wasn't found, try mono_class_get_field
    if (!f_mono_class_get_field_from_name) {
        f_mono_class_get_field_from_name = (mono_class_get_field_from_name_t)GetProcAddress(g_mono_module, "mono_class_get_field");
        if (f_mono_class_get_field_from_name) {
            printf("[Core] Using mono_class_get_field as fallback\n");
            resolved++;
        }
    }

    printf("[Core] Resolved %d/%zu Mono exports\n", resolved, sizeof(exports)/sizeof(exports[0]));
    return resolved > 10; // require at least a reasonable subset
}

// ===
// Core Implementation  
// ===

void Core::init() {
    if (initialized) return;
    
    // Try IL2CPP first
    if (scan_for_il2cpp()) {
        current_runtime = Runtime::IL2CPP;
        printf("[Vertice] Initializing IL2CPP core...\n");
    }
    // Fall back to Mono
    else if (scan_for_mono()) {
        current_runtime = Runtime::Mono;
        printf("[Vertice] Initializing Mono core...\n");
    }
    else {
        printf("[Vertice] FATAL: Could not resolve IL2CPP or Mono exports\n");
        return;
    }

    // Enumerate assemblies based on detected runtime
    if (current_runtime == Runtime::IL2CPP) {
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
                        printf("[Core] Assembly: %p\n", img);
                        g_all_images.push_back(img);
                        if (img->name) {
                            if (strcmp(img->name, "Assembly-CSharp.dll") == 0 || strcmp(img->name, "Assembly-CSharp") == 0)
                                assembly_csharp = img;
                            else if (strstr(img->name, "UnityEngine"))
                                unity_engine = img;
                            else if (strcmp(img->name, "mscorlib.dll") == 0 || strcmp(img->name, "mscorlib") == 0)
                                assembly_mscorlib = img;
                        }
                        if (!assembly_csharp) assembly_csharp = img;
                    }
                }
            }
        }
    }
    else if (current_runtime == Runtime::Mono) {
        if (f_mono_get_root_domain) g_mono_root_domain = f_mono_get_root_domain();
        if (!g_mono_root_domain && f_mono_domain_get) g_mono_root_domain = f_mono_domain_get();
        if (g_mono_root_domain && f_mono_thread_attach) {
            f_mono_thread_attach(g_mono_root_domain);
            printf("[Core] Mono thread attached\n");
        }

        // Fast path: try mono_image_loaded for well-known assemblies
        if (f_mono_image_loaded) {
            const char* targets[] = {
                "Assembly-CSharp.dll", "Assembly-CSharp",
                "UnityEngine.dll", "UnityEngine",
                "mscorlib.dll", "mscorlib",
            };
            MonoImage* results[3] = { nullptr, nullptr, nullptr }; // csharp, unity, mscorlib
            for (int t = 0; t < 6; t++) {
                MonoImage* img = f_mono_image_loaded(targets[t]);
                if (img) {
                    const char* iname = f_mono_image_get_name ? f_mono_image_get_name(img) : targets[t];
                    printf("[Core] Mono fast image: %s = %p\n", iname ? iname : targets[t], img);
                    if (t < 2) results[0] = img; // Assembly-CSharp
                    else if (t < 4) results[1] = img; // UnityEngine
                    else results[2] = img; // mscorlib
                }
            }
            // Push found images and register them
            for (int i = 0; i < 3; i++) {
                if (results[i]) {
                    g_all_mono_images.push_back(results[i]);
                    if (i == 0) mono_assembly_csharp = results[i];
                    else if (i == 1) mono_unity_engine = results[i];
                    else mono_mscorlib = results[i];
                }
            }
        }

        if (g_mono_root_domain && f_mono_domain_get_assemblies && f_mono_assembly_get_image) {
            void* iter = nullptr;
            MonoAssembly** assemblies = nullptr;
            int count = 0;
            f_mono_domain_get_assemblies(g_mono_root_domain, &iter, &assemblies, &count);
            if (assemblies) {
                for (int i = 0; i < count; i++) {
                    if (!assemblies[i]) continue;
                    MonoImage* img = f_mono_assembly_get_image(assemblies[i]);
                    if (!img) continue;
                    printf("[Core] Mono Assembly: %p\n", img);
                    g_all_mono_images.push_back(img);
                    if (f_mono_image_get_name) {
                        const char* name = f_mono_image_get_name(img);
                        if (name) {
                            if (strcmp(name, "Assembly-CSharp.dll") == 0 || strcmp(name, "Assembly-CSharp") == 0)
                                mono_assembly_csharp = img;
                            else if (strstr(name, "UnityEngine"))
                                mono_unity_engine = img;
                            else if (strcmp(name, "mscorlib.dll") == 0 || strcmp(name, "mscorlib") == 0)
                                mono_mscorlib = img;
                        }
                    }
                    if (!mono_assembly_csharp) mono_assembly_csharp = img;
                }
            }
        }
    }

    initialized = true;
    printf("[Vertice] Core initialized (%s)\n", runtime_name());
}

const char* Core::runtime_name() {
    auto rt = current_runtime;
    switch (rt) {
        case Runtime::IL2CPP: return "IL2CPP";
        case Runtime::Mono:   return "Mono";
        default:              return "Unknown";
    }
}

void Core::shutdown() {
    if (!initialized) return;
    initialized = false;
    current_runtime = Runtime::Unknown;
    assembly_csharp = nullptr;
    unity_engine = nullptr;
    assembly_mscorlib = nullptr;
    mono_assembly_csharp = nullptr;
    mono_unity_engine = nullptr;
    mono_mscorlib = nullptr;
    g_all_images.clear();
    g_all_mono_images.clear();
    g_mono_module = nullptr;
    g_mono_root_domain = nullptr;

    // Reset all IL2CPP function pointers
    f_class_from_name = nullptr;
    f_class_get_fields = nullptr;
    f_class_get_field = nullptr;
    f_class_get_methods = nullptr;
    f_class_get_method_from_name = nullptr;
    f_class_get_name = nullptr;
    f_class_get_namespace = nullptr;
    f_string_new = nullptr;
    f_string_length = nullptr;
    f_string_chars = nullptr;
    f_runtime_invoke = nullptr;
    f_field_static_get_value = nullptr;
    f_field_static_set_value = nullptr;
    f_value_box = nullptr;
    f_assembly_get_image = nullptr;
    f_domain_get = nullptr;
    f_domain_get_assemblies = nullptr;
    f_thread_attach = nullptr;
    f_image_get_class_count = nullptr;
    f_image_get_class = nullptr;

    // Reset all Mono function pointers
    f_mono_class_from_name = nullptr;
    f_mono_class_get = nullptr;
    f_mono_class_get_name = nullptr;
    f_mono_class_get_namespace = nullptr;
    f_mono_class_get_fields = nullptr;
    f_mono_class_get_field_from_name = nullptr;
    f_mono_class_get_methods = nullptr;
    f_mono_class_get_method_from_name = nullptr;
    f_mono_class_num_fields = nullptr;
    f_mono_class_num_methods = nullptr;
    f_mono_string_new = nullptr;
    f_mono_string_length = nullptr;
    f_mono_string_chars = nullptr;
    f_mono_string_to_utf8 = nullptr;
    f_mono_runtime_invoke = nullptr;
    f_mono_field_static_get_value = nullptr;
    f_mono_field_static_set_value = nullptr;
    f_mono_field_get_offset = nullptr;
    f_mono_field_get_name = nullptr;
    f_mono_field_get_flags = nullptr;
    f_mono_field_get_value = nullptr;
    f_mono_field_set_value = nullptr;
    f_mono_value_box = nullptr;
    f_mono_assembly_get_image = nullptr;
    f_mono_get_root_domain = nullptr;
    f_mono_domain_get = nullptr;
    f_mono_domain_get_assemblies = nullptr;
    f_mono_thread_attach = nullptr;
    f_mono_image_get_name = nullptr;
    f_mono_image_get_table_rows = nullptr;
    f_mono_image_loaded = nullptr;
    f_mono_method_get_name = nullptr;
    f_mono_method_get_param_count = nullptr;
    f_mono_compile_method = nullptr;
    f_mono_field_get_parent = nullptr;

    printf("[Vertice] Core shutdown\n");
}

Il2CppClass* Core::find_class(const char* namespaze, const char* name) {
    if (current_runtime == Runtime::IL2CPP) {
        if (!f_class_from_name) return nullptr;

        // Try direct lookup on all known images
        for (auto img : g_all_images) {
            Il2CppClass* klass = f_class_from_name(img, namespaze, name);
            if (klass) return klass;
        }

        // Fallback: iterate all classes in all images using translate
        if (!f_class_get_name || !f_class_get_namespace || !f_image_get_class_count || !f_image_get_class)
            return nullptr;

        for (auto img : g_all_images) {
            int count = f_image_get_class_count(img);
            for (int i = 0; i < count; i++) {
                Il2CppClass* c = f_image_get_class(img, i);
                if (!c) continue;
                const char* ns = f_class_get_namespace(c);
                const char* cn = f_class_get_name(c);
                if (!ns || !cn) continue;
                if (strcmp(namespaze, ns) != 0) continue;
                if (strcmp(name, translate(tClasses, cn).c_str()) == 0)
                    return c;
            }
        }
        return nullptr;
    }
    else if (current_runtime == Runtime::Mono) {
        if (!f_mono_class_from_name) return nullptr;

        // Try direct lookup on all known mono images
        for (auto img : g_all_mono_images) {
            MonoClass* klass = f_mono_class_from_name(img, namespaze, name);
            if (klass) return (Il2CppClass*)klass;
        }

        // Fallback: iterate all classes in all images
        if (!f_mono_class_get_name || !f_mono_class_get_namespace || !f_mono_image_get_table_rows || !f_mono_class_get)
            return nullptr;

        for (auto img : g_all_mono_images) {
            int rows = f_mono_image_get_table_rows(img, MONO_TABLE_TYPEDEF);
            // Start from index 1 (index 0 is the pseudo-class <Module>)
            for (int i = 1; i < rows; i++) {
                MonoClass* c = f_mono_class_get(img, i);
                if (!c) continue;
                const char* ns = f_mono_class_get_namespace(c);
                const char* cn = f_mono_class_get_name(c);
                if (!ns || !cn) continue;
                if (strcmp(namespaze, ns) != 0) continue;
                if (strcmp(name, translate(tClasses, cn).c_str()) == 0)
                    return (Il2CppClass*)c;
            }
        }
        return nullptr;
    }
    return nullptr;
}

Il2CppClass* Core::find_class_unsafe(const char* namespaze, const char* name) {
    return find_class(namespaze, name);
}

MethodInfo* Core::find_method(Il2CppClass* klass, const char* name, int args) {
    if (!klass) return nullptr;

    if (current_runtime == Runtime::IL2CPP) {
        if (!f_class_get_method_from_name) return nullptr;

        MethodInfo* method = f_class_get_method_from_name(klass, name, args);
        if (method) return method;

        if (!f_class_get_methods) return nullptr;

        void* iter = nullptr;
        MethodInfo* m = f_class_get_methods(klass, &iter);
        while (m) {
            if (m->name && strcmp(name, translate(tMethods, m->name).c_str()) == 0) {
                if (args < 0 || m->parameters_count == args)
                    return m;
            }
            m = f_class_get_methods(klass, &iter);
        }
        return nullptr;
    }
    else if (current_runtime == Runtime::Mono) {
        MonoClass* mklass = (MonoClass*)klass;
        if (!f_mono_class_get_method_from_name) return nullptr;

        MonoMethod* method = f_mono_class_get_method_from_name(mklass, name, args);
        if (method) return (MethodInfo*)method;

        if (!f_mono_class_get_methods || !f_mono_method_get_name || !f_mono_method_get_param_count)
            return nullptr;

        void* iter = nullptr;
        MonoMethod* m = f_mono_class_get_methods(mklass, &iter);
        while (m) {
            const char* mname = f_mono_method_get_name(m);
            if (mname && strcmp(name, translate(tMethods, mname).c_str()) == 0) {
                if (args < 0 || f_mono_method_get_param_count(m) == args)
                    return (MethodInfo*)m;
            }
            m = f_mono_class_get_methods(mklass, &iter);
        }
        return nullptr;
    }
    return nullptr;
}

MethodInfo* Core::find_method(const char* namespaze, const char* klass, const char* name, int args) {
    Il2CppClass* k = find_class(namespaze, klass);
    if (!k) return nullptr;
    return find_method(k, name, args);
}

void* Core::find_method_ptr(const char* namespaze, const char* klass, const char* name, int args) {
    MethodInfo* m = find_method(namespaze, klass, name, args);
    if (!m) return nullptr;

    if (current_runtime == Runtime::IL2CPP) {
        return *(void**)m; // MethodInfo->methodPointer
    }
    else if (current_runtime == Runtime::Mono) {
        if (!f_mono_compile_method) return nullptr;
        return f_mono_compile_method((MonoMethod*)m);
    }
    return nullptr;
}

FieldInfo* Core::find_field(Il2CppClass* klass, const char* name) {
    if (!klass) return nullptr;

    if (current_runtime == Runtime::IL2CPP) {
        if (!f_class_get_field) return nullptr;

        FieldInfo* field = f_class_get_field(klass, name);
        if (field) return field;

        if (!f_class_get_fields) return nullptr;

        void* iter = nullptr;
        FieldInfo* f = f_class_get_fields(klass, &iter);
        while (f) {
            if (f->name && strcmp(name, translate(tFields, f->name).c_str()) == 0)
                return f;
            f = f_class_get_fields(klass, &iter);
        }
        return nullptr;
    }
    else if (current_runtime == Runtime::Mono) {
        MonoClass* mklass = (MonoClass*)klass;
        if (!f_mono_class_get_field_from_name) return nullptr;

        MonoClassField* field = f_mono_class_get_field_from_name(mklass, name);
        if (field) return (FieldInfo*)field;

        if (!f_mono_class_get_fields || !f_mono_field_get_name) return nullptr;

        void* iter = nullptr;
        MonoClassField* f = f_mono_class_get_fields(mklass, &iter);
        while (f) {
            const char* fname = f_mono_field_get_name(f);
            if (fname && strcmp(name, translate(tFields, fname).c_str()) == 0)
                return (FieldInfo*)f;
            f = f_mono_class_get_fields(mklass, &iter);
        }
        return nullptr;
    }
    return nullptr;
}

FieldInfo* Core::find_field(const char* namespaze, const char* klass, const char* name) {
    Il2CppClass* k = find_class(namespaze, klass);
    return find_field(k, name);
}

static uint32_t field_get_offset_impl(void* field) {
    if (Core::current_runtime == Runtime::IL2CPP) {
        return ((FieldInfo*)field)->offset;
    }
    else if (Core::current_runtime == Runtime::Mono) {
        if (!f_mono_field_get_offset) return UINT32_MAX;
        return f_mono_field_get_offset((MonoClassField*)field);
    }
    return UINT32_MAX;
}

static const char* field_get_name_impl(void* field) {
    if (Core::current_runtime == Runtime::IL2CPP) {
        return ((FieldInfo*)field)->name;
    }
    else if (Core::current_runtime == Runtime::Mono) {
        if (!f_mono_field_get_name) return nullptr;
        return f_mono_field_get_name((MonoClassField*)field);
    }
    return nullptr;
}

static uint32_t field_get_flags_impl(void* field) {
    if (Core::current_runtime == Runtime::IL2CPP) {
        return ((FieldInfo*)field)->flags;
    }
    else if (Core::current_runtime == Runtime::Mono) {
        if (!f_mono_field_get_flags) return 0;
        return f_mono_field_get_flags((MonoClassField*)field);
    }
    return 0;
}

uint32_t Core::get_field_offset(const char* namespaze, const char* klass, const char* field) {
    FieldInfo* f = find_field(namespaze, klass, field);
    if (!f) return UINT32_MAX;
    return field_get_offset_impl(f);
}

uint32_t Core::get_static_field_offset(const char* namespaze, const char* klass, const char* field) {
    Il2CppClass* k = find_class(namespaze, klass);
    if (!k) return UINT32_MAX;
    
    // Mono
    if (current_runtime == Runtime::Mono) {
        if (!f_mono_class_get_fields || !f_mono_field_get_flags || !f_mono_field_get_name || !f_mono_field_get_offset)
            return UINT32_MAX;
        MonoClass* mklass = (MonoClass*)k;
        void* iter = nullptr;
        MonoClassField* f = f_mono_class_get_fields(mklass, &iter);
        while (f) {
            const char* fname = f_mono_field_get_name(f);
            uint32_t flags = f_mono_field_get_flags(f);
            if (fname && strcmp(field, translate(tFields, fname).c_str()) == 0 && (flags & FIELD_ATTRIBUTE_STATIC)) {
                return f_mono_field_get_offset(f);
            }
            f = f_mono_class_get_fields(mklass, &iter);
        }
        return UINT32_MAX;
    }
    
    // IL2CPP
    if (!f_class_get_fields) return UINT32_MAX;
    void* iter = nullptr;
    FieldInfo* f = f_class_get_fields(k, &iter);
    while (f) {
        if (strcmp(field, translate(tFields, f->name).c_str()) == 0 && (f->flags & FIELD_ATTRIBUTE_STATIC)) {
            return f->offset;
        }
        f = f_class_get_fields(k, &iter);
    }
    return UINT32_MAX;
}

void* Core::get_static_field_ptr(const char* namespaze, const char* klass, const char* field) {
    if (current_runtime == Runtime::Mono) {
        return nullptr; // Mono does not expose static field addresses; use field_static_get_value instead
    }
    Il2CppClass* k = find_class(namespaze, klass);
    if (!k) return nullptr;
    
    // Get static field pointer - this is typically in the class data
    uint32_t off = get_static_field_offset(namespaze, klass, field);
    if (off == UINT32_MAX) return nullptr;
    
    // Static fields are at negative offsets from class
    return (void*)((uintptr_t)k - off);
}

int Core::get_assembly_count() {
    if (current_runtime == Runtime::IL2CPP) {
        if (!f_domain_get || !f_domain_get_assemblies) return 0;
        Il2CppDomain* domain = f_domain_get();
        if (!domain) return 0;
        size_t count = 0;
        f_domain_get_assemblies(&count);
        return (int)count;
    }
    else if (current_runtime == Runtime::Mono) {
        if (!g_mono_root_domain || !f_mono_domain_get_assemblies) return 0;
        void* iter = nullptr;
        MonoAssembly** assemblies = nullptr;
        int count = 0;
        f_mono_domain_get_assemblies(g_mono_root_domain, &iter, &assemblies, &count);
        return count;
    }
    return 0;
}

Il2CppImage* Core::get_assembly_image(int index) {
    if (current_runtime == Runtime::IL2CPP) {
        if (!f_domain_get || !f_domain_get_assemblies || index < 0) return nullptr;
        Il2CppDomain* domain = f_domain_get();
        if (!domain) return nullptr;
        size_t count = 0;
        auto assemblies = (Il2CppAssembly**)f_domain_get_assemblies(&count);
        if (!assemblies || (size_t)index >= count) return nullptr;
        if (!f_assembly_get_image) return nullptr;
        return (Il2CppImage*)f_assembly_get_image(assemblies[index]);
    }
    else if (current_runtime == Runtime::Mono) {
        if (!g_mono_root_domain || !f_mono_domain_get_assemblies || !f_mono_assembly_get_image || index < 0) return nullptr;
        void* iter = nullptr;
        MonoAssembly** assemblies = nullptr;
        int count = 0;
        f_mono_domain_get_assemblies(g_mono_root_domain, &iter, &assemblies, &count);
        if (!assemblies || index >= count) return nullptr;
        return (Il2CppImage*)f_mono_assembly_get_image(assemblies[index]);
    }
    return nullptr;
}

int Core::get_image_class_count(Il2CppImage* img) {
    if (!img) return 0;
    if (current_runtime == Runtime::IL2CPP) {
        if (!f_image_get_class_count) return 0;
        return f_image_get_class_count(img);
    }
    else if (current_runtime == Runtime::Mono) {
        if (!f_mono_image_get_table_rows) return 0;
        return f_mono_image_get_table_rows((MonoImage*)img, MONO_TABLE_TYPEDEF);
    }
    return 0;
}

Il2CppClass* Core::get_image_class(Il2CppImage* img, int index) {
    if (!img || index < 0) return nullptr;
    if (current_runtime == Runtime::IL2CPP) {
        if (!f_image_get_class) return nullptr;
        return f_image_get_class(img, index);
    }
    else if (current_runtime == Runtime::Mono) {
        if (!f_mono_class_get) return nullptr;
        return (Il2CppClass*)f_mono_class_get((MonoImage*)img, index);
    }
    return nullptr;
}

void* Core::read_field_raw(Il2CppObject* obj, int32_t offset) {
    if (!obj || offset < 0) return nullptr;
    return (void*)((uintptr_t)obj + offset);
}

bool Core::field_is_static(FieldInfo* field) {
    if (!field) return false;
    return (field_get_flags_impl(field) & FIELD_ATTRIBUTE_STATIC) != 0;
}

FieldInfo* Core::get_class_next_field(Il2CppClass* klass, void** iter) {
    if (!klass || !iter) return nullptr;
    if (current_runtime == Runtime::IL2CPP) {
        if (!f_class_get_fields) return nullptr;
        return f_class_get_fields(klass, iter);
    }
    else if (current_runtime == Runtime::Mono) {
        if (!f_mono_class_get_fields) return nullptr;
        return (FieldInfo*)f_mono_class_get_fields((MonoClass*)klass, iter);
    }
    return nullptr;
}

MethodInfo* Core::get_class_next_method(Il2CppClass* klass, void** iter) {
    if (!klass || !iter) return nullptr;
    if (current_runtime == Runtime::IL2CPP) {
        if (!f_class_get_methods) return nullptr;
        return f_class_get_methods(klass, iter);
    }
    else if (current_runtime == Runtime::Mono) {
        if (!f_mono_class_get_methods) return nullptr;
        return (MethodInfo*)f_mono_class_get_methods((MonoClass*)klass, iter);
    }
    return nullptr;
}

Il2CppString* Core::string_new(const char* str) {
    if (current_runtime == Runtime::IL2CPP) {
        if (!f_string_new) return nullptr;
        return f_string_new(str);
    }
    else if (current_runtime == Runtime::Mono) {
        if (!f_mono_string_new || !g_mono_root_domain) return nullptr;
        return (Il2CppString*)f_mono_string_new(g_mono_root_domain, str);
    }
    return nullptr;
}

Il2CppChar* Core::string_get_chars(Il2CppString* str) {
    if (!str) return nullptr;
    if (current_runtime == Runtime::IL2CPP) {
        if (!f_string_chars) return nullptr;
        return f_string_chars(str);
    }
    else if (current_runtime == Runtime::Mono) {
        if (!f_mono_string_chars) return nullptr;
        return (Il2CppChar*)f_mono_string_chars((MonoString*)str);
    }
    return nullptr;
}

Il2CppString* Core::string_concat(Il2CppString* a, Il2CppString* b) {
    if (!a || !b) return a;

    if (current_runtime == Runtime::IL2CPP) {
        if (!f_string_length || !f_string_chars || !f_string_new) return a;
        int len_a = f_string_length(a);
        int len_b = f_string_length(b);
        Il2CppChar* chars_a = f_string_chars(a);
        Il2CppChar* chars_b = f_string_chars(b);
        if (!chars_a || !chars_b) return a;
        int total_len = len_a + len_b;
        Il2CppChar* wide_buf = (Il2CppChar*)malloc((total_len + 1) * sizeof(Il2CppChar));
        if (!wide_buf) return a;
        memcpy(wide_buf, chars_a, len_a * sizeof(Il2CppChar));
        memcpy(wide_buf + len_a, chars_b, len_b * sizeof(Il2CppChar));
        wide_buf[total_len] = L'\0';
        int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wide_buf, total_len, nullptr, 0, nullptr, nullptr);
        char* utf8_buf = (char*)malloc(utf8_len + 1);
        if (!utf8_buf) { free(wide_buf); return a; }
        WideCharToMultiByte(CP_UTF8, 0, wide_buf, total_len, utf8_buf, utf8_len, nullptr, nullptr);
        utf8_buf[utf8_len] = '\0';
        Il2CppString* result = f_string_new(utf8_buf);
        free(wide_buf);
        free(utf8_buf);
        return result ? result : a;
    }
    else if (current_runtime == Runtime::Mono) {
        // Mono: use mono_string_concat or manual alternative
        // For simplicity, allocate a new buffer and use mono_string_new
        if (!f_mono_string_new || !g_mono_root_domain || !f_mono_string_length || !f_mono_string_chars || !f_mono_string_to_utf8)
            return a;
        char* u8_a = f_mono_string_to_utf8((MonoString*)a);
        char* u8_b = f_mono_string_to_utf8((MonoString*)b);
        if (!u8_a || !u8_b) return a;
        size_t len_a = strlen(u8_a);
        size_t len_b = strlen(u8_b);
        char* combined = (char*)malloc(len_a + len_b + 1);
        if (!combined) return a;
        memcpy(combined, u8_a, len_a);
        memcpy(combined + len_a, u8_b, len_b);
        combined[len_a + len_b] = '\0';
        MonoString* result = f_mono_string_new(g_mono_root_domain, combined);
        free(combined);
        return (Il2CppString*)result;
    }
    return a;
}

void Core::field_static_get_value(FieldInfo* field, void* value) {
    if (!field || !value) return;
    if (current_runtime == Runtime::IL2CPP) {
        if (!f_field_static_get_value) return;
        f_field_static_get_value(field, value);
    }
    else if (current_runtime == Runtime::Mono) {
        if (!f_mono_field_static_get_value) return;
        MonoClass* parent = nullptr;
        if (f_mono_field_get_parent) {
            parent = f_mono_field_get_parent((MonoClassField*)field);
        }
        f_mono_field_static_get_value(parent, (MonoClassField*)field, value);
    }
}

Il2CppObject* Core::runtime_invoke(MethodInfo* method, Il2CppObject* obj, void** parameters, Il2CppException** exc) {
    if (current_runtime == Runtime::IL2CPP) {
        if (!f_runtime_invoke) return nullptr;
        return f_runtime_invoke(method, obj, parameters, exc);
    }
    else if (current_runtime == Runtime::Mono) {
        if (!f_mono_runtime_invoke) return nullptr;
        return (Il2CppObject*)f_mono_runtime_invoke((MonoMethod*)method, obj, parameters, (MonoObject**)exc);
    }
    return nullptr;
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

static Il2CppObject* box_value(Il2CppClass* klass, void* data) {
    if (Core::current_runtime == Runtime::IL2CPP) {
        if (!f_value_box) return nullptr;
        return f_value_box(klass, data);
    }
    else if (Core::current_runtime == Runtime::Mono) {
        if (!f_mono_value_box || !g_mono_root_domain) return nullptr;
        return (Il2CppObject*)f_mono_value_box(g_mono_root_domain, (MonoClass*)klass, data);
    }
    return nullptr;
}

void Core::transform_set_position(Il2CppObject* transform, float x, float y, float z) {
    if (!transform) return;
    
    float vec3_data[3] = { x, y, z };
    
    if (current_runtime == Runtime::Mono) {
        // Mono runtime_invoke expects raw valuetype data pointer for struct params
        void* params[] = { (void*)vec3_data };
        runtime_invoke("UnityEngine", "Transform", "set_position", 1, transform, params);
    } else {
        // IL2CPP runtime_invoke expects boxed valuetype objects
        Il2CppClass* vec3_class = find_class("UnityEngine", "Vector3");
        if (!vec3_class) return;
        Il2CppObject* vec3_obj = box_value(vec3_class, vec3_data);
        if (!vec3_obj) return;
        void* params[] = { vec3_obj };
        runtime_invoke("UnityEngine", "Transform", "set_position", 1, transform, params);
    }
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
    Il2CppObject* vec3_obj = box_value(vec3_class, vec3_data);
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