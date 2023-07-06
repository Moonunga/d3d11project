// an ultra simple hlsl vertex shader
#pragma pack_matrix(row_major) 
// TODO: Part 1C
struct OutputVertex
{
	float4 xyzw : SV_POSITION;
};

struct InputVertex
{
	float4 xyzw : POSITION;
};

// TODO: Part 2B 
cbuffer Shader_Vars : register(b0)
{
	float4x4 world_matrix;
	float4x4 view_matrix;
	float4x4 projection_matrix;
};

// TODO: Part 2F 
// TODO: Part 2G 
// TODO: Part 3B 

OutputVertex main(InputVertex input )
{
	OutputVertex output = (OutputVertex)0;
	output.xyzw = input.xyzw;

	output.xyzw = mul(output.xyzw , world_matrix);
	output.xyzw = mul(output.xyzw, view_matrix);
	output.xyzw = mul(output.xyzw, projection_matrix);
 	return output;

}