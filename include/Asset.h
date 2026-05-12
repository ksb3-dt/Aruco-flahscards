#pragma once

#include <memory>
#include <string>

class Renderer;  // forward declaration — avoids pulling Renderer.h into clients

/// Abstract base for anything that can be overlaid onto a marker. Concrete
/// subclasses model distinct content types: a textured image, a 3D
/// wireframe, etc.
///
/// Each subclass also acts as a Factory (Method pattern) for its compatible
/// Renderer — clients ask the asset for a renderer and stay decoupled from
/// the concrete renderer hierarchy.
class Asset {
public:
    virtual ~Asset() = default;

    /// Human-readable identifier for logs (e.g. "skeleton.png").
    virtual std::string name() const = 0;

    /// Factory Method: returns a Renderer that knows how to draw this asset.
    /// Each Asset subclass returns its matching Renderer subclass.
    virtual std::unique_ptr<Renderer> createRenderer() const = 0;

    /// Loads an asset from a file, dispatching on the extension:
    ///   .png .jpg .jpeg .bmp -> TextureAsset
    ///   .obj                 -> WireframeAsset (subset of OBJ supported)
    /// Throws std::invalid_argument for unrecognized extensions.
    static std::unique_ptr<Asset> createFromFile(const std::string& path);

protected:
    Asset() = default;
    Asset(const Asset&) = delete;
    Asset& operator=(const Asset&) = delete;
};
