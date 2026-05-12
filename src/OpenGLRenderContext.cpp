#include "OpenGLRenderContext.h"

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <GL/glew.h>
#include <SDL.h>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>

#include "PoseEstimator.h"

namespace {

const char* kBackgroundVertexShader = R"glsl(
#version 330 core
layout (location = 0) in vec2 aPosition;
layout (location = 1) in vec2 aTexCoord;

out vec2 vTexCoord;

void main() {
    vTexCoord = aTexCoord;
    gl_Position = vec4(aPosition, 0.0, 1.0);
}
)glsl";

const char* kBackgroundFragmentShader = R"glsl(
#version 330 core
in vec2 vTexCoord;

out vec4 fragColor;

uniform sampler2D uFrame;

void main() {
    fragColor = texture(uFrame, vTexCoord);
}
)glsl";

const char* kModelVertexShader = R"glsl(
#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 vNormal;
out vec2 vTexCoord;

uniform mat4 uProjection;
uniform mat4 uModelView;
uniform mat3 uNormalMatrix;

void main() {
    vec4 eyePosition = uModelView * vec4(aPosition, 1.0);
    vNormal = normalize(uNormalMatrix * aNormal);
    vTexCoord = aTexCoord;
    gl_Position = uProjection * eyePosition;
}
)glsl";

const char* kModelFragmentShader = R"glsl(
#version 330 core
in vec3 vNormal;
in vec2 vTexCoord;

out vec4 fragColor;

uniform vec3 uColor;
uniform sampler2D uTexture;
uniform bool uUseTexture;

void main() {
    vec3 lightDirection = normalize(vec3(0.3, 0.6, 1.0));
    float diffuse = max(dot(normalize(vNormal), lightDirection), 0.0);
    vec3 baseColor = uUseTexture ? texture(uTexture, vTexCoord).rgb : uColor;
    vec3 color = baseColor * (0.25 + 0.75 * diffuse);
    fragColor = vec4(color, 1.0);
}
)glsl";

std::string shaderLog(GLuint shader) {
    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    if (length <= 1) {
        return {};
    }

    std::vector<char> log(static_cast<std::size_t>(length));
    glGetShaderInfoLog(shader, length, nullptr, log.data());
    return std::string(log.data());
}

std::string programLog(GLuint program) {
    GLint length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    if (length <= 1) {
        return {};
    }

    std::vector<char> log(static_cast<std::size_t>(length));
    glGetProgramInfoLog(program, length, nullptr, log.data());
    return std::string(log.data());
}

void throwSdlError(const std::string& prefix) {
    throw std::runtime_error(prefix + ": " + SDL_GetError());
}

}  // namespace

OpenGLRenderContext::OpenGLRenderContext(const std::string& title,
                                         cv::Size frameSize)
    : frameSize_(frameSize) {
    if (frameSize_.width <= 0 || frameSize_.height <= 0) {
        throw std::invalid_argument(
            "OpenGLRenderContext: frame size must be positive");
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        throwSdlError("OpenGLRenderContext: SDL_Init failed");
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    window_ = SDL_CreateWindow(title.c_str(),
                               SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED,
                               frameSize_.width,
                               frameSize_.height,
                               SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (window_ == nullptr) {
        throwSdlError("OpenGLRenderContext: SDL_CreateWindow failed");
    }

    glContext_ = SDL_GL_CreateContext(window_);
    if (glContext_ == nullptr) {
        throwSdlError("OpenGLRenderContext: SDL_GL_CreateContext failed");
    }

    SDL_GL_MakeCurrent(window_, glContext_);
    SDL_GL_SetSwapInterval(1);

    glewExperimental = GL_TRUE;
    const GLenum glewStatus = glewInit();
    glGetError();  // GLEW may leave a harmless GL_INVALID_ENUM behind.
    if (glewStatus != GLEW_OK) {
        throw std::runtime_error(
            "OpenGLRenderContext: GLEW init failed: " +
            std::string(reinterpret_cast<const char*>(
                glewGetErrorString(glewStatus))));
    }

    glViewport(0, 0, frameSize_.width, frameSize_.height);
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    initializeBackgroundResources();
    initializeModelResources();
}

OpenGLRenderContext::~OpenGLRenderContext() {
    if (modelProgram_ != 0) {
        glDeleteProgram(modelProgram_);
    }
    if (backgroundTexture_ != 0) {
        glDeleteTextures(1, &backgroundTexture_);
    }
    if (backgroundVbo_ != 0) {
        glDeleteBuffers(1, &backgroundVbo_);
    }
    if (backgroundVao_ != 0) {
        glDeleteVertexArrays(1, &backgroundVao_);
    }
    if (backgroundProgram_ != 0) {
        glDeleteProgram(backgroundProgram_);
    }
    if (glContext_ != nullptr) {
        SDL_GL_DeleteContext(glContext_);
    }
    if (window_ != nullptr) {
        SDL_DestroyWindow(window_);
    }
    SDL_Quit();
}

bool OpenGLRenderContext::pollQuit() {
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        if (event.type == SDL_QUIT) {
            return true;
        }
        if (event.type == SDL_KEYDOWN &&
            event.key.keysym.sym == SDLK_ESCAPE) {
            return true;
        }
    }
    return false;
}

void OpenGLRenderContext::beginFrame(const cv::Mat& frame) {
    uploadBackgroundTexture(frame);

    glViewport(0, 0, frameSize_.width, frameSize_.height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glUseProgram(backgroundProgram_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, backgroundTexture_);
    glUniform1i(glGetUniformLocation(backgroundProgram_, "uFrame"), 0);
    glBindVertexArray(backgroundVao_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void OpenGLRenderContext::endFrame() {
    SDL_GL_SwapWindow(window_);
}

cv::Matx44f OpenGLRenderContext::projectionMatrix(
    const PoseEstimator& estimator,
    float nearPlane,
    float farPlane) const {
    const cv::Mat& cameraMatrix = estimator.cameraMatrix();
    const float fx = static_cast<float>(cameraMatrix.at<double>(0, 0));
    const float fy = static_cast<float>(cameraMatrix.at<double>(1, 1));
    const float cx = static_cast<float>(cameraMatrix.at<double>(0, 2));
    const float cy = static_cast<float>(cameraMatrix.at<double>(1, 2));
    const float width = static_cast<float>(frameSize_.width);
    const float height = static_cast<float>(frameSize_.height);

    return cv::Matx44f(
        (2.0F * fx) / width, 0.0F, 1.0F - (2.0F * cx) / width, 0.0F,
        0.0F, (2.0F * fy) / height, (2.0F * cy) / height - 1.0F, 0.0F,
        0.0F, 0.0F, -(farPlane + nearPlane) / (farPlane - nearPlane),
        -(2.0F * farPlane * nearPlane) / (farPlane - nearPlane),
        0.0F, 0.0F, -1.0F, 0.0F);
}

cv::Matx44f OpenGLRenderContext::modelViewMatrix(const Pose& pose) const {
    cv::Mat rotation;
    cv::Rodrigues(pose.rvec, rotation);

    cv::Matx44f openCvPose = cv::Matx44f::eye();
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            openCvPose(row, col) =
                static_cast<float>(rotation.at<double>(row, col));
        }
        openCvPose(row, 3) = static_cast<float>(pose.tvec[row]);
    }

    const cv::Matx44f cvToGl(
        1.0F, 0.0F, 0.0F, 0.0F,
        0.0F, -1.0F, 0.0F, 0.0F,
        0.0F, 0.0F, -1.0F, 0.0F,
        0.0F, 0.0F, 0.0F, 1.0F);
    return cvToGl * openCvPose;
}

void OpenGLRenderContext::setWindowTitle(const std::string& title) {
    SDL_SetWindowTitle(window_, title.c_str());
}

void OpenGLRenderContext::initializeBackgroundResources() {
    backgroundProgram_ =
        compileProgram(kBackgroundVertexShader, kBackgroundFragmentShader);

    const float vertices[] = {
        -1.0F, -1.0F, 0.0F, 1.0F,
         1.0F, -1.0F, 1.0F, 1.0F,
         1.0F,  1.0F, 1.0F, 0.0F,
        -1.0F, -1.0F, 0.0F, 1.0F,
         1.0F,  1.0F, 1.0F, 0.0F,
        -1.0F,  1.0F, 0.0F, 0.0F,
    };

    glGenVertexArrays(1, &backgroundVao_);
    glGenBuffers(1, &backgroundVbo_);

    glBindVertexArray(backgroundVao_);
    glBindBuffer(GL_ARRAY_BUFFER, backgroundVbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(sizeof(vertices)),
                 vertices,
                 GL_STATIC_DRAW);

    const GLsizei stride = static_cast<GLsizei>(4 * sizeof(float));
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        stride,
        reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    glGenTextures(1, &backgroundTexture_);
    glBindTexture(GL_TEXTURE_2D, backgroundTexture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void OpenGLRenderContext::initializeModelResources() {
    modelProgram_ = compileProgram(kModelVertexShader, kModelFragmentShader);
}

unsigned int OpenGLRenderContext::compileProgram(
    const char* vertexSource,
    const char* fragmentSource) const {
    const GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, nullptr);
    glCompileShader(vertexShader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        const std::string log = shaderLog(vertexShader);
        glDeleteShader(vertexShader);
        throw std::runtime_error(
            "OpenGLRenderContext: vertex shader compile failed: " + log);
    }

    const GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        const std::string log = shaderLog(fragmentShader);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        throw std::runtime_error(
            "OpenGLRenderContext: fragment shader compile failed: " + log);
    }

    const GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (linked != GL_TRUE) {
        const std::string log = programLog(program);
        glDeleteProgram(program);
        throw std::runtime_error(
            "OpenGLRenderContext: shader link failed: " + log);
    }

    return program;
}

void OpenGLRenderContext::uploadBackgroundTexture(const cv::Mat& frame) {
    if (frame.empty()) {
        throw std::invalid_argument(
            "OpenGLRenderContext: cannot upload an empty frame");
    }
    if (frame.size() != frameSize_) {
        frameSize_ = frame.size();
        SDL_SetWindowSize(window_, frameSize_.width, frameSize_.height);
    }

    GLenum format = GL_RGB;
    if (frame.channels() == 3) {
        cv::cvtColor(frame, uploadFrame_, cv::COLOR_BGR2RGB);
        format = GL_RGB;
    } else if (frame.channels() == 4) {
        cv::cvtColor(frame, uploadFrame_, cv::COLOR_BGRA2RGBA);
        format = GL_RGBA;
    } else if (frame.channels() == 1) {
        cv::cvtColor(frame, uploadFrame_, cv::COLOR_GRAY2RGB);
        format = GL_RGB;
    } else {
        throw std::invalid_argument(
            "OpenGLRenderContext: unsupported frame channel count");
    }

    const cv::Mat contiguous =
        uploadFrame_.isContinuous() ? uploadFrame_ : uploadFrame_.clone();

    glBindTexture(GL_TEXTURE_2D, backgroundTexture_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 static_cast<GLint>(format),
                 frameSize_.width,
                 frameSize_.height,
                 0,
                 format,
                 GL_UNSIGNED_BYTE,
                 contiguous.data);
}
