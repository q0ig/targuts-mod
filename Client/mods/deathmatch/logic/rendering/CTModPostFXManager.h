#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <string>

class CTModPostFXManager
{
public:
    static CTModPostFXManager& GetSingleton()
    {
        static CTModPostFXManager instance;
        return instance;
    }

    void Init();
    void Render(IDirect3DDevice9* pDevice);
    void SetSourceColorCorrection(bool bEnabled);
    bool IsSourceColorCorrectionEnabled() const { return m_bEnabled; }

private:
    CTModPostFXManager();
    ~CTModPostFXManager();

    bool m_bEnabled = false;
    bool m_bInitialized = false;

    IDirect3DTexture9* m_pScreenTexture = nullptr;
    IDirect3DVertexShader9* m_pVertexShader = nullptr;
    IDirect3DPixelShader9* m_pPixelShader = nullptr;

    void CreateShaders(IDirect3DDevice9* pDevice);
    void CleanupShaders();
};
