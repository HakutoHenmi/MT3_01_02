#define _USE_MATH_DEFINES
#include <Novice.h>
#include <cmath>
#include <imgui.h>
#include <math.h>

const char kWindowTitle[] = "LE2B_20_ヘンミ_ハクト";

struct Vector3 {
    float x, y, z;
};

struct Matrix4x4 {
    float m[4][4];
};

struct Sphere {
    Vector3 center;
    float radius;
};

Matrix4x4 MakeIdentityMatrix()
{
    Matrix4x4 result = {};
    for (int i = 0; i < 4; i++) {
        result.m[i][i] = 1.0f;
    }
    return result;
}

Matrix4x4 MakeTranslateMatrix(Vector3 t)
{
    Matrix4x4 result = MakeIdentityMatrix();
    result.m[3][0] = t.x;
    result.m[3][1] = t.y;
    result.m[3][2] = t.z;
    return result;
}

Matrix4x4 MakeRotateX(float angle)
{
    Matrix4x4 result = MakeIdentityMatrix();
    result.m[1][1] = cosf(angle);
    result.m[1][2] = sinf(angle);
    result.m[2][1] = -sinf(angle);
    result.m[2][2] = cosf(angle);
    return result;
}

Matrix4x4 MakeRotateY(float angle)
{
    Matrix4x4 result = MakeIdentityMatrix();
    result.m[0][0] = cosf(angle);
    result.m[0][2] = -sinf(angle);
    result.m[2][0] = sinf(angle);
    result.m[2][2] = cosf(angle);
    return result;
}

Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2)
{
    Matrix4x4 result = {};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                result.m[i][j] += m1.m[i][k] * m2.m[k][j];
            }
        }
    }
    return result;
}

Vector3 Transform(const Vector3& v, const Matrix4x4& m)
{
    Vector3 result;
    float w;
    result.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0];
    result.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1];
    result.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2];
    w = v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + m.m[3][3];
    result.x /= w;
    result.y /= w;
    result.z /= w;
    return result;
}

Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspect, float nearZ, float farZ)
{
    Matrix4x4 result = {};
    float f = 1.0f / tanf(fovY / 2.0f);
    result.m[0][0] = f / aspect;
    result.m[1][1] = f;
    result.m[2][2] = farZ / (nearZ - farZ);
    result.m[2][3] = -1.0f;
    result.m[3][2] = (nearZ * farZ) / (nearZ - farZ);
    return result;
}

Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth)
{
    Matrix4x4 result = {};
    result.m[0][0] = width / 2.0f;
    result.m[1][1] = height / 2.0f; // ← 修正箇所: Y軸反転削除
    result.m[2][2] = maxDepth - minDepth;
    result.m[3][0] = left + width / 2.0f;
    result.m[3][1] = top + height / 2.0f;
    result.m[3][2] = minDepth;
    result.m[3][3] = 1.0f;
    return result;
}

void DrawGrid(const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix)
{
    const float kGridHalfWidth = 2.0f;
    const uint32_t kSubdivision = 10;
    const float kGridEvery = (kGridHalfWidth * 2.0f) / kSubdivision;
    for (uint32_t x = 0; x <= kSubdivision; ++x) {
        float offset = -kGridHalfWidth + x * kGridEvery;
        Vector3 start = { offset, 0.0f, -kGridHalfWidth };
        Vector3 end = { offset, 0.0f, +kGridHalfWidth };
        start = Transform(Transform(start, viewProjectionMatrix), viewportMatrix);
        end = Transform(Transform(end, viewProjectionMatrix), viewportMatrix);
        Novice::DrawLine((int)start.x, (int)start.y, (int)end.x, (int)end.y, 0xAAAAAAFF);
    }
    for (uint32_t z = 0; z <= kSubdivision; ++z) {
        float offset = -kGridHalfWidth + z * kGridEvery;
        Vector3 start = { -kGridHalfWidth, 0.0f, offset };
        Vector3 end = { +kGridHalfWidth, 0.0f, offset };
        start = Transform(Transform(start, viewProjectionMatrix), viewportMatrix);
        end = Transform(Transform(end, viewProjectionMatrix), viewportMatrix);
        Novice::DrawLine((int)start.x, (int)start.y, (int)end.x, (int)end.y, 0xAAAAAAFF);
    }
}

void DrawSphere(const Sphere& sphere, const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, uint32_t color)
{
    const uint32_t kSubdivision = 16;
    const float kLatEvery = float(M_PI) / kSubdivision;
    const float kLonEvery = float(M_PI * 2.0f) / kSubdivision;

    for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
        float lat = -float(M_PI) / 2.0f + kLatEvery * latIndex;
        for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
            float lon = kLonEvery * lonIndex;
            Vector3 a = {
                sphere.radius * cosf(lat) * cosf(lon) + sphere.center.x,
                sphere.radius * sinf(lat) + sphere.center.y,
                sphere.radius * cosf(lat) * sinf(lon) + sphere.center.z
            };
            Vector3 b = {
                sphere.radius * cosf(lat + kLatEvery) * cosf(lon) + sphere.center.x,
                sphere.radius * sinf(lat + kLatEvery) + sphere.center.y,
                sphere.radius * cosf(lat + kLatEvery) * sinf(lon) + sphere.center.z
            };
            Vector3 c = {
                sphere.radius * cosf(lat) * cosf(lon + kLonEvery) + sphere.center.x,
                sphere.radius * sinf(lat) + sphere.center.y,
                sphere.radius * cosf(lat) * sinf(lon + kLonEvery) + sphere.center.z
            };
            a = Transform(Transform(a, viewProjectionMatrix), viewportMatrix);
            b = Transform(Transform(b, viewProjectionMatrix), viewportMatrix);
            c = Transform(Transform(c, viewProjectionMatrix), viewportMatrix);
            Novice::DrawLine((int)a.x, (int)a.y, (int)b.x, (int)b.y, color);
            Novice::DrawLine((int)a.x, (int)a.y, (int)c.x, (int)c.y, color);
        }
    }
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    Novice::Initialize(kWindowTitle, 1280, 720);
    char keys[256] = {};
    char preKeys[256] = {};

    Vector3 cameraTranslate = { 0.0f, 1.9f, -6.49f };
    Vector3 cameraRotate = { 0.0f, 0.0f, 0.0f };
    Sphere sphere = { { 0.0f, 1.0f, 0.0f }, 1.0f };

    while (Novice::ProcessMessage() == 0) {
        Novice::BeginFrame();

        memcpy(preKeys, keys, 256);
        Novice::GetHitKeyStateAll(keys);

        ImGui::Begin("window");
        ImGui::DragFloat3("CameraTranslate", &cameraTranslate.x, 0.01f);
        ImGui::DragFloat3("CameraRotate", &cameraRotate.x, 0.01f);
        ImGui::DragFloat3("SphereCenter", &sphere.center.x, 0.01f);
        ImGui::DragFloat("SphereRadius", &sphere.radius, 0.01f);
        ImGui::End();

        Matrix4x4 worldMatrix = MakeIdentityMatrix();
        Matrix4x4 cameraMatrix = Multiply(MakeTranslateMatrix({ -cameraTranslate.x, -cameraTranslate.y, -cameraTranslate.z }),
            Multiply(MakeRotateY(-cameraRotate.y), MakeRotateX(-cameraRotate.x)));
        Matrix4x4 viewMatrix = cameraMatrix;
        Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, 1280.0f / 720.0f, 0.1f, 100.0f);
        Matrix4x4 viewProjectionMatrix = Multiply(viewMatrix, projectionMatrix);
        Matrix4x4 viewportMatrix = MakeViewportMatrix(0, 0, 1280, 720, 0.0f, 1.0f);

        DrawGrid(viewProjectionMatrix, viewportMatrix);
        DrawSphere(sphere, viewProjectionMatrix, viewportMatrix, 0x000000FF);

        Novice::EndFrame();
        if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) {
            break;
        }
    }

    Novice::Finalize();
    return 0;
}
