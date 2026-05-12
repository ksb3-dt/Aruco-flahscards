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
    /// Creates a manager that normalizes model assets for this marker size.
    explicit AssetManager(double markerSizeMeters = 0.05);

    /// Registers an asset under a marker ID. The manager takes ownership
    /// and creates the matching renderer via asset->createRenderer().
    /// Replaces any existing entry for that ID.
    void registerAsset(int markerId, std::unique_ptr<Asset> asset);

    /// Registers a fallback asset used for detected markers without an
    /// explicit marker-ID mapping.
    void registerDefaultAsset(std::unique_ptr<Asset> asset);

    /// Loads marker-ID -> file-path mappings from a JSON config file.
    /// Expected OBJ-only format for this OpenGL milestone:
    ///
    ///   {
    ///     "default": { "path": "assets/skull/skull.obj", "color": [0.9, 0.8, 0.7] },
    ///     "3": { "path": "assets/cube.obj", "color": [0.2, 0.8, 1.0] }
    ///   }
    ///
    /// Keys beginning with '_' are ignored as comments. Explicit marker IDs
    /// override the default asset.
    void loadFromConfig(const std::string& configPath);

    /// Returns a non-owning pointer to the asset for `markerId`, or nullptr
    /// if no asset is registered for that ID.
    const Asset* findAsset(int markerId) const;

    /// Returns a non-owning pointer to the renderer for `markerId`, or
    /// nullptr if no asset is registered.
    Renderer* findRenderer(int markerId) const;

    /// Number of registered marker mappings.
    std::size_t size() const {
        return entries_.size() + (hasDefaultEntry_ ? 1U : 0U);
    }

private:
    struct Entry {
        std::unique_ptr<Asset>    asset;
        std::unique_ptr<Renderer> renderer;
    };
    double markerSizeMeters_;
    Entry defaultEntry_;
    bool hasDefaultEntry_ = false;
    std::unordered_map<int, Entry> entries_;
};
