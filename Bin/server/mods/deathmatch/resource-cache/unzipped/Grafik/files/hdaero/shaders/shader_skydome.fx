// shader_skydome.fx
// Author: Ren712/AngerMAN

float4x4 gWorldViewProjection : WORLDVIEWPROJECTION;

texture gTEX;
float3 gObjScale=(1,1,1);
float3 gTexScale=(1,2,1);
float gColorAdd=0;
float gColorPow=1;
float gAlpha=1;

//---------------------------------------------------------------------
//-- Sampler for the main texture (needed for pixel shaders)
//---------------------------------------------------------------------


sampler texMap = sampler_state
{
	Texture = (gTEX);
	MinFilter = Linear;
	MagFilter = Linear;
	MipFilter = Linear;
	AddressU =  Wrap;
	AddressV =  Wrap;
    AddressW =  Wrap;
};


//---------------------------------------------------------------------
//-- Structure of data sent to the vertex shader
//--------------------------------------------------------------------- 
 
 struct VSInput
{
    float4 Position : POSITION; 
    float3 TexCoord : TEXCOORD0;
};

//---------------------------------------------------------------------
//-- Structure of data sent to the pixel shader ( from the vertex shader )
//---------------------------------------------------------------------

struct PSInput
{
    float4 Position : POSITION; 
	float3 TexCoord : TEXCOORD0;	
};

	
//-----------------------------------------------------------------------------
//-- VertexShader
//-----------------------------------------------------------------------------
PSInput VertexShaderSB(VSInput VS)
{
    PSInput PS = (PSInput)0;
	VS.Position.xy*=gObjScale.xy;
	VS.Position.z*=gObjScale.z;
    PS.Position = mul(VS.Position, gWorldViewProjection);
	PS.TexCoord=VS.TexCoord.xyz;
    return PS;
}
 
//-----------------------------------------------------------------------------
//-- PixelShader
//-----------------------------------------------------------------------------

float4 PixelShaderSB(PSInput PS) : COLOR0
{	
  
	float3 TexCoord = PS.TexCoord.xyz;
	float4 Tex =  tex2D(texMap, TexCoord*gTexScale);
	float4 outPut=saturate(float4(pow(Tex.rgb+gColorAdd,gColorPow),Tex.a*gAlpha));
    return outPut;
}


////////////////////////////////////////////////////////////
//////////////////////////////// TECHNIQUES ////////////////
////////////////////////////////////////////////////////////
technique skybox_alt
{
    pass P0
    {
		AlphaRef = 1;
		AlphaBlendEnable = TRUE;
		VertexShader = compile vs_2_0 VertexShaderSB();
        PixelShader = compile ps_2_0 PixelShaderSB();
    }
}
