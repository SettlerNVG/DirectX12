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

// Catmull-Rom filtering for better history sampling
// Based on https://gist.github.com/TheRealMJP/c83b8c0f46b63f3a88a5986f4fa982b1
float3 SampleTextureCatmullRom(Texture2D tex, SamplerState samp, float2 uv, float2 texSize)
{
    float2 samplePos = uv * texSize;
    float2 texPos1 = floor(samplePos - 0.5) + 0.5;
    float2 f = samplePos - texPos1;
    
    // Catmull-Rom weights
    float2 w0 = f * (-0.5 + f * (1.0 - 0.5 * f));
    float2 w1 = 1.0 + f * f * (-2.5 + 1.5 * f);
    float2 w2 = f * (0.5 + f * (2.0 - 1.5 * f));
    float2 w3 = f * f * (-0.5 + 0.5 * f);
    
    float2 w12 = w1 + w2;
    float2 offset12 = w2 / (w1 + w2);
    
    float2 texPos0 = texPos1 - 1.0;
    float2 texPos3 = texPos1 + 2.0;
    float2 texPos12 = texPos1 + offset12;
    
    texPos0 /= texSize;
    texPos3 /= texSize;
    texPos12 /= texSize;
    
    float3 result = float3(0.0, 0.0, 0.0);
    result += tex.SampleLevel(samp, float2(texPos0.x, texPos0.y), 0).rgb * w0.x * w0.y;
    result += tex.SampleLevel(samp, float2(texPos12.x, texPos0.y), 0).rgb * w12.x * w0.y;
    result += tex.SampleLevel(samp, float2(texPos3.x, texPos0.y), 0).rgb * w3.x * w0.y;
    
    result += tex.SampleLevel(samp, float2(texPos0.x, texPos12.y), 0).rgb * w0.x * w12.y;
    result += tex.SampleLevel(samp, float2(texPos12.x, texPos12.y), 0).rgb * w12.x * w12.y;
    result += tex.SampleLevel(samp, float2(texPos3.x, texPos12.y), 0).rgb * w3.x * w12.y;
    
    result += tex.SampleLevel(samp, float2(texPos0.x, texPos3.y), 0).rgb * w0.x * w3.y;
    result += tex.SampleLevel(samp, float2(texPos12.x, texPos3.y), 0).rgb * w12.x * w3.y;
    result += tex.SampleLevel(samp, float2(texPos3.x, texPos3.y), 0).rgb * w3.x * w3.y;
    
    return max(result, 0.0);
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
    float2 texelSize = 1.0 / gScreenSize;
    
    // Sample current frame
    float3 currentColor = gCurrentFrame.Sample(gsamPointClamp, texCoord).rgb;
    float currentDepth = gDepthMap.Sample(gsamPointClamp, texCoord).r;
    
    // Sample motion vector with 3x3 max filter (velocity dilation)
    // This helps with edges and thin objects
    float2 velocity = gMotionVectors.Sample(gsamPointClamp, texCoord).rg;
    float maxVelocityLength = length(velocity);
    
    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            if (x == 0 && y == 0) continue;
            
            float2 offset = float2(x, y) * texelSize;
            float2 neighborVelocity = gMotionVectors.Sample(gsamPointClamp, texCoord + offset).rg;
            float neighborLength = length(neighborVelocity);
            
            if (neighborLength > maxVelocityLength)
            {
                velocity = neighborVelocity;
                maxVelocityLength = neighborLength;
            }
        }
    }
    
    // Calculate history texture coordinate
    float2 historyTexCoord = texCoord - velocity;
    
    // Check if history sample is valid (within screen bounds)
    bool validHistory = all(historyTexCoord >= 0.0) && all(historyTexCoord <= 1.0);
    
    if (!validHistory)
    {
        // No valid history, use current frame
        return float4(currentColor, 1.0);
    }
    
    // Sample history with Catmull-Rom filtering for better quality
    float3 historyColor = SampleTextureCatmullRom(gHistoryFrame, gsamLinearClamp, historyTexCoord, gScreenSize);
    
    // Depth-based disocclusion detection
    float historyDepth = gDepthMap.Sample(gsamLinearClamp, historyTexCoord).r;
    float depthDiff = abs(currentDepth - historyDepth);
    bool disoccluded = depthDiff > 0.01; // Threshold for disocclusion
    
    // Neighborhood sampling for variance-based clipping (3x3)
    float3 colorMin = currentColor;
    float3 colorMax = currentColor;
    float3 colorSum = currentColor;
    float3 colorSum2 = currentColor * currentColor;
    float sampleCount = 1.0;
    
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
            colorSum2 += neighborColor * neighborColor;
            sampleCount += 1.0;
        }
    }
    
    // Calculate mean and variance
    float3 colorAvg = colorSum / sampleCount;
    float3 colorVar = (colorSum2 / sampleCount) - (colorAvg * colorAvg);
    float3 colorStdDev = sqrt(max(colorVar, 0.0));
    
    // Variance-based clipping (better than min/max)
    float varianceClipGamma = 1.0;
    float3 boxMin = colorAvg - varianceClipGamma * colorStdDev;
    float3 boxMax = colorAvg + varianceClipGamma * colorStdDev;
    
    // Convert to YCoCg for better color space clamping
    float3 currentYCoCg = RGBToYCoCg(currentColor);
    float3 historyYCoCg = RGBToYCoCg(historyColor);
    float3 boxMinYCoCg = RGBToYCoCg(boxMin);
    float3 boxMaxYCoCg = RGBToYCoCg(boxMax);
    
    // Clip history to neighborhood
    historyYCoCg = ClipAABB(boxMinYCoCg, boxMaxYCoCg, historyYCoCg, currentYCoCg);
    historyColor = YCoCgToRGB(historyYCoCg);
    
    // Adaptive blend factor based on multiple factors
    float motionLength = length(velocity * gScreenSize);
    float variance = length(colorVar);
    float adaptiveBlend = gBlendFactor;
    
    // Increase blend factor for fast motion (less history)
    adaptiveBlend = lerp(adaptiveBlend, 0.5, saturate(motionLength / 40.0));
    
    // Increase blend factor in high variance areas (less history)
    adaptiveBlend = lerp(adaptiveBlend, 0.3, saturate(variance * 10.0));
    
    // Increase blend factor for disoccluded pixels
    if (disoccluded)
    {
        adaptiveBlend = max(adaptiveBlend, 0.8);
    }
    
    // Temporal blend
    float3 finalColor = lerp(historyColor, currentColor, adaptiveBlend);
    
    // Sharpening pass to compensate for temporal blur
    // Based on unsharp mask technique
    float sharpenAmount = 0.25;
    float3 sharpened = finalColor + (finalColor - colorAvg) * sharpenAmount;
    finalColor = lerp(finalColor, sharpened, 0.5);
    
    // Clamp to valid range
    finalColor = max(finalColor, 0.0);
    
    return float4(finalColor, 1.0);
}
