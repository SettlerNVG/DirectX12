//***************************************************************************************
// Terrain.hlsl - Terrain rendering shader with heightmap displacement
//***************************************************************************************

#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 1
#endif

struct Light
{
    float3 Strength;
    float FalloffStart;
    float3 Direction;
    float FalloffEnd;
    float3 Position;
    float SpotPower;
};

#define MaxLights 16

cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float4x4 gTexTransform;
    uint gMaterialIndex;
    uint gLODLevel;
    uint gObjPad1;
    uint gObjPad2;
};

cbuffer cbPass : register(b1)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float3 gEyePosW;
    float cbPerPassPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
    float4 gAmbientLight;
    float4 gFrustumPlanes[6];
    Light gLights[MaxLights];
};

cbuffer cbTerrain : register(b2)
{
    float gMinHeight;
    float gMaxHeight;
    float gTerrainSize;
    float gTexelSize;
    float2 gHeightMapSize;
    float2 gTerrainPadding;
};

Texture2D gHeightMap : register(t0);
Texture2D gDiffuseMap : register(t1);
Texture2D gNormalMap : register(t2);

SamplerState gsamLinearWrap : register(s0);
SamplerState gsamLinearClamp : register(s1);

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 TexC : TEXCOORD;
    float Height : HEIGHT;
};

// Sample height from heightmap
float SampleHeight(float2 uv)
{
    // DDS heightmap may be in different formats, sample red channel
    float h = gHeightMap.SampleLevel(gsamLinearClamp, saturate(uv), 0).r;
    return gMinHeight + saturate(h) * (gMaxHeight - gMinHeight);
}

// Calculate normal from heightmap
float3 CalculateNormal(float2 uv)
{
    float2 texelSize = 1.0 / gHeightMapSize;
    
    float hL = SampleHeight(uv + float2(-texelSize.x, 0));
    float hR = SampleHeight(uv + float2(texelSize.x, 0));
    float hD = SampleHeight(uv + float2(0, -texelSize.y));
    float hU = SampleHeight(uv + float2(0, texelSize.y));
    
    float3 normal;
    normal.x = hL - hR;
    normal.y = 2.0 * gTerrainSize * texelSize.x;
    normal.z = hD - hU;
    
    return normalize(normal);
}

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    
    // Transform local position to world space
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    
    // Calculate UV for heightmap sampling
    float2 uv = vin.TexC;
    
    // Sample height and displace vertex
    float height = SampleHeight(uv);
    posW.y = height;
    
    vout.PosW = posW.xyz;
    vout.Height = height;
    
    // Calculate normal from heightmap
    vout.NormalW = CalculateNormal(uv);
    
    // Transform to homogeneous clip space
    vout.PosH = mul(posW, gViewProj);
    
    // Output texture coordinates for detail texturing
    vout.TexC = vin.TexC * 32.0; // Tile the texture
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    // Sample diffuse/weathering texture (from TerrainDetails)
    float4 diffuseAlbedo = gDiffuseMap.Sample(gsamLinearWrap, pin.TexC);
    
    // Sample normal map if available
    float3 normalMapSample = gNormalMap.Sample(gsamLinearWrap, pin.TexC).rgb;
    
    // Normalize interpolated normal
    float3 normal = normalize(pin.NormalW);
    
    // If normal map is valid (not white), blend with calculated normal
    if (length(normalMapSample - 0.5) > 0.01)
    {
        float3 normalFromMap = normalMapSample * 2.0 - 1.0;
        normal = normalize(normal + normalFromMap * 0.5);
    }
    
    // Simple directional lighting
    float3 lightDir = normalize(-gLights[0].Direction);
    float NdotL = max(dot(normal, lightDir), 0.0);
    
    // Ambient + diffuse
    float3 ambient = gAmbientLight.rgb * 0.4;
    float3 diffuse = gLights[0].Strength * NdotL;
    
    // Height-based coloring (grass -> rock -> snow)
    float heightFactor = saturate((pin.Height - gMinHeight) / (gMaxHeight - gMinHeight));
    
    float3 grassColor = float3(0.25, 0.45, 0.15);
    float3 rockColor = float3(0.45, 0.40, 0.35);
    float3 snowColor = float3(0.95, 0.95, 0.98);
    
    // Slope-based blending (steeper = more rock)
    float slope = 1.0 - normal.y;
    
    float3 terrainColor;
    if (heightFactor < 0.3)
        terrainColor = lerp(grassColor, rockColor, slope * 2.0);
    else if (heightFactor < 0.6)
        terrainColor = lerp(grassColor, rockColor, saturate(heightFactor * 2.0 + slope));
    else
        terrainColor = lerp(rockColor, snowColor, saturate((heightFactor - 0.6) * 2.5));
    
    // Blend terrain color with loaded texture
    float3 finalColor;
    if (diffuseAlbedo.a > 0.1) // If texture is valid
        finalColor = lerp(terrainColor, diffuseAlbedo.rgb, 0.6);
    else
        finalColor = terrainColor;
    
    finalColor = ambient * finalColor + diffuse * finalColor;
    
    return float4(finalColor, 1.0);
}

// Wireframe pixel shader for debugging
float4 PS_Wireframe(VertexOut pin) : SV_Target
{
    // Color by LOD level
    float3 lodColors[5] = { 
        float3(1, 0, 0),    // LOD 0 - Red (highest detail)
        float3(0, 1, 0),    // LOD 1 - Green
        float3(0, 0, 1),    // LOD 2 - Blue
        float3(1, 1, 0),    // LOD 3 - Yellow
        float3(1, 0, 1)     // LOD 4 - Magenta (lowest detail)
    };
    
    return float4(lodColors[gLODLevel % 5], 1.0);
}
