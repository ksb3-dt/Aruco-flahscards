#include "ModelRenderer.h"

#include <stdexcept>
#include <vector>

#include <GL/glew.h>

#include "ModelAsset.h"
#include "OpenGLRenderContext.h"
#include "PoseEstimator.h"

namespace {

cv::Matx33f normalMatrixFrom(const cv::Matx44f& modelView) {
    return cv::Matx33f(
        modelView(0, 0), modelView(0, 1), modelView(0, 2),
        modelView(1, 0), modelView(1, 1), modelView(1, 2),
        modelView(2, 0), modelView(2, 1), modelView(2, 2));
}

}  // namespace

ModelRenderer::~ModelRenderer() {
    if (ebo_ != 0) {
        glDeleteBuffers(1, &ebo_);
    }
    if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
    }
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
    }
}

void ModelRenderer::draw(OpenGLRenderContext& context,
                         const Pose& pose,
                         const Asset& asset,
                         const PoseEstimator& estimator) {
    const ModelAsset* model = dynamic_cast<const ModelAsset*>(&asset);
    if (model == nullptr) {
        throw std::invalid_argument(
            "ModelRenderer: asset is not a ModelAsset");
    }

    if (uploadedAsset_ != model) {
        upload(*model);
    }

    const cv::Matx44f projection = context.projectionMatrix(estimator);
    const cv::Matx44f modelView = context.modelViewMatrix(pose);
    const cv::Matx33f normalMatrix = normalMatrixFrom(modelView);
    const cv::Vec3f color = model->color();
    const GLuint program = context.modelProgram();

    glUseProgram(program);
    glUniformMatrix4fv(glGetUniformLocation(program, "uProjection"),
                       1,
                       GL_TRUE,
                       projection.val);
    glUniformMatrix4fv(glGetUniformLocation(program, "uModelView"),
                       1,
                       GL_TRUE,
                       modelView.val);
    glUniformMatrix3fv(glGetUniformLocation(program, "uNormalMatrix"),
                       1,
                       GL_TRUE,
                       normalMatrix.val);
    glUniform3f(glGetUniformLocation(program, "uColor"),
                color[0],
                color[1],
                color[2]);

    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES,
                   static_cast<GLsizei>(indexCount_),
                   GL_UNSIGNED_INT,
                   nullptr);
    glBindVertexArray(0);
}

void ModelRenderer::upload(const ModelAsset& asset) {
    std::vector<float> interleaved;
    interleaved.reserve(asset.vertices().size() * 6);
    for (const ModelAsset::Vertex& vertex : asset.vertices()) {
        interleaved.push_back(vertex.position[0]);
        interleaved.push_back(vertex.position[1]);
        interleaved.push_back(vertex.position[2]);
        interleaved.push_back(vertex.normal[0]);
        interleaved.push_back(vertex.normal[1]);
        interleaved.push_back(vertex.normal[2]);
    }

    if (vao_ == 0) {
        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);
        glGenBuffers(1, &ebo_);
    }

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(
                     interleaved.size() * sizeof(float)),
                 interleaved.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(
                     asset.indices().size() * sizeof(unsigned int)),
                 asset.indices().data(),
                 GL_STATIC_DRAW);

    const GLsizei stride = static_cast<GLsizei>(6 * sizeof(float));
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        stride,
        reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    indexCount_ = asset.indices().size();
    uploadedAsset_ = &asset;
}
