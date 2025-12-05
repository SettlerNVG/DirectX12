//***************************************************************************************
// MotionVectors.hlsl - Generate motion vectors for TAA
//***************************************************************************************

#include "Common.hlsl"

struct VertexIn
{
    float3 PosL    : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC    : TEXCOORD;
};

struct VertexOut
{
    float4 PosH         : SV_POSITION;
    float4 CurrPosH     : POSITION0;
    float4 PrevPosH     : POSITION1;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    
    // Transform to world space
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    
    // Current frame clip space position
    vout.CurrPosH = mul(posW, gViewProj);
    vout.PosH = vout.CurrPosH;
    
    // Previous frame clip space position
    vout.PrevPosH = mul(posW, gPrevViewProj);
    
    return vout;
}

float2 PS(VertexOut pin) : SV_Target
{
    // Convert to NDC (Normalized Device Coordinates)
    float2 currNDC = pin.CurrPosH.xy / pin.CurrPosH.w;
    float2 prevNDC = pin.PrevPosH.xy / pin.PrevPosH.w;
    
    // Calculate motion vector (in screen space)
    // Motion vector points from previous position to current position
    float2 velocity = (currNDC - prevNDC) * 0.5f; // Scale to match screen space
    
    return velocity;
}
