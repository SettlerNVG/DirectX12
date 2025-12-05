//***************************************************************************************
// TAAResolve.hlsl - Temporal Anti-Aliasing resolve pass
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

SamplerState gsamPointClamp  : register(s0);
SamplerState gsamLinearClamp : register(s1);

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

// RGB to YCoCg color space conversion for better color clamping
float3 RGBToYCoCg(float3 rgb)
{
    float Y  = dot(rgb, float3(0.25, 0.5, 0.25));
    float Co = dot(rgb, float3(0.5, 0.0, -0.5));
    float Cg = dot(rgb, float3(-0.25, 0.5, -0.25));
    return float3(Y, Co, Cg);
}

float3 YCoCgToRGB(float3 ycocg)
{
    float Y  = ycocg.x;
    float Co = ycocg.y;
    float Cg = ycocg.z;
    
    float r = Y + Co - Cg;
    float g = Y + Cg;
    float b = Y - Co - Cg;
    
    return float3(r, g, b);
}

// Clip history color to neighborhood of current frame
// This reduces ghosting artifacts
float3 ClipAABB(float3 aabbMin, float3 aabbMax, float3 prevColor, float3 currColor)
{
    // Find the closest point in AABB to prevColor
    float3 center = 0.5 * (aabbMax + aabbMin);
    float3 extents = 0.5 * (aabbMax - aabbMin);
    
    float3 offset = prevColor - center;
    float3 v_unit = offset / extents;
    float3 absUnit = abs(v_unit);
    float maxUnit = max(absUnit.x, max(absUnit.y, absUnit.z));
    
    if (maxUnit > 1.0)
        return center + (offset / maxUnit);
    else
        return prevColor;
}

float4 PS(VertexOut pin) : SV_Target
{
    float2 texCoord = pin.TexC;
    
    // Sample current frame
    float3 currentColor = gCurrentFrame.Sample(gsamPointClamp, texCoord).rgb;
    
    // Sample motion vector
    float2 velocity = gMotionVectors.Sample(gsamPointClamp, texCoord).rg;
    
    // Calculate history texture coordinate
    float2 historyTexCoord = texCoord - velocity;
    
    // Check if history sample is valid (within screen bounds)
    bool validHistory = all(historyTexCoord >= 0.0) && all(historyTexCoord <= 1.0);
    
    if (!validHistory)
    {
        // No valid history, use current frame
        return float4(currentColor, 1.0);
    }
    
    // Sample history with bilinear filtering
    float3 historyColor = gHistoryFrame.Sample(gsamLinearClamp, historyTexCoord).rgb;
    
    // Neighborhood clamping to reduce ghosting
    // Sample 3x3 neighborhood around current pixel
    float3 colorMin = currentColor;
    float3 colorMax = currentColor;
    float3 colorSum = currentColor;
    float3 colorSum2 = currentColor * currentColor;
    
    const int radius = 1;
    float sampleCount = 1.0;
    
    [unroll]
    for (int y = -radius; y <= radius; ++y)
    {
        [unroll]
        for (int x = -radius; x <= radius; ++x)
        {
            if (x == 0 && y == 0) continue;
            
            float2 offset = float2(x, y) / gScreenSize;
            float3 neighborColor = gCurrentFrame.Sample(gsamPointClamp, texCoord + offset).rgb;
            
            colorMin = min(colorMin, neighborColor);
            colorMax = max(colorMax, neighborColor);
            colorSum += neighborColor;
            colorSum2 += neighborColor * neighborColor;
            sampleCount += 1.0;
        }
    }
    
    // Calculate variance for adaptive blending
    float3 colorAvg = colorSum / sampleCount;
    float3 colorVar = (colorSum2 / sampleCount) - (colorAvg * colorAvg);
    float variance = length(colorVar);
    
    // Convert to YCoCg for better color space clamping
    float3 currentYCoCg = RGBToYCoCg(currentColor);
    float3 historyYCoCg = RGBToYCoCg(historyColor);
    float3 minYCoCg = RGBToYCoCg(colorMin);
    float3 maxYCoCg = RGBToYCoCg(colorMax);
    
    // Clip history to neighborhood
    historyYCoCg = ClipAABB(minYCoCg, maxYCoCg, historyYCoCg, currentYCoCg);
    historyColor = YCoCgToRGB(historyYCoCg);
    
    // Adaptive blend factor based on motion and variance
    float motionLength = length(velocity * gScreenSize);
    float adaptiveBlend = gBlendFactor;
    
    // Increase blend factor for fast motion (less history)
    adaptiveBlend = lerp(gBlendFactor, 0.5, saturate(motionLength / 40.0));
    
    // Increase blend factor in high variance areas (less history)
    adaptiveBlend = lerp(adaptiveBlend, 0.3, saturate(variance * 10.0));
    
    // Temporal blend
    float3 finalColor = lerp(historyColor, currentColor, adaptiveBlend);
    
    return float4(finalColor, 1.0);
}
