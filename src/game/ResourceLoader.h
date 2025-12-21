#ifndef DDD_GAME_RESOURCE_LOADER_H
#define DDD_GAME_RESOURCE_LOADER_H

#include <string>
#include <vector>

class ResourceManager;

class ResourceLoader {
  public:
    static void loadOptionalDebugFontCandidates(ResourceManager &resourceManager,
                                                const std::vector<std::string> &candidates);
    static void loadTilesAndAtlasRegions(ResourceManager &resourceManager);
    static void loadPreferredDebugFont(ResourceManager &resourceManager);
};

#endif // DDD_GAME_RESOURCE_LOADER_H


