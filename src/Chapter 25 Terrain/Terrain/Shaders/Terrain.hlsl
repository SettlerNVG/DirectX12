//***************************************************************************************
// Terrain.hlsl - Terrain rendering with heightmap displacement
//***************************************************************************************

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
    float2 HeightmapUV : TEXCOORD1;
    float Height : HEIGHT;
};

// Sample height from heightmap (UV in [0,1])
float SampleHeight(float2 uv)
{
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
    normal.y = 2.0 * gTerrainSize / gHeightMapSize.x;
    normal.z = hD - hU;
    
    return normalize(normal);
}

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    
    // vin.TexC is already [0,1] from mesh generation
    // Use it directly for heightmap sampling
    float2 heightmapUV = vin.TexC;
    
    // Sample height
    float height = SampleHeight(heightmapUV);
    
    // Local position: mesh is [-0.5, 0.5], scale by world matrix
    float3 posL = vin.PosL;
    posL.y = 0; // Height will be set after world transform
    
    // Transform to world space
    float4 posW = mul(float4(posL, 1.0f), gWorld);
    
    // Apply height in world space
    posW.y = height;
    
    vout.PosW = posW.xyz;
    vout.Height = height;
    vout.HeightmapUV = heightmapUV;
    
    // Calculate normal from heightmap
    vout.NormalW = CalculateNormal(heightmapUV);
    
    // Transform to clip space
    vout.PosH = mul(posW, gViewProj);
    
    // Texture UV - same as heightmap UV (covers entire terrain)
    vout.TexC = heightmapUV;
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    // Sample textures
    float4 diffuseAlbedo = gDiffuseMap.Sample(gsamLinearWrap, pin.TexC);
    float3 normalMapSample = gNormalMap.Sample(gsamLinearWrap, pin.TexC).rgb;
    
    float3 normal = normalize(pin.NormalW);
    
    // Blend normal map if valid
    if (length(normalMapSample - 0.5) > 0.01)
    {
        float3 normalFromMap = normalMapSample * 2.0 - 1.0;
        normal = normalize(normal + normalFromMap * 0.3);
    }
    
    // Lighting
    float3 lightDir = normalize(-gLights[0].Direction);
    float NdotL = max(dot(normal, lightDir), 0.0);
    
    float3 ambient = gAmbientLight.rgb * 0.5;
    float3 diffuse = gLights[0].Strength * NdotL;
    
    // Height-based terrain coloring
    float heightFactor = saturate((pin.Height - gMinHeight) / (gMaxHeight - gMinHeight));
    float slope = 1.0 - normal.y;
    
    float3 grassColor = float3(0.3, 0.5, 0.2);
    float3 rockColor = float3(0.5, 0.45, 0.4);
    float3 snowColor = float3(0.95, 0.95, 0.98);
    
    float3 terrainColor;
    if (heightFactor < 0.4)
        terrainColor = lerp(grassColor, rockColor, saturate(slope * 3.0));
    else if (heightFactor < 0.7)
        terrainColor = lerp(grassColor, rockColor, saturate(heightFactor + slope));
    else
        terrainColor = lerp(rockColor, snowColor, saturate((heightFactor - 0.7) * 3.0));
    
    // Blend with diffuse texture
    float3 finalColor = terrainColor;
    if (diffuseAlbedo.a > 0.1)
        finalColor = lerp(terrainColor, diffuseAlbedo.rgb, 0.5);
    
    finalColor = (ambient + diffuse) * finalColor;
    
    return float4(finalColor, 1.0);
}

// Wireframe shader - shows LOD level by color
float4 PS_Wireframe(VertexOut pin) : SV_Target
{
    float3 lodColors[5] = { 
        float3(1, 0, 0),    // LOD 0 - Red
        float3(0, 1, 0),    // LOD 1 - Green
        float3(0, 0, 1),    // LOD 2 - Blue
        float3(1, 1, 0),    // LOD 3 - Yellow
        float3(1, 0, 1)     // LOD 4 - Magenta
    };
    
    return float4(lodColors[gLODLevel % 5], 1.0);
}
