# Vertice Framework

Reusable C++ framework for Unity IL2CPP game modding (and in future Unity Mono).

## What's inside

| File | Purpose |
|------|---------|
| `core.h/cpp` | IL2CPP runtime wrappers (classes, methods, fields) |
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

## Future

Unity **Mono** modding support is planned.

## License

MIT
