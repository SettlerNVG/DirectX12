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
    
    // Current frame clip space position (with jitter)
    vout.CurrPosH = mul(posW, gViewProj);
    vout.PosH = vout.CurrPosH;
    
    // Previous frame clip space position (with previous frame's jitter)
    vout.PrevPosH = mul(posW, gPrevViewProj);
    
    return vout;
}

float2 PS(VertexOut pin) : SV_Target
{
    // Convert to NDC (Normalized Device Coordinates)
    float2 currNDC = pin.CurrPosH.xy / pin.CurrPosH.w;
    float2 prevNDC = pin.PrevPosH.xy / pin.PrevPosH.w;
    
    // Calculate motion vector (from current to previous for reprojection)
    // Convert from NDC [-1,1] to texture space [0,1]
    float2 currUV = currNDC * 0.5f + 0.5f;
    float2 prevUV = prevNDC * 0.5f + 0.5f;
    
    // Flip Y for DirectX
    currUV.y = 1.0f - currUV.y;
    prevUV.y = 1.0f - prevUV.y;
    
    // Velocity points from current to previous (for history lookup)
    float2 velocity = currUV - prevUV;
    
    return velocity;
}
