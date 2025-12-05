//***************************************************************************************
// MotionVectors.hlsl - Generate per-pixel motion vectors for TAA
//
// Computes screen-space velocity by comparing current and previous frame positions
// Output: RG texture with motion vectors in texture space [0,1]
// Motion vectors point from current position to previous position (for reprojection)
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
    float4 CurrPosH     : POSITION0;  // Current frame clip space position
    float4 PrevPosH     : POSITION1;  // Previous frame clip space position
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    
    // Transform to world space
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    
    // Current frame clip space position WITHOUT jitter (for motion vectors)
    vout.CurrPosH = mul(posW, gUnjitteredViewProj);
    
    // For rasterization, use jittered position
    vout.PosH = mul(posW, gViewProj);
    
    // Previous frame clip space position (also without jitter)
    vout.PrevPosH = mul(posW, gPrevViewProj);
    
    return vout;
}

float2 PS(VertexOut pin) : SV_Target
{
    // Convert to NDC (Normalized Device Coordinates)
    float2 currNDC = pin.CurrPosH.xy / pin.CurrPosH.w;
    float2 prevNDC = pin.PrevPosH.xy / pin.PrevPosH.w;
    
    // Motion vector in NDC space, then convert to UV space
    // Velocity = previous - current (where the pixel came from)
    float2 velocity = (prevNDC - currNDC) * 0.5f;
    
    return velocity;
}
