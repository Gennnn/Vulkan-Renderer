#include "CameraController.h"

#include <cmath>

bool CameraController::Initialize(const CameraControllerConfig& _config)
{
    config = _config;
    return +math.Create();
}

void CameraController::InitializeCameraView(SceneCamera& camera)
{
    GW::MATH::GVECTORF target = { 0.0f, 0.0f, 0.0f, 1.0f };
    SetLookAt(camera, camera.worldPosition, target);
    RebuildProjection(camera);
}

void CameraController::SetLookAt(SceneCamera& camera, const GW::MATH::GVECTORF& position, const GW::MATH::GVECTORF& target) {
    GW::MATH::GVECTORF up = { 0,1,0,0 };

    math.LookAtLHF(position, target, up, camera.view);
    camera.worldPosition = position;

    const float x = target.x - position.x;
    const float y = target.y - position.y;
    const float z = target.z - position.z;

    const float mag = sqrtf(x * x + y * y + z * z);
    if (mag <= 0.000001f) {
        accumulatedPitch = 0.0f;
        return;
    }

    const float nx = x / mag;
    const float ny = y / mag;
    const float nz = z / mag;

    const float flatMag = sqrt(nx * nx + nz * nz);
    accumulatedPitch = flatMag < 0.0001f ? 0.0f : -atan2f(ny, flatMag);
}

void CameraController::RebuildProjection(SceneCamera& camera) {
    const float fov = G_DEGREE_TO_RADIAN(camera.fovDegrees);
    math.ProjectionVulkanLHF(fov, camera.aspectRatio, camera.farPlane, camera.nearPlane, camera.projection);
}

void CameraController::Update(SceneCamera& camera, InputSystem& input, float dt) {
    if (!input.IsWindowFocused()) return;

#if defined(_WIN32)
    if (!input.IsCursorCaptured() && input.GetMouseButtonState(G_BUTTON_LEFT) > 0.0f) {
        input.CaptureCursor();
        input.CenterCursor();
        return;
    }
    if (input.IsCursorCaptured() && input.GetKeyState(G_KEY_ESCAPE) > 0.0f) {
        input.ReleaseCursor();
        return;
    }

    if (input.IsCursorCaptured()) {
        input.CenterCursor();
    }
#endif

    if (!input.IsCursorCaptured()) return;

    GW::MATH::GMATRIXF cameraWorld = {};
    math.InverseF(camera.view, cameraWorld);
    const float up = input.GetKeyState(G_KEY_SPACE) - input.GetKeyState(G_KEY_LEFTSHIFT) + input.GetControllerState(0, G_RIGHT_TRIGGER_AXIS) - input.GetControllerState(0, G_LEFT_TRIGGER_AXIS);
    const float right = input.GetKeyState(G_KEY_D) - input.GetKeyState(G_KEY_A) + input.GetControllerState(0, G_LX_AXIS);
    const float fwd = input.GetKeyState(G_KEY_W) - input.GetKeyState(G_KEY_S) + input.GetControllerState(0, G_LY_AXIS);

    cameraWorld.row4.y += up * config.moveSpeed * dt;

    GW::MATH::GVECTORF translate = { right * config.moveSpeed * dt, 0.0f, fwd * config.moveSpeed * dt, 0.0f };
    math.TranslateLocalF(cameraWorld, translate, cameraWorld);

    float mouseX = 0.0f;
    float mouseY = 0.0f;

    if (!input.GetMouseDelta(mouseX, mouseY)) {
        mouseX = 0.0f;
        mouseY = 0.0f;
    }

    unsigned int width = 1;
    unsigned int height = 1;
    input.GetClientSize(width, height);

    if (width == 0) width = 1;
    if (height == 0) height = 1;

    camera.aspectRatio = static_cast<float>(width) / static_cast<float>(height);

    const float controllerPitch = input.GetControllerState(0, G_RY_AXIS);
    const float controllerYaw = input.GetControllerState(0, G_RX_AXIS);

    const float thumbSpeed = G_PI_F * dt;

    const float pitchDelta = (dt * config.lookSensitivity * G_DEGREE_TO_RADIAN(camera.fovDegrees) * mouseY / height) + controllerPitch * -thumbSpeed;
    float newPitch = accumulatedPitch + pitchDelta;

    const float pitchLimit = G_PI_F * 0.5f;
    if (newPitch > pitchLimit) newPitch = pitchLimit;
    if (newPitch < -pitchLimit) newPitch = -pitchLimit;

    const float totalPitch = newPitch - accumulatedPitch;
    accumulatedPitch = newPitch;

    GW::MATH::GMATRIXF pitchMatrix;
    math.RotateXLocalF(GW::MATH::GIdentityMatrixF, totalPitch, pitchMatrix);
    math.MultiplyMatrixF(pitchMatrix, cameraWorld, cameraWorld);

    const float aspectRatio = camera.aspectRatio;
    const float totalYaw = (dt * config.lookSensitivity * G_DEGREE_TO_RADIAN_F(camera.fovDegrees) * aspectRatio * mouseX / width) + controllerYaw * thumbSpeed;

    GW::MATH::GMATRIXF yawMatrix;
    GW::MATH::GVECTORF position = cameraWorld.row4;

    math.RotateYGlobalF(GW::MATH::GIdentityMatrixF, totalYaw, yawMatrix);
    math.MultiplyMatrixF(cameraWorld, yawMatrix, cameraWorld);

    cameraWorld.row4 = position;

    camera.worldPosition = { position.x, position.y, position.z, 1.0f };
    math.InverseF(cameraWorld, camera.view);

    RebuildProjection(camera);
}