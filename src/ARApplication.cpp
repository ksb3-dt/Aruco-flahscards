#include "ARApplication.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

#include <opencv2/aruco.hpp>

#include "OpenGLRenderContext.h"

namespace {

cv::Mat firstUsableFrame(Camera& camera) {
    for (int attempt = 0; attempt < 100; ++attempt) {
        cv::Mat frame = camera.nextFrame();
        if (!frame.empty()) {
            return frame;
        }
    }
    throw std::runtime_error("ARApplication: camera produced no frames");
}

}  // namespace

ARApplication::ARApplication(Config config)
    : config_(std::move(config)),
      assetManager_(config_.markerSizeMeters) {}

ARApplication::~ARApplication() = default;

void ARApplication::run() {
    camera_ = Camera::create(config_.cameraUri);

    cv::Mat pendingFrame;
    cv::Size frameSize = camera_->resolution();
    if (frameSize.width <= 0 || frameSize.height <= 0) {
        pendingFrame = firstUsableFrame(*camera_);
        frameSize = pendingFrame.size();
    }

    if (config_.calibrationFile.empty()) {
        poseEstimator_ =
            std::make_unique<PoseEstimator>(frameSize,
                                            config_.markerSizeMeters);
    } else {
        poseEstimator_ =
            std::make_unique<PoseEstimator>(config_.calibrationFile,
                                            config_.markerSizeMeters);
    }

    if (!config_.assetsConfig.empty()) {
        assetManager_.loadFromConfig(config_.assetsConfig);
    }

    renderContext_ =
        std::make_unique<OpenGLRenderContext>(config_.windowTitle, frameSize);

    std::cout << "Camera opened: " << config_.cameraUri << "\n"
              << "Resolution:    " << frameSize << "\n"
              << "Assets:        " << assetManager_.size() << "\n"
              << "Press ESC in the OpenGL window to quit.\n";

    std::size_t framesSinceTitle = 0;
    auto lastTitleUpdate = std::chrono::steady_clock::now();

    while (!renderContext_->pollQuit()) {
        cv::Mat frame;
        if (!pendingFrame.empty()) {
            frame = pendingFrame;
            pendingFrame.release();
        } else {
            frame = camera_->nextFrame();
        }

        if (frame.empty()) {
            std::cerr << "Empty frame, retrying...\n";
            continue;
        }

        processFrame(frame);

        if (config_.showFps) {
            ++framesSinceTitle;
            const auto now = std::chrono::steady_clock::now();
            const double seconds =
                std::chrono::duration<double>(now - lastTitleUpdate).count();
            if (seconds >= 1.0) {
                const double fps =
                    static_cast<double>(framesSinceTitle) / seconds;
                std::ostringstream title;
                title << config_.windowTitle << " - "
                      << std::fixed << std::setprecision(1)
                      << fps << " FPS";
                renderContext_->setWindowTitle(title.str());
                framesSinceTitle = 0;
                lastTitleUpdate = now;
            }
        }
    }
}

void ARApplication::processFrame(cv::Mat& frame) {
    const std::vector<DetectedMarker> markers = detector_.detect(frame);
    if (!markers.empty()) {
        std::vector<std::vector<cv::Point2f>> corners;
        std::vector<int> ids;
        corners.reserve(markers.size());
        ids.reserve(markers.size());
        for (const DetectedMarker& marker : markers) {
            corners.push_back(marker.corners);
            ids.push_back(marker.id);
        }
        cv::aruco::drawDetectedMarkers(frame, corners, ids);
    }

    renderContext_->beginFrame(frame);

    for (const DetectedMarker& marker : markers) {
        const Asset* asset = assetManager_.findAsset(marker.id);
        Renderer* renderer = assetManager_.findRenderer(marker.id);
        if (asset == nullptr || renderer == nullptr) {
            continue;
        }

        const Pose pose = poseEstimator_->estimate(marker);
        renderer->draw(*renderContext_, pose, *asset, *poseEstimator_);
    }

    renderContext_->endFrame();
}
