//***************************************************************************************
// TAAResolve.hlsl - Temporal Anti-Aliasing resolve pass
// Based on:
// - https://www.elopezr.com/temporal-aa-and-the-quest-for-the-holy-trail/
// - https://sugulee.wordpress.com/2021/06/21/temporal-anti-aliasingtaa-tutorial/
// - https://alextardif.com/TAA.html
//***************************************************************************************

cbuffer cbTAA : register(b0)
{
    float2 gJitterOffset;
    float2 gScreenSize;
    float gBlendFactor;      // Typically 0.05-0.1
    float gMotionScale;      // Scale for motion vectors
    float2 gPadding;
};

Texture2D gCurrentFrame  : register(t0);
Texture2D gHistoryFrame  : register(t1);
Texture2D gMotionVectors : register(t2);
Texture2D gDepthMap      : register(t3);

// Samplers match GetStaticSamplers() in TAAApp.cpp
SamplerState gsamPointWrap   : register(s0);
SamplerState gsamPointClamp  : register(s1);
SamplerState gsamLinearWrap  : register(s2);
SamplerState gsamLinearClamp : register(s3);

struct VertexOut
{
    float4 PosH  : SV_POSITION;
    float2 TexC  : TEXCOORD;
};

VertexOut VS(uint vid : SV_VertexID)
{
    VertexOut vout;
    
    // Full-screen triangle
    vout.TexC = float2((vid << 1) & 2, vid & 2);
    vout.PosH = float4(vout.TexC * float2(2, -2) + float2(-1, 1), 0, 1);
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    float2 texCoord = pin.TexC;
    float2 texelSize = 1.0 / gScreenSize;
    
    // Sample current frame
    float3 currentColor = gCurrentFrame.Sample(gsamPointClamp, texCoord).rgb;
    
    // Sample motion vector
    float2 velocity = gMotionVectors.Sample(gsamPointClamp, texCoord).rg;
    
    // Calculate history texture coordinate
    float2 historyTexCoord = texCoord + velocity;
    
    // Check if history sample is valid (within screen bounds)
    bool validHistory = all(historyTexCoord >= 0.0) && all(historyTexCoord <= 1.0);
    
    if (!validHistory)
    {
        // No valid history, use current frame
        return float4(currentColor, 1.0);
    }
    
    // Sample history with bilinear filtering
    float3 historyColor = gHistoryFrame.Sample(gsamLinearClamp, historyTexCoord).rgb;
    
    // Neighborhood sampling for min/max clipping (3x3)
    float3 colorMin = currentColor;
    float3 colorMax = currentColor;
    float3 colorSum = currentColor;
    
    [unroll]
    for (int dy = -1; dy <= 1; ++dy)
    {
        [unroll]
        for (int dx = -1; dx <= 1; ++dx)
        {
            if (dx == 0 && dy == 0) continue;
            
            float2 offset = float2(dx, dy) * texelSize;
            float3 neighborColor = gCurrentFrame.Sample(gsamPointClamp, texCoord + offset).rgb;
            
            colorMin = min(colorMin, neighborColor);
            colorMax = max(colorMax, neighborColor);
            colorSum += neighborColor;
        }
    }
    
    float3 colorAvg = colorSum / 9.0;
    
    // Simple clamp to neighborhood (reduces ghosting)
    historyColor = clamp(historyColor, colorMin, colorMax);
    
    // Adaptive blend factor based on motion
    float motionLength = length(velocity * gScreenSize);
    float adaptiveBlend = gBlendFactor;
    
    // Increase blend factor for fast motion (less history)
    adaptiveBlend = lerp(adaptiveBlend, 0.5, saturate(motionLength / 20.0));
    
    // Temporal blend: lerp(history, current, blend)
    // Lower blend = more history (smoother), higher blend = more current (sharper)
    float3 finalColor = lerp(historyColor, currentColor, adaptiveBlend);
    
    return float4(finalColor, 1.0);
}
