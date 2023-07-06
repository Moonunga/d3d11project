// an ultra simple hlsl pixel shader

struct InputVertex
{
	float4 pos : SV_POSITION;
};


float4 main() : SV_TARGET
{
	return float4(0.0f, 0.0f, 1.0f, 1.0f); // TODO: Part 1A (optional) 
}