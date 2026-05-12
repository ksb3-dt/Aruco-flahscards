#include "AssetManager.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include <opencv2/core/persistence.hpp>

#include "ModelAsset.h"

namespace {

cv::Vec3f parseColor(const cv::FileNode& node, const std::string& key) {
    if (node.empty()) {
        return cv::Vec3f(0.2F, 0.8F, 1.0F);
    }
    if (!node.isSeq() || node.size() != 3) {
        throw std::runtime_error(
            "AssetManager: color for marker " + key +
            " must be an array of three numbers");
    }

    cv::Vec3f color;
    for (int i = 0; i < 3; ++i) {
        const double value = static_cast<double>(node[i]);
        if (value < 0.0 || value > 1.0) {
            throw std::runtime_error(
                "AssetManager: color values for marker " + key +
                " must be in [0, 1]");
        }
        color[i] = static_cast<float>(value);
    }
    return color;
}

cv::Vec3f parseVec3(const cv::FileNode& node,
                    const std::string& key,
                    const std::string& field,
                    cv::Vec3f defaultValue) {
    if (node.empty()) {
        return defaultValue;
    }
    if (!node.isSeq() || node.size() != 3) {
        throw std::runtime_error(
            "AssetManager: " + field + " for marker " + key +
            " must be an array of three numbers");
    }

    cv::Vec3f value;
    for (int i = 0; i < 3; ++i) {
        value[i] = static_cast<float>(static_cast<double>(node[i]));
    }
    return value;
}

float parsePositiveFloat(const cv::FileNode& node,
                         const std::string& key,
                         const std::string& field,
                         float defaultValue) {
    if (node.empty()) {
        return defaultValue;
    }

    const double parsed = static_cast<double>(node);
    if (parsed <= 0.0) {
        throw std::runtime_error(
            "AssetManager: " + field + " for marker " + key +
            " must be positive");
    }
    return static_cast<float>(parsed);
}

int parseMarkerId(const std::string& key) {
    std::size_t parsed = 0;
    const int markerId = std::stoi(key, &parsed);
    if (parsed != key.size()) {
        throw std::runtime_error(
            "AssetManager: marker key is not a valid integer: " + key);
    }
    return markerId;
}

ModelAsset::Transform parseTransform(const cv::FileNode& node,
                                     const std::string& key) {
    ModelAsset::Transform transform;
    transform.scale =
        parsePositiveFloat(node["scale"], key, "scale", transform.scale);
    transform.rotationDegrees =
        parseVec3(node["rotation_deg"],
                  key,
                  "rotation_deg",
                  transform.rotationDegrees);
    transform.translation =
        parseVec3(node["translation"],
                  key,
                  "translation",
                  transform.translation);
    return transform;
}

std::unique_ptr<Asset> makeModelAsset(const cv::FileNode& node,
                                      const std::string& key,
                                      double markerSizeMeters) {
    if (!node.isMap()) {
        throw std::runtime_error(
            "AssetManager: marker " + key +
            " must map to an object with path and optional color");
    }

    const cv::FileNode pathNode = node["path"];
    const std::string path = static_cast<std::string>(pathNode);
    if (path.empty()) {
        throw std::runtime_error(
            "AssetManager: marker " + key + " is missing path");
    }
    if (path.size() < 4 || path.substr(path.size() - 4) != ".obj") {
        throw std::runtime_error(
            "AssetManager: only .obj assets are supported in this "
            "OpenGL milestone: " + path);
    }

    const cv::Vec3f color = parseColor(node["color"], key);
    const ModelAsset::Transform transform = parseTransform(node, key);
    return std::make_unique<ModelAsset>(
        path, color, markerSizeMeters, transform);
}

}  // namespace

AssetManager::AssetManager(double markerSizeMeters)
    : markerSizeMeters_(markerSizeMeters) {
    if (markerSizeMeters_ <= 0.0) {
        throw std::invalid_argument(
            "AssetManager: marker size must be positive");
    }
}

void AssetManager::registerAsset(int markerId, std::unique_ptr<Asset> asset) {
    if (!asset) {
        throw std::invalid_argument(
            "AssetManager: cannot register a null asset");
    }

    Entry entry;
    entry.renderer = asset->createRenderer();
    entry.asset = std::move(asset);
    entries_[markerId] = std::move(entry);
}

void AssetManager::registerDefaultAsset(std::unique_ptr<Asset> asset) {
    if (!asset) {
        throw std::invalid_argument(
            "AssetManager: cannot register a null default asset");
    }

    Entry entry;
    entry.renderer = asset->createRenderer();
    entry.asset = std::move(asset);
    defaultEntry_ = std::move(entry);
    hasDefaultEntry_ = true;
}

void AssetManager::loadFromConfig(const std::string& configPath) {
    cv::FileStorage storage(configPath,
                            cv::FileStorage::READ |
                                cv::FileStorage::FORMAT_JSON);
    if (!storage.isOpened()) {
        throw std::runtime_error(
            "AssetManager: failed to open asset config " + configPath);
    }

    const cv::FileNode root = storage.root();
    if (!root.isMap()) {
        throw std::runtime_error(
            "AssetManager: asset config root must be a JSON object");
    }

    for (cv::FileNodeIterator it = root.begin(); it != root.end(); ++it) {
        const cv::FileNode node = *it;
        const std::string key = node.name();
        if (key.empty() || key[0] == '_') {
            continue;
        }
        if (key == "default") {
            registerDefaultAsset(
                makeModelAsset(node, key, markerSizeMeters_));
            continue;
        }

        registerAsset(parseMarkerId(key),
                      makeModelAsset(node, key, markerSizeMeters_));
    }
}

const Asset* AssetManager::findAsset(int markerId) const {
    const auto it = entries_.find(markerId);
    if (it != entries_.end()) {
        return it->second.asset.get();
    }
    return hasDefaultEntry_ ? defaultEntry_.asset.get() : nullptr;
}

Renderer* AssetManager::findRenderer(int markerId) const {
    const auto it = entries_.find(markerId);
    if (it != entries_.end()) {
        return it->second.renderer.get();
    }
    return hasDefaultEntry_ ? defaultEntry_.renderer.get() : nullptr;
}
