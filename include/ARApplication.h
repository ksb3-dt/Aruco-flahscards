#pragma once

#include <memory>
#include <string>

#include "AssetManager.h"
#include "Camera.h"
#include "MarkerDetector.h"
#include "PoseEstimator.h"

class OpenGLRenderContext;

/// Top-level orchestrator. Composes the camera, detector, pose estimator
/// and asset manager into the per-frame pipeline:
///
///   Camera -> MarkerDetector -> PoseEstimator -> AssetManager -> Renderer.
///
/// Owns all subsystems. Lifetime of subsystems = lifetime of ARApplication.
class ARApplication {
public:
    struct Config {
        std::string  cameraUri;             ///< e.g. "http://192.168.0.42:8080/video"
        std::string  calibrationFile;       ///< Path to calibration YAML (empty -> approximate intrinsics).
        std::string  assetsConfig;          ///< Path to assets.json.
        double       markerSizeMeters = 0.05;
        bool         showFps = true;
        std::string  windowTitle = "AR Flashcards";
    };

    explicit ARApplication(Config config);
    ~ARApplication();

    /// Runs the main loop until the user presses ESC. Throws on
    /// unrecoverable setup failure (e.g. camera cannot be opened).
    void run();

private:
    /// Processes one frame: detect markers, estimate pose, draw assets.
    void processFrame(cv::Mat& frame);

    Config                         config_;
    std::unique_ptr<Camera>        camera_;
    MarkerDetector                 detector_;
    std::unique_ptr<PoseEstimator> poseEstimator_;  // built lazily once we know the frame size
    std::unique_ptr<OpenGLRenderContext> renderContext_;
    AssetManager                   assetManager_;
};
