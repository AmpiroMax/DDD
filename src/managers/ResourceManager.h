#ifndef DDD_MANAGERS_RESOURCE_MANAGER_H
#define DDD_MANAGERS_RESOURCE_MANAGER_H

#include <SFML/Graphics.hpp>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

class ResourceManager {
  public:
    struct AtlasRegion {
        std::string textureName;
        sf::IntRect rect;
    };

    void setBasePaths(const std::string &resourcesRoot, const std::string &texturesDir, const std::string &fontsDir = "fonts") {
        resourcesPath = resourcesRoot;
        texturesPath = texturesDir;
        fontsPath = fontsDir;
    }

    std::string resolveTexturePath(const std::string &relative) const {
        return (std::filesystem::path(resourcesPath) / texturesPath / relative).string();
    }

    std::string resolveFontPath(const std::string &relative) const {
        return (std::filesystem::path(resourcesPath) / fontsPath / relative).string();
    }

    void loadTexture(const std::string &name, const std::string &path) {
        auto tex = std::make_unique<sf::Texture>();
        if (!tex->loadFromFile(path)) {
            throw std::runtime_error("Failed to load texture: " + path);
        }
        textures[name] = std::move(tex);
    }

    bool hasTexture(const std::string &name) const { return textures.count(name) != 0; }

    sf::Texture &getTexture(const std::string &name) {
        auto it = textures.find(name);
        if (it == textures.end())
            throw std::runtime_error("Texture not found: " + name);
        return *it->second;
    }

    void loadFont(const std::string &name, const std::string &path) {
        auto font = std::make_unique<sf::Font>();
        if (!font->loadFromFile(path)) {
            throw std::runtime_error("Failed to load font: " + path);
        }
        fonts[name] = std::move(font);
    }

    bool hasFont(const std::string &name) const { return fonts.count(name) != 0; }

    sf::Font &getFont(const std::string &name) {
        auto it = fonts.find(name);
        if (it == fonts.end())
            throw std::runtime_error("Font not found: " + name);
        return *it->second;
    }

    void registerAtlasRegion(const std::string &regionName, const std::string &textureName, const sf::IntRect &rect) {
        atlas[regionName] = AtlasRegion{textureName, rect};
    }

    bool hasAtlasRegion(const std::string &regionName) const { return atlas.count(regionName) != 0; }

    const AtlasRegion &getAtlasRegion(const std::string &regionName) const {
        auto it = atlas.find(regionName);
        if (it == atlas.end())
            throw std::runtime_error("Atlas region not found: " + regionName);
        return it->second;
    }

  private:
    std::string resourcesPath{"resources"};
    std::string texturesPath{"textures"};
    std::string fontsPath{"fonts"};

    std::unordered_map<std::string, std::unique_ptr<sf::Texture>> textures;
    std::unordered_map<std::string, std::unique_ptr<sf::Font>> fonts;
    std::unordered_map<std::string, AtlasRegion> atlas;
};

#endif // DDD_MANAGERS_RESOURCE_MANAGER_H
