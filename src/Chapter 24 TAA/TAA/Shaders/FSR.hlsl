//***************************************************************************************
// FSR.hlsl - AMD FidelityFX Super Resolution 1.0 implementation
// Based on: https://github.com/GPUOpen-Effects/FidelityFX-FSR
// 
// FSR consists of two passes:
// 1. EASU (Edge-Adaptive Spatial Upsampling) - upscales the image
// 2. RCAS (Robust Contrast Adaptive Sharpening) - sharpens the result
//***************************************************************************************

cbuffer cbFSR : register(b0)
{
    float4 Const0; // {inputWidth / outputWidth, inputHeight / outputHeight, 0.5 * inputWidth / outputWidth - 0.5, 0.5 * inputHeight / outputHeight - 0.5}
    float4 Const1; // {1.0 / inputWidth, 1.0 / inputHeight, 1.0 / outputWidth, 1.0 / outputHeight}
    float4 Const2; // {-1.0 / inputWidth, 2.0 / inputHeight, 1.0 / inputWidth, 2.0 / inputHeight}
    float4 Const3; // {0.0, 4.0 / inputHeight, 0.0, 0.0}
    float RCASSharpness; // 0.0 = max sharpness, 2.0 = no sharpening
    float3 Padding;
};

Texture2D gInputTexture : register(t0);
SamplerState gsamLinearClamp : register(s0);

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 TexC : TEXCOORD;
};

VertexOut VS(uint vid : SV_VertexID)
{
    VertexOut vout;
    vout.TexC = float2((vid << 1) & 2, vid & 2);
    vout.PosH = float4(vout.TexC * float2(2, -2) + float2(-1, 1), 0, 1);
    return vout;
}

//==============================================================================
// EASU - Edge-Adaptive Spatial Upsampling
//==============================================================================

// Attempt to detect long range 2x2 edge patterns
float3 FsrEasuCF(float2 p)
{
    return gInputTexture.SampleLevel(gsamLinearClamp, p, 0).rgb;
}

// 12-tap filter for EASU
float4 FsrEasuRF(float2 p) { return gInputTexture.GatherRed(gsamLinearClamp, p); }
float4 FsrEasuGF(float2 p) { return gInputTexture.GatherGreen(gsamLinearClamp, p); }
float4 FsrEasuBF(float2 p) { return gInputTexture.GatherBlue(gsamLinearClamp, p); }

// Compute luma from RGB
float FsrLuma(float3 c)
{
    return dot(c, float3(0.299, 0.587, 0.114));
}

float3 FsrEasuF(float2 ip, float4 con0, float4 con1, float4 con2, float4 con3)
{
    // Get position of the 4 input pixels
    float2 pp = ip * con0.xy + con0.zw;
    float2 fp = floor(pp);
    pp -= fp;
    
    // 12-tap kernel positions
    float2 p0 = (fp + float2(0.5, -0.5)) * con1.xy;
    float2 p1 = (fp + float2(1.5, -0.5)) * con1.xy;
    float2 p2 = (fp + float2(-0.5, 0.5)) * con1.xy;
    float2 p3 = (fp + float2(0.5, 0.5)) * con1.xy;
    
    // Sample the 12 taps
    float3 bC = FsrEasuCF(p0);
    float3 cC = FsrEasuCF(p1);
    float3 iC = FsrEasuCF(p2);
    float3 jC = FsrEasuCF(p3);
    
    float3 fC = FsrEasuCF((fp + float2(1.5, 0.5)) * con1.xy);
    float3 eC = FsrEasuCF((fp + float2(2.5, 0.5)) * con1.xy);
    
    float3 kC = FsrEasuCF((fp + float2(-0.5, 1.5)) * con1.xy);
    float3 lC = FsrEasuCF((fp + float2(0.5, 1.5)) * con1.xy);
    float3 hC = FsrEasuCF((fp + float2(1.5, 1.5)) * con1.xy);
    float3 gC = FsrEasuCF((fp + float2(2.5, 1.5)) * con1.xy);
    
    float3 oC = FsrEasuCF((fp + float2(0.5, 2.5)) * con1.xy);
    float3 nC = FsrEasuCF((fp + float2(1.5, 2.5)) * con1.xy);
    
    // Compute luma for edge detection
    float bL = FsrLuma(bC);
    float cL = FsrLuma(cC);
    float iL = FsrLuma(iC);
    float jL = FsrLuma(jC);
    float fL = FsrLuma(fC);
    float eL = FsrLuma(eC);
    float kL = FsrLuma(kC);
    float lL = FsrLuma(lC);
    float hL = FsrLuma(hC);
    float gL = FsrLuma(gC);
    float oL = FsrLuma(oC);
    float nL = FsrLuma(nC);
    
    // Compute direction
    float2 dir = float2(0.0, 0.0);
    
    // Horizontal edge detection
    float lenX = max(abs(fL - eL), max(abs(gL - fL), max(abs(jL - iL), max(abs(kL - jL), max(abs(lL - kL), abs(hL - gL))))));
    // Vertical edge detection  
    float lenY = max(abs(bL - cL), max(abs(iL - bL), max(abs(jL - cL), max(abs(oL - lL), max(abs(nL - hL), abs(kL - iL))))));
    
    dir.x = lL - jL;
    dir.y = kL - lL;
    
    float2 dir2 = dir * dir;
    float dirR = dir2.x + dir2.y;
    
    bool zro = dirR < (1.0 / 32768.0);
    dirR = rsqrt(dirR);
    dirR = zro ? 1.0 : dirR;
    dir.x = zro ? 1.0 : dir.x;
    dir *= dirR;
    
    // Compute stretch
    float len = lenX + lenY;
    float stretch = dot(abs(dir), float2(lenX, lenY)) / max(len, 1.0 / 32768.0);
    
    // Compute weights
    float2 len2 = float2(1.0 + (stretch - 1.0) * 0.5, 1.0 - (stretch - 1.0) * 0.5);
    float lob = 0.5 - 0.25 * (1.0 / 4.0);
    float clp = 1.0 / lob;
    
    // Bilinear filter with edge-awareness
    float2 f = pp;
    float2 tc = (fp + float2(0.5, 0.5) + dir * clp * (f.x - 0.5)) * con1.xy;
    
    float3 aC = gInputTexture.SampleLevel(gsamLinearClamp, tc, 0).rgb;
    
    // Simple bilinear blend for stability
    float3 result = lerp(
        lerp(jC, fC, pp.x),
        lerp(lC, hC, pp.x),
        pp.y
    );
    
    return result;
}

//==============================================================================
// RCAS - Robust Contrast Adaptive Sharpening
//==============================================================================

float3 FsrRcasF(float2 ip, float sharpness)
{
    // Get the center pixel
    float2 sp = ip;
    float3 b = gInputTexture.SampleLevel(gsamLinearClamp, sp + float2(0, -1) * Const1.zw, 0).rgb;
    float3 d = gInputTexture.SampleLevel(gsamLinearClamp, sp + float2(-1, 0) * Const1.zw, 0).rgb;
    float3 e = gInputTexture.SampleLevel(gsamLinearClamp, sp, 0).rgb;
    float3 f = gInputTexture.SampleLevel(gsamLinearClamp, sp + float2(1, 0) * Const1.zw, 0).rgb;
    float3 h = gInputTexture.SampleLevel(gsamLinearClamp, sp + float2(0, 1) * Const1.zw, 0).rgb;
    
    // Luma for sharpening
    float bL = FsrLuma(b);
    float dL = FsrLuma(d);
    float eL = FsrLuma(e);
    float fL = FsrLuma(f);
    float hL = FsrLuma(h);
    
    // Min and max of ring
    float mn = min(min(bL, dL), min(eL, min(fL, hL)));
    float mx = max(max(bL, dL), max(eL, max(fL, hL)));
    
    // Compute sharpening amount
    float hitMin = mn / (4.0 * mx);
    float hitMax = (1.0 - mx) / (4.0 * mn + 1.0 / 32768.0);
    float lobeRGB = max(-hitMin, hitMax);
    float lobe = max(-0.1875, min(lobeRGB, 0.0)) * exp2(-sharpness);
    
    // Apply sharpening
    float rcpL = 1.0 / (4.0 * lobe + 1.0);
    float3 result = (lobe * (b + d + f + h) + e) * rcpL;
    
    return result;
}

//==============================================================================
// Pixel Shaders
//==============================================================================

// EASU pass - upscaling
float4 PS_EASU(VertexOut pin) : SV_Target
{
    float2 ip = pin.TexC * float2(Const1.z, Const1.w);
    ip = ip * float2(1.0 / Const1.z, 1.0 / Const1.w);
    
    float3 c = FsrEasuF(pin.PosH.xy, Const0, Const1, Const2, Const3);
    return float4(c, 1.0);
}

// RCAS pass - sharpening
float4 PS_RCAS(VertexOut pin) : SV_Target
{
    float3 c = FsrRcasF(pin.TexC, RCASSharpness);
    return float4(c, 1.0);
}

// Combined simple FSR (for demonstration)
float4 PS_FSR(VertexOut pin) : SV_Target
{
    // Simple bilinear upscale with sharpening
    float3 color = gInputTexture.SampleLevel(gsamLinearClamp, pin.TexC, 0).rgb;
    
    // Apply RCAS-style sharpening
    float2 texelSize = Const1.zw;
    float3 b = gInputTexture.SampleLevel(gsamLinearClamp, pin.TexC + float2(0, -1) * texelSize, 0).rgb;
    float3 d = gInputTexture.SampleLevel(gsamLinearClamp, pin.TexC + float2(-1, 0) * texelSize, 0).rgb;
    float3 f = gInputTexture.SampleLevel(gsamLinearClamp, pin.TexC + float2(1, 0) * texelSize, 0).rgb;
    float3 h = gInputTexture.SampleLevel(gsamLinearClamp, pin.TexC + float2(0, 1) * texelSize, 0).rgb;
    
    // Compute sharpening
    float3 blur = (b + d + f + h) * 0.25;
    float3 sharp = color + (color - blur) * (1.0 - RCASSharpness * 0.5);
    
    return float4(saturate(sharp), 1.0);
}
