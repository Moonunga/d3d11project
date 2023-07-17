#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib") //needed for runtime shader compilation. Consider compiling shaders before runtime 
#include"XTime.h"

void PrintLabeledDebugString(const char* label, const char* toPrint)
{
	std::cout << label << toPrint << std::endl;
#if defined WIN32 //OutputDebugStringA is a windows-only function 
	OutputDebugStringA(label);
	OutputDebugStringA(toPrint);
#endif
}

enum constantMode
{
	scene,
	mesh
};

struct SceneData
{
	GW::MATH::GMATRIXF viewMatrix, projectionMatrix;
	GW::MATH::GVECTORF sunDirection, sunColor;
};

struct MeshData
{
	GW::MATH::GMATRIXF worldMatrix;
	H2B::MATERIAL material;
};

class Renderer
{
	// handle level data
	Level_Data& levelHandle;

	GW::SYSTEM::GWindow win;
	GW::GRAPHICS::GDirectX11Surface d3d;

	Microsoft::WRL::ComPtr<ID3D11Buffer>		vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11VertexShader>	vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>	pixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>	vertexFormat;
	Microsoft::WRL::ComPtr<ID3D11Buffer>		indexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer>		constantbuffer[2];
	

	GW::MATH::GMATRIXF world_matrix;
	GW::MATH::GMATRIXF view_matrix;
	GW::MATH::GMATRIXF camera_matrix;
	GW::MATH::GMATRIXF projection_matrix;
	GW::MATH::GVECTORF sun_Direction;
	GW::MATH::GVECTORF sun_Color;

	//proxy
	GW::MATH::GMatrix matrixProxy;
	GW::INPUT::GInput GinputProxy;
	GW::INPUT::GController GControllerProxy;

	SceneData scene_Data;
	MeshData  mesh_Data;
	// timer
	XTime timer;
	float deltatime;


public:
	Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GDirectX11Surface _d3d ,
				Level_Data& _handle) : levelHandle(_handle) //constuctor intializer
	{
		//xtime 
		timer.Restart();
		//proxy create
		matrixProxy.Create();
		GinputProxy.Create(_win);
		GControllerProxy.Create();

		win = _win;
		d3d = _d3d;
		
		InitializeMatrix();
		InitializeSun();

		scene_Data.viewMatrix = view_matrix;
		scene_Data.projectionMatrix = projection_matrix;
		scene_Data.sunColor = sun_Color;
		scene_Data.sunDirection = sun_Direction;

		mesh_Data.worldMatrix = world_matrix;
		//mesh_Data.material = FSLogo_materials[0].attrib;
		
		InitializeGraphics();
	}


private:
	//Constructor helper functions 
	void InitializeMatrix()
	{
		world_matrix = GW::MATH::GIdentityMatrixF;

		// view_matrix initialing
		view_matrix = GW::MATH::GIdentityMatrixF;
		GW::MATH::GVECTORF v1 = { 0.25f,-0.125f,-0.25f,0 };
		GW::MATH::GVECTORF v2 = { 0,-0.5,0,0 };
		GW::MATH::GVECTORF v3 = { 0,1,0,0 };
		/*GW::MATH::GMatrix::TranslateGlobalF(view_matrix, v1, view_matrix);
		GW::MATH::GMatrix::InverseF(view_matrix , view_matrix);*/ //don't know why this not working
		//matrixProxy--->LookAtLHF(v1, v2, v2 ,view_matrix); this way you can find the parameter
		matrixProxy.LookAtLHF(v1, v2, v3, view_matrix);
							//( myposition ,target, which vector is Up)

		matrixProxy.InverseF(view_matrix, camera_matrix);
					

		// projection matrix
		float aspectratio = 0 ;
		d3d.GetAspectRatio(aspectratio);
		projection_matrix = GW::MATH::GIdentityMatrixF;
		GW::MATH::GMatrix::ProjectionDirectXLHF(1.134f, aspectratio, 0.1f, 100, projection_matrix);
		//matrixProxy--->ProjectionDirectXLHF();
		//xmmatrixperspectiveFovLH(field of view , aspect ratio , near plane, far plane , out_matrix);
	}

	void InitializeSun()
	{
		sun_Color = { 0.9f ,0.9f ,1.0f ,1.0f };
		sun_Direction = { -1, -1, +2, 1 };
	}

	void InitializeGraphics()
	{
		ID3D11Device* creator;
		d3d.GetDevice((void**)&creator);

		InitializeVertexBuffer(creator);
		InitializeIndexBuffer(creator);
		InitializeConstantBuffer(creator);
		InitializePipeline(creator);

		// free temporary handle
		creator->Release();
	}

	void InitializeVertexBuffer(ID3D11Device* creator)
	{
		CreateVertexBuffer(creator, levelHandle.levelVertices.data() , sizeof(H2B::VERTEX) * levelHandle.levelVertices.size());
	}

	void CreateVertexBuffer(ID3D11Device* creator, const void* data, unsigned int sizeInBytes)
	{
		D3D11_SUBRESOURCE_DATA bData = { data, 0, 0 };
		CD3D11_BUFFER_DESC bDesc(sizeInBytes, D3D11_BIND_VERTEX_BUFFER);
		APP_DEPRECATED_HRESULT hr = creator->CreateBuffer(&bDesc, &bData, vertexBuffer.GetAddressOf());
	}

	void InitializeIndexBuffer(ID3D11Device* creator)
	{
		CreateIndexBuffer(creator, levelHandle.levelIndices.data(), sizeof(unsigned int) * levelHandle.levelIndices.size());
	}

	void CreateIndexBuffer(ID3D11Device* creator, const void* data, unsigned int sizeInBytes)
	{
		D3D11_SUBRESOURCE_DATA bData = { data, 0, 0 };
		CD3D11_BUFFER_DESC bDesc(sizeInBytes, D3D11_BIND_INDEX_BUFFER);
		creator->CreateBuffer(&bDesc, &bData, indexBuffer.GetAddressOf());
	}


	void InitializeConstantBuffer(ID3D11Device* creator)
	{

		SceneData scene = scene_Data;
		CreateConstantBuffer(creator, &scene, sizeof(SceneData), constantMode::scene );
		MeshData mesh = mesh_Data;
		CreateConstantBuffer(creator, &scene, sizeof(MeshData), constantMode::mesh);

	}

	void CreateConstantBuffer(ID3D11Device* creator, const void* data, unsigned int sizeInBytes , constantMode mode)
	{
		D3D11_SUBRESOURCE_DATA bData = { data, 0, 0 };
		CD3D11_BUFFER_DESC bDesc(sizeInBytes, D3D11_BIND_CONSTANT_BUFFER,D3D11_USAGE_DYNAMIC,D3D11_CPU_ACCESS_WRITE);
	 	APP_DEPRECATED_HRESULT hr = creator->CreateBuffer(&bDesc, &bData, constantbuffer[(int)mode].GetAddressOf());
	}

	

	void InitializePipeline(ID3D11Device* creator)
	{
		UINT compilerFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if _DEBUG
		compilerFlags |= D3DCOMPILE_DEBUG;
#endif
		Microsoft::WRL::ComPtr<ID3DBlob> vsBlob = CompileVertexShader(creator, compilerFlags);
		Microsoft::WRL::ComPtr<ID3DBlob> psBlob = CompilePixelShader(creator, compilerFlags);
		CreateVertexInputLayout(creator, vsBlob);
	}

	Microsoft::WRL::ComPtr<ID3DBlob> CompileVertexShader(ID3D11Device* creator, UINT compilerFlags)
	{
		std::string vertexShaderSource = ReadFileIntoString("../Shaders/VertexShader.hlsl");

		Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, errors;

		HRESULT compilationResult =
			D3DCompile(vertexShaderSource.c_str(), vertexShaderSource.length(),
				nullptr, nullptr, nullptr, "main", "vs_4_0", compilerFlags, 0,
				vsBlob.GetAddressOf(), errors.GetAddressOf());

		if (SUCCEEDED(compilationResult))
		{
			creator->CreateVertexShader(vsBlob->GetBufferPointer(),
				vsBlob->GetBufferSize(), nullptr, vertexShader.GetAddressOf());
		}
		else
		{
			PrintLabeledDebugString("Vertex Shader Errors:\n", (char*)errors->GetBufferPointer());
			abort();
			return nullptr;
		}

		return vsBlob;
	}

	Microsoft::WRL::ComPtr<ID3DBlob> CompilePixelShader(ID3D11Device* creator, UINT compilerFlags)
	{
		std::string pixelShaderSource = ReadFileIntoString("../Shaders/PixelShader.hlsl");

		Microsoft::WRL::ComPtr<ID3DBlob> psBlob, errors;

		HRESULT compilationResult =
			D3DCompile(pixelShaderSource.c_str(), pixelShaderSource.length(),
				nullptr, nullptr, nullptr, "main", "ps_4_0", compilerFlags, 0,
				psBlob.GetAddressOf(), errors.GetAddressOf());

		if (SUCCEEDED(compilationResult))
		{
			creator->CreatePixelShader(psBlob->GetBufferPointer(),
				psBlob->GetBufferSize(), nullptr, pixelShader.GetAddressOf());
		}
		else
		{
			PrintLabeledDebugString("Pixel Shader Errors:\n", (char*)errors->GetBufferPointer());
			abort();
			return nullptr;
		}

		return psBlob;

	}

	void CreateVertexInputLayout(ID3D11Device* creator, Microsoft::WRL::ComPtr<ID3DBlob>& vsBlob)
	{

		D3D11_INPUT_ELEMENT_DESC attributes[3];

		attributes[0].SemanticName = "POSITION";
		attributes[0].SemanticIndex = 0;
		attributes[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		attributes[0].InputSlot = 0;
		attributes[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		attributes[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		attributes[0].InstanceDataStepRate = 0;

		//textureUV input setting
		attributes[1].SemanticName = "TEXTUREUV";
		attributes[1].SemanticIndex = 0;
		attributes[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		attributes[1].InputSlot = 0;
		attributes[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		attributes[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		attributes[1].InstanceDataStepRate = 0;

		//textureUV input setting
		attributes[2].SemanticName = "NORMAL";
		attributes[2].SemanticIndex = 0;
		attributes[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		attributes[2].InputSlot = 0;
		attributes[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		attributes[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		attributes[2].InstanceDataStepRate = 0;

		creator->CreateInputLayout(attributes, ARRAYSIZE(attributes),
			vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
			vertexFormat.GetAddressOf());

	}


public:
	void Render()
	{
		
		PipelineHandles curHandles = GetCurrentPipelineHandles();
		SetUpPipeline(curHandles);
		
		for (size_t i = 0; i < levelHandle.levelModels.size(); i++)
		{
			for (size_t j = 0; j < levelHandle.levelMeshes.size(); j++)
			{
				D3D11_MAPPED_SUBRESOURCE GPUbuffer;
				curHandles.context->Map(constantbuffer[mesh].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &GPUbuffer);
				//*((GW::MATH::GMATRIXF*)(GPUbuffer.pData)) = gridMatrices[i];
				//matrixProxy--->IdentityF(mesh_Data.worldMatrix); // change world matrix each here
				mesh_Data.material.attrib = levelHandle.levelMaterials[levelHandle.levelMeshes[j].materialIndex].attrib;
				memcpy(GPUbuffer.pData, constantbuffer[mesh].GetAddressOf(), sizeof(constantbuffer[mesh]));
				curHandles.context->Unmap(constantbuffer[mesh].Get(), 0);
				curHandles.context->DrawIndexedInstanced(levelHandle.levelMeshes[j].drawInfo.indexCount, levelHandle.levelMeshes[j].drawInfo.indexOffset, 0);
			}
		}
			
		
		
		
		ReleasePipelineHandles(curHandles);

		
	}

	void UpdateCamera()
	{
		
		timer.Signal();
		deltatime = timer.Delta();
		float camera_speed = 0.3f;

		float perframeSpeed = camera_speed * deltatime;

		// up down
		float space_state = 0;
		float shift_state = 0;
		GinputProxy.GetState(G_KEY_SPACE, space_state);
		GinputProxy.GetState(G_KEY_LEFTSHIFT, shift_state);
		if (space_state == 1 || shift_state == 1)
		{
			
			float total_y_change = space_state - shift_state;
			camera_matrix.row4.y += total_y_change * perframeSpeed;
			
		}
		
		//forward backward
		float w_state = 0;
		float s_state = 0;
		//right left 
		float a_state = 0;
		float d_state = 0;

		GinputProxy.GetState(G_KEY_W, w_state);
		GinputProxy.GetState(G_KEY_S, s_state);
		GinputProxy.GetState(G_KEY_A, a_state);
		GinputProxy.GetState(G_KEY_D, d_state);


		if (w_state == 1 || s_state == 1 || a_state|| d_state)
		{
			float total_z_change = w_state - s_state;
			float total_x_change = d_state - a_state;

			GW::MATH::GMATRIXF temp = GW::MATH::GIdentityMatrixF; 
			GW::MATH::GVECTORF v = { total_x_change * perframeSpeed, 0, total_z_change * perframeSpeed ,0 };
			GW::MATH::GMatrix::TranslateGlobalF(temp, v, temp);
			matrixProxy.MultiplyMatrixF(temp, camera_matrix, camera_matrix);
		}
		

		//////mouse rotate up and down
		float ydelta = 0;
		float xdelta = 0;
		GinputProxy.GetMouseDelta(xdelta, ydelta);

		UINT windowHeight;
		win.GetHeight(windowHeight);

		UINT windowWidth;
		win.GetWidth(windowWidth);

		float rotateSpeed = 3.14f * deltatime * 7;

		float total_pitch = 1.134f * ydelta / windowHeight * rotateSpeed;
							//(fov in radian)

		{
			GW::MATH::GMATRIXF temp = GW::MATH::GIdentityMatrixF;
			matrixProxy.RotateXGlobalF(temp, total_pitch, temp);
			matrixProxy.MultiplyMatrixF(temp, camera_matrix, camera_matrix);
		}
		
		////////////mouse left and right

		{
			float aspectratio = 0;
			d3d.GetAspectRatio(aspectratio);

			float total_yaw = 1.134f * aspectratio * xdelta / windowWidth * rotateSpeed;

			GW::MATH::GMATRIXF temp = GW::MATH::GIdentityMatrixF;
			matrixProxy.RotateYGlobalF(temp, total_yaw, temp);
			matrixProxy.MultiplyMatrixF(camera_matrix, temp, camera_matrix);

		}
		

		matrixProxy.InverseF(camera_matrix, scene_Data.viewMatrix);
	}

	
	// TODO: Part 4C 
	// TODO: Part 4D 
	// TODO: Part 4E 
	// TODO: Part 4F 
	// TODO: Part 4G 

private:
	struct PipelineHandles
	{
		ID3D11DeviceContext* context;
		ID3D11RenderTargetView* targetView;
		ID3D11DepthStencilView* depthStencil;
	};
	//Render helper functions
	PipelineHandles GetCurrentPipelineHandles()
	{
		PipelineHandles retval;
		d3d.GetImmediateContext((void**)&retval.context);
		d3d.GetRenderTargetView((void**)&retval.targetView);
		d3d.GetDepthStencilView((void**)&retval.depthStencil);
		return retval;
	}

	void SetUpPipeline(PipelineHandles handles)
	{
		SetRenderTargets(handles);
		SetVertexBuffers(handles);
		SetShaders(handles);
		SetIndexBuffers(handles);

		handles.context->VSSetConstantBuffers(0,2, constantbuffer->GetAddressOf());
		handles.context->PSSetConstantBuffers(0, 2, constantbuffer->GetAddressOf());
		handles.context->IASetInputLayout(vertexFormat.Get());
		handles.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST); //TODO: Part 1B 
	}

	void SetRenderTargets(PipelineHandles handles)
	{
		ID3D11RenderTargetView* const views[] = { handles.targetView };
		handles.context->OMSetRenderTargets(ARRAYSIZE(views), views, handles.depthStencil);
	}

	void SetIndexBuffers(PipelineHandles handles)
	{
		ID3D11Buffer* buffs = indexBuffer.Get();
		handles.context->IASetIndexBuffer(buffs, DXGI_FORMAT_R32_UINT, 0);
	}

	void SetVertexBuffers(PipelineHandles handles)
	{
		const UINT strides[] = { sizeof(float) * 9 };
		const UINT offsets[] = { 0 };
		ID3D11Buffer* const buffs[] = { vertexBuffer.Get() };
		handles.context->IASetVertexBuffers(0, ARRAYSIZE(buffs), buffs, strides, offsets);
	}

	void SetShaders(PipelineHandles handles)
	{
		handles.context->VSSetShader(vertexShader.Get(), nullptr, 0);
		handles.context->PSSetShader(pixelShader.Get(), nullptr, 0);
	}

	void ReleasePipelineHandles(PipelineHandles toRelease)
	{
		toRelease.depthStencil->Release();
		toRelease.targetView->Release();
		toRelease.context->Release();
	}


public:
	~Renderer()
	{
		// ComPtr will auto release so nothing to do here yet
	}
};
