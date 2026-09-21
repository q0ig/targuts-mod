#include "StdInc.h"
#include "CTModPostFXManager.h"
#include "CClientGame.h"
#include <game/CGame.h>
#include <game/CWaterManager.h>
#include <game/CWorld.h>
#include <core/CCoreInterface.h>
#include <multiplayer/CMultiplayer.h>

static const char* g_PostFX_HLSL = R"(
struct VS_INPUT {
    float4 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
};

struct VS_OUTPUT {
    float4 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
};

VS_OUTPUT mainVS(VS_INPUT input) {
    VS_OUTPUT output;
    // Just pass-through clip-space coordinates
    output.Position = input.Position;
    output.TexCoord = input.TexCoord;
    return output;
}

sampler2D ScreenSampler : register(s0);

float4 mainPS(VS_OUTPUT input) : COLOR {
    float4 color = tex2D(ScreenSampler, input.TexCoord);
    
    // 1. Exposure reduction — prevents blown-out highlights from GTA's
    //    full-white color filter pass. Without this, removing GTA's native
    //    orange tint leaves the scene overexposed.
    float exposure = 0.92f;
    color.rgb *= exposure;
    
    // 2. Gentle contrast — 1.08 is enough to add depth without clipping
    //    bright surfaces. The original 1.15 was pushing sky/white surfaces
    //    well above 1.0 before clamp, causing the "eye-searing glare".
    float contrast = 1.08f;
    color.rgb = (color.rgb - 0.5f) * contrast + 0.5f;
    
    // 3. Tints — cool blue shadows + neutral highlights (Source Engine palette)
    float luminance = dot(color.rgb, float3(0.299f, 0.587f, 0.114f));
    float3 shadowTint  = float3(0.85f, 0.90f, 1.0f);
    float3 highlightTint = float3(1.0f, 0.98f, 0.95f);
    
    float3 graded = lerp(color.rgb * shadowTint, color.rgb * highlightTint, luminance);
    
    // 4. Saturation (Source engine is slightly less saturated than GTA)
    float saturation = 0.85f;
    color.rgb = lerp(float3(luminance, luminance, luminance), graded, saturation);
    
    // 5. Soft highlight compression (Reinhard-style tone mapping) —
    //    gently rolls off highlights so bright areas (sky, reflections)
    //    don't clip to pure white. This is the key to removing "glare".
    color.rgb = color.rgb / (color.rgb + 0.15f);
    // Re-scale so midtones stay close to their original brightness
    color.rgb *= (1.0f + 0.15f);
    
    // 6. Final clamp to valid range
    color.rgb = saturate(color.rgb);
    
    return color;
}
)";

struct SScreenVertex {
    float x, y, z, w;
    float u, v;
};

CTModPostFXManager::CTModPostFXManager()
{
}

CTModPostFXManager::~CTModPostFXManager()
{
    CleanupShaders();
}

void CTModPostFXManager::Init()
{
}

void CTModPostFXManager::SetSourceColorCorrection(bool bEnabled)
{
    m_bEnabled = bEnabled;
    
    if (g_pMultiplayer)
    {
        if (m_bEnabled)
        {
            // Reset GTA Color filter to purely white (removes yellow/orange tint)
            g_pMultiplayer->SetColorFilter(0xFFFFFFFF, 0xFFFFFFFF);
            if (g_pGame) g_pGame->SetBlurLevel(0);
        }
        else
        {
            g_pMultiplayer->ResetColorFilter();
            if (g_pGame) g_pGame->SetBlurLevel(36); // Default GTA SA blur level approx
        }
    }
}

void CTModPostFXManager::CreateShaders(IDirect3DDevice9* pDevice)
{
    if (m_bInitialized) return;

    ID3DXBuffer* pVSBuffer = nullptr;
    ID3DXBuffer* pPSBuffer = nullptr;
    ID3DXBuffer* pError = nullptr;

    if (SUCCEEDED(D3DXCompileShader(g_PostFX_HLSL, strlen(g_PostFX_HLSL), nullptr, nullptr, "mainVS", "vs_2_0", 0, &pVSBuffer, &pError, nullptr))) {
        pDevice->CreateVertexShader((DWORD*)pVSBuffer->GetBufferPointer(), &m_pVertexShader);
        pVSBuffer->Release();
    }
    
    if (SUCCEEDED(D3DXCompileShader(g_PostFX_HLSL, strlen(g_PostFX_HLSL), nullptr, nullptr, "mainPS", "ps_2_0", 0, &pPSBuffer, &pError, nullptr))) {
        pDevice->CreatePixelShader((DWORD*)pPSBuffer->GetBufferPointer(), &m_pPixelShader);
        pPSBuffer->Release();
    }
    
    if (pError) pError->Release();
    
    m_bInitialized = true;
}

void CTModPostFXManager::CleanupShaders()
{
    if (m_pVertexShader) { m_pVertexShader->Release(); m_pVertexShader = nullptr; }
    if (m_pPixelShader) { m_pPixelShader->Release(); m_pPixelShader = nullptr; }
    if (m_pScreenTexture) { m_pScreenTexture->Release(); m_pScreenTexture = nullptr; }
    m_bInitialized = false;
}

void CTModPostFXManager::Render(IDirect3DDevice9* pDevice)
{
    if (!m_bEnabled) return;
    
    if (!m_bInitialized)
        CreateShaders(pDevice);
        
    if (!m_pVertexShader || !m_pPixelShader)
        return;

    IDirect3DSurface9* pBackBuffer = nullptr;
    if (FAILED(pDevice->GetRenderTarget(0, &pBackBuffer)) || !pBackBuffer)
        return;

    D3DSURFACE_DESC desc;
    pBackBuffer->GetDesc(&desc);

    // Create or resize screen texture if needed
    if (!m_pScreenTexture) {
        pDevice->CreateTexture(desc.Width, desc.Height, 1, D3DUSAGE_RENDERTARGET, desc.Format, D3DPOOL_DEFAULT, &m_pScreenTexture, nullptr);
    } else {
        D3DSURFACE_DESC texDesc;
        m_pScreenTexture->GetLevelDesc(0, &texDesc);
        if (texDesc.Width != desc.Width || texDesc.Height != desc.Height) {
            m_pScreenTexture->Release();
            pDevice->CreateTexture(desc.Width, desc.Height, 1, D3DUSAGE_RENDERTARGET, desc.Format, D3DPOOL_DEFAULT, &m_pScreenTexture, nullptr);
        }
    }

    if (!m_pScreenTexture) {
        pBackBuffer->Release();
        return;
    }

    IDirect3DSurface9* pTextureSurface = nullptr;
    if (SUCCEEDED(m_pScreenTexture->GetSurfaceLevel(0, &pTextureSurface))) {
        pDevice->StretchRect(pBackBuffer, nullptr, pTextureSurface, nullptr, D3DTEXF_NONE);
        pTextureSurface->Release();
    }
    
    pBackBuffer->Release();

    // Render fullscreen quad
    pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    
    pDevice->SetVertexShader(m_pVertexShader);
    pDevice->SetPixelShader(m_pPixelShader);
    
    pDevice->SetTexture(0, m_pScreenTexture);
    pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);

    // Align texels to pixels (D3D9 half-pixel offset)
    float hU = 0.5f / (float)desc.Width;
    float hV = 0.5f / (float)desc.Height;

    SScreenVertex quad[4] = {
        { -1.0f,  1.0f, 0.0f, 1.0f, 0.0f + hU, 0.0f + hV },
        {  1.0f,  1.0f, 0.0f, 1.0f, 1.0f + hU, 0.0f + hV },
        { -1.0f, -1.0f, 0.0f, 1.0f, 0.0f + hU, 1.0f + hV },
        {  1.0f, -1.0f, 0.0f, 1.0f, 1.0f + hU, 1.0f + hV }
    };

    pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
    pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(SScreenVertex));

    pDevice->SetVertexShader(nullptr);
    pDevice->SetPixelShader(nullptr);
    pDevice->SetTexture(0, nullptr);
    pDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
}
