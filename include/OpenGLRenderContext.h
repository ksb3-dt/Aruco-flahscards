#pragma once

#include <string>

#include <opencv2/core.hpp>

class PoseEstimator;
struct Pose;
struct SDL_Window;

/// Owns the SDL window, OpenGL context, shared shaders, and camera background
/// texture used by all OpenGL renderers.
class OpenGLRenderContext {
public:
    /// Creates an SDL2 window and initializes GLEW/OpenGL resources.
    OpenGLRenderContext(const std::string& title, cv::Size frameSize);
    ~OpenGLRenderContext();

    OpenGLRenderContext(const OpenGLRenderContext&) = delete;
    OpenGLRenderContext& operator=(const OpenGLRenderContext&) = delete;

    /// Returns true when the user requested application shutdown.
    bool pollQuit();

    /// Starts a frame by drawing the camera image as the OpenGL background.
    void beginFrame(const cv::Mat& frame);

    /// Presents the current OpenGL back buffer.
    void endFrame();

    /// Shared shader program for solid model rendering.
    unsigned int modelProgram() const { return modelProgram_; }

    /// Builds an OpenGL projection matrix from OpenCV camera intrinsics.
    cv::Matx44f projectionMatrix(const PoseEstimator& estimator,
                                 float nearPlane = 0.01F,
                                 float farPlane = 100.0F) const;

    /// Converts an OpenCV marker pose into an OpenGL model-view matrix.
    cv::Matx44f modelViewMatrix(const Pose& pose) const;

    /// Updates the SDL window title.
    void setWindowTitle(const std::string& title);

    cv::Size frameSize() const { return frameSize_; }

private:
    void initializeBackgroundResources();
    void initializeModelResources();
    unsigned int compileProgram(const char* vertexSource,
                                const char* fragmentSource) const;
    void uploadBackgroundTexture(const cv::Mat& frame);

    SDL_Window* window_ = nullptr;
    void* glContext_ = nullptr;
    cv::Size frameSize_;
    cv::Mat uploadFrame_;

    unsigned int backgroundProgram_ = 0;
    unsigned int backgroundTexture_ = 0;
    unsigned int backgroundVao_ = 0;
    unsigned int backgroundVbo_ = 0;
    unsigned int modelProgram_ = 0;
};
