#include "ModelAsset.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "ModelRenderer.h"

namespace {

constexpr float kDegreesToRadians = static_cast<float>(CV_PI / 180.0);

cv::Vec3f normalize(const cv::Vec3f& value) {
    const float length = std::sqrt(value.dot(value));
    if (length <= std::numeric_limits<float>::epsilon()) {
        return cv::Vec3f(0.0F, 0.0F, 1.0F);
    }
    return value * (1.0F / length);
}

ModelAsset::Vertex convertVertex(const aiMesh& mesh, unsigned int index) {
    const aiVector3D& position = mesh.mVertices[index];
    const cv::Vec3f markerPosition(position.x, -position.z, position.y);

    cv::Vec3f markerNormal(0.0F, 0.0F, 1.0F);
    if (mesh.HasNormals()) {
        const aiVector3D& normal = mesh.mNormals[index];
        markerNormal = normalize(cv::Vec3f(normal.x, -normal.z, normal.y));
    }

    return ModelAsset::Vertex{markerPosition, markerNormal};
}

void normalizeToMarker(std::vector<ModelAsset::Vertex>& vertices,
                       double markerSizeMeters) {
    if (vertices.empty()) {
        throw std::runtime_error("ModelAsset: OBJ has no vertices");
    }

    cv::Vec3f minimum(std::numeric_limits<float>::max(),
                      std::numeric_limits<float>::max(),
                      std::numeric_limits<float>::max());
    cv::Vec3f maximum(std::numeric_limits<float>::lowest(),
                      std::numeric_limits<float>::lowest(),
                      std::numeric_limits<float>::lowest());

    for (const ModelAsset::Vertex& vertex : vertices) {
        for (int axis = 0; axis < 3; ++axis) {
            minimum[axis] = std::min(minimum[axis], vertex.position[axis]);
            maximum[axis] = std::max(maximum[axis], vertex.position[axis]);
        }
    }

    const float footprintX = maximum[0] - minimum[0];
    const float footprintY = maximum[1] - minimum[1];
    const float footprint = std::max(footprintX, footprintY);
    if (footprint <= std::numeric_limits<float>::epsilon()) {
        throw std::runtime_error(
            "ModelAsset: OBJ footprint is too small to auto-fit");
    }

    const float centerX = (minimum[0] + maximum[0]) * 0.5F;
    const float centerY = (minimum[1] + maximum[1]) * 0.5F;
    const float markerSize = static_cast<float>(markerSizeMeters);
    const float scale = (markerSize * 0.8F) / footprint;

    for (ModelAsset::Vertex& vertex : vertices) {
        vertex.position[0] = (vertex.position[0] - centerX) * scale;
        vertex.position[1] = (vertex.position[1] - centerY) * scale;
        vertex.position[2] = (vertex.position[2] - minimum[2]) * scale;
    }
}

cv::Matx33f rotationMatrix(const cv::Vec3f& degrees) {
    const float rx = degrees[0] * kDegreesToRadians;
    const float ry = degrees[1] * kDegreesToRadians;
    const float rz = degrees[2] * kDegreesToRadians;

    const float cx = std::cos(rx);
    const float sx = std::sin(rx);
    const float cy = std::cos(ry);
    const float sy = std::sin(ry);
    const float cz = std::cos(rz);
    const float sz = std::sin(rz);

    const cv::Matx33f rotateX(
        1.0F, 0.0F, 0.0F,
        0.0F, cx, -sx,
        0.0F, sx, cx);
    const cv::Matx33f rotateY(
        cy, 0.0F, sy,
        0.0F, 1.0F, 0.0F,
        -sy, 0.0F, cy);
    const cv::Matx33f rotateZ(
        cz, -sz, 0.0F,
        sz, cz, 0.0F,
        0.0F, 0.0F, 1.0F);

    return rotateZ * rotateY * rotateX;
}

void applyTransform(std::vector<ModelAsset::Vertex>& vertices,
                    const ModelAsset::Transform& transform) {
    if (transform.scale <= 0.0F) {
        throw std::invalid_argument("ModelAsset: scale must be positive");
    }

    const cv::Matx33f rotation = rotationMatrix(transform.rotationDegrees);
    for (ModelAsset::Vertex& vertex : vertices) {
        vertex.position =
            rotation * (vertex.position * transform.scale) +
            transform.translation;
        vertex.normal = normalize(rotation * vertex.normal);
    }
}

}  // namespace

ModelAsset::ModelAsset(const std::string& objPath,
                       cv::Vec3f color,
                       double markerSizeMeters,
                       Transform transform)
    : name_(std::filesystem::path(objPath).filename().string()),
      color_(color) {
    if (markerSizeMeters <= 0.0) {
        throw std::invalid_argument("ModelAsset: marker size must be positive");
    }

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        objPath,
        aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_JoinIdenticalVertices |
            aiProcess_ImproveCacheLocality);

    if (scene == nullptr || scene->mNumMeshes == 0) {
        throw std::runtime_error(
            "ModelAsset: failed to load OBJ '" + objPath + "': " +
            importer.GetErrorString());
    }

    for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes;
         ++meshIndex) {
        const aiMesh* mesh = scene->mMeshes[meshIndex];
        if (mesh == nullptr) {
            continue;
        }
        if (vertices_.size() >
            static_cast<std::size_t>(
                std::numeric_limits<unsigned int>::max() -
                mesh->mNumVertices)) {
            throw std::runtime_error(
                "ModelAsset: OBJ has too many vertices for 32-bit indices");
        }

        const unsigned int baseIndex =
            static_cast<unsigned int>(vertices_.size());
        for (unsigned int vertexIndex = 0; vertexIndex < mesh->mNumVertices;
             ++vertexIndex) {
            vertices_.push_back(convertVertex(*mesh, vertexIndex));
        }

        for (unsigned int faceIndex = 0; faceIndex < mesh->mNumFaces;
             ++faceIndex) {
            const aiFace& face = mesh->mFaces[faceIndex];
            if (face.mNumIndices != 3) {
                continue;
            }
            indices_.push_back(baseIndex + face.mIndices[0]);
            indices_.push_back(baseIndex + face.mIndices[1]);
            indices_.push_back(baseIndex + face.mIndices[2]);
        }
    }

    if (indices_.empty()) {
        throw std::runtime_error("ModelAsset: OBJ has no triangle faces");
    }

    normalizeToMarker(vertices_, markerSizeMeters);
    applyTransform(vertices_, transform);
}

std::unique_ptr<Renderer> ModelAsset::createRenderer() const {
    return std::make_unique<ModelRenderer>();
}
