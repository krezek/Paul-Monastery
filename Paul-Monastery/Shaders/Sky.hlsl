#include "Common.hlsl"


VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut)0.0f;

	// Use local vertex position as cubemap lookup vector.
    vout.PosW = vin.PosL;
	
	// Transform to world space.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);

	// Always center sky about camera.
    posW.xyz += gEyePosW;

	// Set z = w so that z/w = 1 (i.e., skydome always on far plane).
    vout.PosH = mul(posW, gViewProj).xyww;
	
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    return gCubeMap.Sample(gsamLinearWrap, pin.PosW);
}
