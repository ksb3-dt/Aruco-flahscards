#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "Asset.h"
#include "Renderer.h"

/// Holds the marker-ID -> (Asset, Renderer) mapping. The manager owns both
/// objects via unique_ptr; clients borrow non-owning references.
///
/// Each Asset is asked for its matching Renderer at registration time
/// (Asset::createRenderer()), so the renderer is created once and cached.
class AssetManager {
public:
    /// Registers an asset under a marker ID. The manager takes ownership
    /// and creates the matching renderer via asset->createRenderer().
    /// Replaces any existing entry for that ID.
    void registerAsset(int markerId, std::unique_ptr<Asset> asset);

    /// Loads marker-ID -> file-path mappings from a JSON config file.
    /// Expected format (object keys are marker IDs as strings):
    ///
    ///   {
    ///     "3":  "assets/skeleton.png",
    ///     "7":  "assets/molecule.png",
    ///     "11": "assets/cube.obj"
    ///   }
    ///
    /// Each path is passed to Asset::createFromFile().
    void loadFromConfig(const std::string& configPath);

    /// Returns a non-owning pointer to the asset for `markerId`, or nullptr
    /// if no asset is registered for that ID.
    const Asset* findAsset(int markerId) const;

    /// Returns a non-owning pointer to the renderer for `markerId`, or
    /// nullptr if no asset is registered.
    Renderer* findRenderer(int markerId) const;

    /// Number of registered marker mappings.
    std::size_t size() const { return entries_.size(); }

private:
    struct Entry {
        std::unique_ptr<Asset>    asset;
        std::unique_ptr<Renderer> renderer;
    };
    std::unordered_map<int, Entry> entries_;
};
