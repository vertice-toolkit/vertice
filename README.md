# Vertice Framework

Reusable C++ framework for Unity modding — supports **IL2CPP** and **Mono** runtimes (auto-detected).

## What's inside

| File | Purpose |
|------|---------|
| `core.h/cpp` | IL2CPP + Mono runtime wrappers (classes, methods, fields) |
| `hooks.h/cpp` | MinHook-based hooking + feature toggle system |
| `esp.h` | World-to-screen + ESP rendering helpers |
| `vertice.h` | Main include — pulls in everything + convenience macros |

## Quick example

```cpp
#include "vertice/vertice.h"

// Toggles with keybinds
TOGGLE(godmode, VK_F1);
TOGGLE(speed_hack, VK_F2);
CFG_FLOAT(speed_multiplier, 3.0f);

// Hook a method
MAKE_HOOK(take_damage, bool, PlayerHealth, TakeDamage, 3,
    void* health, float amount, void* attacker, float delay)
{
    if (godmode) amount = 0.0f;
    return take_damage_orig(health, amount, attacker, delay);
}

void init() {
    INSTALL_HOOK(take_damage, PlayerHealth, TakeDamage, 3);
    resolve_all(); // resolve field offsets
}
```

More vertice helpers:
```cpp
auto method = FIND_METHOD(PlayerHealth, TakeDamage, 3);
auto gm     = GET_SINGLETON(GameManager);
auto local  = GET_LOCAL(PlayerController);
auto items  = FIND_ALL(HeldItemInteractable);
READ(player, float, health_offset);
WRITE(player, float, health_offset, 100.0f);
CALL_CMD(player, PlayerHealth, CmdTakeDamage, &target, &params);
```

## How to use

1. Copy the `vertice/` folder into your project.
2. Create a game offsets file resolving field offsets via `GET_OFFSET(ClassName, fieldName)`.
3. Define features with `TOGGLE` / `CFG_FLOAT` / `MAKE_HOOK` / `INSTALL_HOOK`.
4. Call your init + update loop from `dllmain`.

## Building

**CMake** — add to your `CMakeLists.txt`:
```cmake
include_directories(path/to/vertice)
add_library(my_cheat SHARED
    my_cheat.cpp
    path/to/vertice/core.cpp
    path/to/vertice/hooks.cpp
)
```

**Visual Studio** — add `vertice/*.cpp` to your project and add `vertice/` to the include paths.

## Runtime Detection

Vertice auto-detects the Unity runtime at startup:

1. **IL2CPP** — looks for `GameAssembly.dll` exports (`il2cpp_*`)
2. **Mono** — falls back to `mono-2.0-bdwgc.dll` / `mono.dll` exports (`mono_*`)

Use `vertice::Core::runtime_name()` or `Core::current_runtime` to check which runtime was detected.

## Limitations

- **Mono hooks** — `INSTALL_HOOK` uses `mono_compile_method()` to resolve JIT-compiled method pointers; some methods may not be compilable until called at least once.
- **Static field pointers** — `get_static_field_ptr()` is IL2CPP-only; use `field_static_get_value()` for Mono.
- **read_static_field / write_static_field templates** — these access `klass->namespaze` (IL2CPP-specific); for Mono, use the explicit `get_static_field_offset(namespace, class, field)` pattern instead.

## License

MIT
