#pragma once

class Asset;          // forward decl
class OpenGLRenderContext;  // forward decl
class PoseEstimator;  // forward decl
struct Pose;          // forward decl

/// Abstract drawing strategy. Concrete subclasses know how to render
/// a specific kind of Asset onto a frame at a given pose.
class Renderer {
public:
    virtual ~Renderer() = default;

    /// Draws `asset` into the current OpenGL frame at `pose`.
    virtual void draw(OpenGLRenderContext& context,
                      const Pose&          pose,
                      const Asset&         asset,
                      const PoseEstimator& estimator) = 0;

protected:
    Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
};
