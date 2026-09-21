#pragma once
#include <string>

enum class ETModCameraMode
{
    DEFAULT,  // MTA/GTA Default Camera
    FIRST_PERSON,
    THIRD_PERSON
};

class CTModCameraManager
{
public:
    static CTModCameraManager& GetSingleton()
    {
        static CTModCameraManager instance;
        return instance;
    }

    void Init();
    void DoPulse();

    // Called right after GTA's CGame::Process() and during PreRender to bind the camera matrix
    // to the character's head bone (BONE_HEAD) before RenderWare/D3D draws the 3D scene.
    void UpdateCameraPreRender();

    void        SetCameraMode(const std::string& mode);
    std::string GetCameraMode() const;
    void        SetCameraOffset(float x, float y, float z);
    void        SetCameraDistance(float distance);

    ETModCameraMode GetMode() const { return m_mode; }

private:
    CTModCameraManager() = default;
    ~CTModCameraManager() = default;

    ETModCameraMode m_mode = ETModCameraMode::DEFAULT;
    float           m_fOffsetX = 0.0f;
    float           m_fOffsetY = 0.0f;
    float           m_fOffsetZ = 0.0f;               // Custom user offset (eye-level baseline is applied automatically)
    float           m_fDistance = 2.5f;              // TPV Distance
    bool            m_bLocalPedAlphaHidden = false;  // Tracks whether local ped alpha was set to 0 for FPV
};
