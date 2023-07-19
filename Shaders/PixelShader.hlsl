// an ultra simple hlsl pixel shader

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


struct InputVertex
{
    float4 xyzw : SV_POSITION;
    float3 posW : WORLD;
    float3 normW: NORMALW;
};


float4 main(InputVertex input) : SV_TARGET
{
    //return float4(input.normW, 1);
    return float4(attributes[material_id].Kd, 0);
}