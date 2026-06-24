#include "hooks.h"

namespace vertice {

std::vector<Toggle> FeatureManager::toggles;
std::vector<std::function<void()>> FeatureManager::updates;
std::vector<std::function<void()>> FeatureManager::on_enable;
std::vector<std::function<void()>> FeatureManager::on_disable;
bool FeatureManager::menu_open = false;

Network::CmdCache Network::cache[32];
int Network::cache_count = 0;

} // namespace vertice