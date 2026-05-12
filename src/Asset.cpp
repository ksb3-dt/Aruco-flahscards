#include "Asset.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>

#include <opencv2/core.hpp>

#include "ModelAsset.h"

namespace {

std::string lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) {
                       return static_cast<char>(std::tolower(ch));
                   });
    return value;
}

}  // namespace

std::unique_ptr<Asset> Asset::createFromFile(const std::string& path) {
    const std::string extension =
        lowercase(std::filesystem::path(path).extension().string());

    if (extension == ".obj") {
        return std::make_unique<ModelAsset>(
            path, cv::Vec3f(0.2F, 0.8F, 1.0F), 0.05);
    }

    throw std::invalid_argument(
        "Asset::createFromFile: unsupported asset extension '" +
        extension + "' for " + path);
}
