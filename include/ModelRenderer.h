#pragma once

#include <cstddef>

#include "Renderer.h"

class ModelAsset;

/// Renders a ModelAsset as a solid shaded OpenGL mesh.
class ModelRenderer final : public Renderer {
public:
    ModelRenderer() = default;
    ~ModelRenderer() override;

    void draw(OpenGLRenderContext& context,
              const Pose&          pose,
              const Asset&         asset,
              const PoseEstimator& estimator) override;

private:
    void upload(const ModelAsset& asset);

    unsigned int vao_ = 0;
    unsigned int vbo_ = 0;
    unsigned int ebo_ = 0;
    unsigned int texture_ = 0;
    std::size_t indexCount_ = 0;
    const ModelAsset* uploadedAsset_ = nullptr;
};
