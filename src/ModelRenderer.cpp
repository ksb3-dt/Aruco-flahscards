#include "ModelRenderer.h"

#include <stdexcept>
#include <vector>

#include <GL/glew.h>
#include <opencv2/imgproc.hpp>

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
    if (texture_ != 0) {
        glDeleteTextures(1, &texture_);
    }
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
    glUniform1i(glGetUniformLocation(program, "uUseTexture"),
                model->hasTexture() ? GL_TRUE : GL_FALSE);
    if (model->hasTexture()) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture_);
        glUniform1i(glGetUniformLocation(program, "uTexture"), 1);
    }

    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES,
                   static_cast<GLsizei>(indexCount_),
                   GL_UNSIGNED_INT,
                   nullptr);
    glBindVertexArray(0);
}

void ModelRenderer::upload(const ModelAsset& asset) {
    std::vector<float> interleaved;
    interleaved.reserve(asset.vertices().size() * 8);
    for (const ModelAsset::Vertex& vertex : asset.vertices()) {
        interleaved.push_back(vertex.position[0]);
        interleaved.push_back(vertex.position[1]);
        interleaved.push_back(vertex.position[2]);
        interleaved.push_back(vertex.normal[0]);
        interleaved.push_back(vertex.normal[1]);
        interleaved.push_back(vertex.normal[2]);
        interleaved.push_back(vertex.texCoord[0]);
        interleaved.push_back(1.0F - vertex.texCoord[1]);
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

    const GLsizei stride = static_cast<GLsizei>(8 * sizeof(float));
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
    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        stride,
        reinterpret_cast<void*>(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    if (texture_ != 0) {
        glDeleteTextures(1, &texture_);
        texture_ = 0;
    }
    if (asset.hasTexture()) {
        cv::Mat uploadImage;
        GLenum format = GL_RGB;
        const cv::Mat& source = asset.textureImage();
        if (source.channels() == 3) {
            cv::cvtColor(source, uploadImage, cv::COLOR_BGR2RGB);
            format = GL_RGB;
        } else if (source.channels() == 4) {
            cv::cvtColor(source, uploadImage, cv::COLOR_BGRA2RGBA);
            format = GL_RGBA;
        } else if (source.channels() == 1) {
            cv::cvtColor(source, uploadImage, cv::COLOR_GRAY2RGB);
            format = GL_RGB;
        } else {
            throw std::runtime_error(
                "ModelRenderer: unsupported texture channel count");
        }

        const cv::Mat contiguous =
            uploadImage.isContinuous() ? uploadImage : uploadImage.clone();

        glGenTextures(1, &texture_);
        glBindTexture(GL_TEXTURE_2D, texture_);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     static_cast<GLint>(format),
                     contiguous.cols,
                     contiguous.rows,
                     0,
                     format,
                     GL_UNSIGNED_BYTE,
                     contiguous.data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    indexCount_ = asset.indices().size();
    uploadedAsset_ = &asset;
}
