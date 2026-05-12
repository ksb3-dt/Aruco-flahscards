#pragma once

#include <string>
#include <vector>

#include <opencv2/core.hpp>

#include "Asset.h"

/// Triangle mesh loaded from an OBJ file and normalized into marker-local
/// coordinates. The marker plane is XY and +Z points out of the card.
class ModelAsset final : public Asset {
public:
    struct Vertex {
        cv::Vec3f position;
        cv::Vec3f normal;
        cv::Vec2f texCoord;
    };

    /// Optional placement adjustment applied after auto-fit normalization.
    struct Transform {
        float scale = 1.0F;
        cv::Vec3f rotationDegrees = cv::Vec3f(0.0F, 0.0F, 0.0F);
        cv::Vec3f translation = cv::Vec3f(0.0F, 0.0F, 0.0F);
    };

    /// Loads an OBJ mesh and auto-fits it to the marker footprint.
    ModelAsset(const std::string& objPath,
               cv::Vec3f color,
               double markerSizeMeters,
               Transform transform,
               std::string texturePath);

    std::string name() const override { return name_; }

    std::unique_ptr<Renderer> createRenderer() const override;

    /// Interleaved source data consumed by ModelRenderer.
    const std::vector<Vertex>& vertices() const { return vertices_; }

    /// Triangle index buffer.
    const std::vector<unsigned int>& indices() const { return indices_; }

    /// Diffuse RGB color, each channel in [0, 1].
    cv::Vec3f color() const { return color_; }

    /// Optional texture image loaded from config or OBJ material.
    const cv::Mat& textureImage() const { return textureImage_; }
    bool hasTexture() const { return !textureImage_.empty(); }

private:
    std::string name_;
    std::vector<Vertex> vertices_;
    std::vector<unsigned int> indices_;
    cv::Vec3f color_;
    cv::Mat textureImage_;
};
