// an ultra simple hlsl vertex shader
#pragma pack_matrix(row_major) 


struct OutputVertex
{
    float4 xyzw : SV_POSITION;
    float3 posW : WORLD;
    float3 normW: NORMALW;
};

struct InputVertex
{
    float3 xyz : POSITION;
    float3 uvw : TEXTUREUV;
    float3 nrm : NORMAL;
};



struct ATTRIBUTES
{
    float3 Kd; // diffuse reflectivity
    float d; // dissolve (transparency) 
    float3 Ks; // specular reflectivity
    float Ns; // specular exponent
    float3 Ka; // ambient reflectivity
    float sharpness; // local reflection map sharpness
    float3 Tf; // transmission filter
    float Ni; // optical density (index of refraction)
    float3 Ke; // emissive reflectivity
    uint illum; // illumination model
};


cbuffer SceneBuffer : register(b0)
{
    float4x4 view_matrix;
    float4x4 projection_matrix;
    float4 sunDirection;
    float4 sunColor;
};

cbuffer MeshBuffer : register(b1)
{
    float4x4 world_matrix[200];
    ATTRIBUTES attributes[200];
};

cbuffer ObjectID : register(b2)
{
    int model_id;
    int material_id;
    int dummy1;
    int dummy2;
};



OutputVertex main(InputVertex input, uint instanceId : SV_InstanceID  )
{
    OutputVertex output = (OutputVertex) 0;
    output.xyzw = float4(input.xyz, 1);
    
    output.xyzw = mul(output.xyzw, world_matrix[ model_id + instanceId]);
    output.posW = output.xyzw.xyz;
    output.xyzw = mul(output.xyzw, view_matrix);
    output.xyzw = mul(output.xyzw, projection_matrix);
    
    //output.normW = mul(input.nrm, (float3x3) (world_matrix[instanceId]));
    
    return output;
}