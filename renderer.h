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

// TODO: Part 1C 
struct My_vertex
{
	float xyzw[4];

};

// TODO: Part 2B 

struct Shader_Vars
{
	GW::MATH::GMATRIXF Sworld_matrix;
	GW::MATH::GMATRIXF Sview_matrix;
	GW::MATH::GMATRIXF Sprojection_matrix;
};

// TODO: Part 2G 

class Renderer
{
	// proxy handles
	GW::SYSTEM::GWindow win;
	GW::GRAPHICS::GDirectX11Surface d3d;
	// what we need at a minimum to draw a triangle
	Microsoft::WRL::ComPtr<ID3D11Buffer>		vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11VertexShader>	vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>	pixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>	vertexFormat;

	

	// TODO: Part 2A 
	GW::MATH::GMATRIXF world_matrix;
	// TODO: Part 2C 
	Shader_Vars shader_vars;
	// TODO: Part 2D 
	Microsoft::WRL::ComPtr<ID3D11Buffer> constantbuffer;
	// TODO: Part 2G 
	GW::MATH::GMATRIXF view_matrix;
	GW::MATH::GMATRIXF camera_matrix;
	// TODO: Part 3A 
	GW::MATH::GMATRIXF projection_matrix;
	// TODO: Part 3C 
	std::vector<GW::MATH::GMATRIXF> gridMatrices;
	// TODO: Part 4A 
	GW::MATH::GMatrix matrixProxy;
	GW::INPUT::GInput GinputProxy;
	GW::INPUT::GController GControllerProxy;

	// timer
	XTime timer;
	float totaltime;

public:
	Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GDirectX11Surface _d3d)
	{
		timer.Restart();
		//proxy
		matrixProxy.Create();
		GinputProxy.Create(_win);
		GControllerProxy.Create();

		win = _win;
		d3d = _d3d;
		// TODO: Part 2A 
		InitializeMatrix();
		// TODO: Part 2C 
		shader_vars.Sworld_matrix = world_matrix;
		// TODO: Part 2G 
		shader_vars.Sview_matrix = view_matrix;
		// TODO: Part 3A 
		shader_vars.Sprojection_matrix = projection_matrix;
		// TODO: Part 3B 
		// TODO: Part 3C 
		// TODO: Part 4A 
		

		InitializeGraphics();


	}


private:
	//Constructor helper functions 
	void InitializeGraphics()
	{
		ID3D11Device* creator;
		d3d.GetDevice((void**)&creator);

		InitializeVertexBuffer(creator);
		//TODO: Part 2D 
		InitializeConstantBuffer(creator);

		InitializePipeline(creator);
		// free temporary handle
		creator->Release();
	}

	void InitializeMatrix()
	{

#pragma region grid Matrices initial
		// world_matrix initialing ground ---------> index 0
		GW::MATH::GMATRIXF temp;

		temp = GW::MATH::GIdentityMatrixF;
		matrixProxy.RotateXGlobalF(temp, 1.57, temp);
		GW::MATH::GVECTORF v = { 0,-0.5f,0,0 };
		GW::MATH::GMatrix::TranslateGlobalF(temp, v, temp);
		gridMatrices.push_back(temp);

		// world_matrix initialing ceiling ---------> index 1
		temp = GW::MATH::GIdentityMatrixF;
		matrixProxy.RotateXGlobalF(temp, 1.57, temp);
		v = { 0,+0.5f,0,0 };
		GW::MATH::GMatrix::TranslateGlobalF(temp, v, temp);
		gridMatrices.push_back(temp);

		// world_matrix initialing back ---------> index 2
		temp = GW::MATH::GIdentityMatrixF;
		 v = { 0,0,+0.5f,0 };
		GW::MATH::GMatrix::TranslateGlobalF(temp, v, temp);
		gridMatrices.push_back(temp);

		// world_matrix initialing front ---------> index 3
		temp = GW::MATH::GIdentityMatrixF;
		 v = { 0,0,-0.5f,0 };
		GW::MATH::GMatrix::TranslateGlobalF(temp, v, temp);
		gridMatrices.push_back(temp);

		// world_matrix initialing right ---------> index 4
		temp = GW::MATH::GIdentityMatrixF;
		matrixProxy.RotateYGlobalF(temp, 1.57, temp);
		 v = { +0.5f,0,0,0 };
		GW::MATH::GMatrix::TranslateGlobalF(temp, v, temp);
		gridMatrices.push_back(temp);

		// world_matrix initialing left ---------> index 5
		temp = GW::MATH::GIdentityMatrixF;
		matrixProxy.RotateYGlobalF(temp, 1.57, temp);
		 v = { -0.5f,0,0,0 };
		GW::MATH::GMatrix::TranslateGlobalF(temp, v, temp);
		gridMatrices.push_back(temp);

#pragma endregion

		world_matrix = gridMatrices[0];
		// view_matrix initialing
		view_matrix = GW::MATH::GIdentityMatrixF;
		GW::MATH::GVECTORF v1 = { 0.25f,-0.125f,-0.25f,0 };
		GW::MATH::GVECTORF v2 = { 0,-0.5,0,0 };
		GW::MATH::GVECTORF v3 = { 0,1,0,0 };
		/*GW::MATH::GMatrix::TranslateGlobalF(view_matrix, v1, view_matrix);
		GW::MATH::GMatrix::InverseF(view_matrix , view_matrix);*/ //don't know why this not working
		//matrixProxy--->LookAtLHF(v1, v2, v2 ,view_matrix); this way you can find the parameter
		matrixProxy.LookAtLHF(v1, v2, v3, view_matrix);
		matrixProxy.InverseF(view_matrix, camera_matrix);
		//( myposition ,target, which vector is Up)

		// projection matrix
		float aspectratio = 0 ;
		d3d.GetAspectRatio(aspectratio);
		projection_matrix = GW::MATH::GIdentityMatrixF;
		GW::MATH::GMatrix::ProjectionDirectXLHF(1.134f, aspectratio, 0.1f, 100, projection_matrix);
		//matrixProxy--->ProjectionDirectXLHF();
		//xmmatrixperspectiveFovLH(field of view , aspect ratio , near plane, far plane , out_matrix);
	}

	void InitializeVertexBuffer(ID3D11Device* creator)
	{
		// TODO: Part 1B 
		// TODO: Part 1C 
		// TODO: Part 1D 

		// triangle 
		/*float verts[] = {
			0,   0.5f, 0 ,1.0f,
			0.5f, -0.5f, 0 ,1.0f,
			0.5f, -0.5f, 0 ,1.0f,
			-0.5f, -0.5f, 0 ,1.0f,
			-0.5f, -0.5f, 0 ,1.0f,
			0,   0.5f, 0 ,1.0f
		};*/

		My_vertex* verts = Creategird();

		CreateVertexBuffer(creator, &verts[0], sizeof(float) * 4 * 104);
	}

	My_vertex* Creategird()
	{
		My_vertex* grid = new My_vertex[104];

		float x1 = -0.5f;
		float y1 = +0.5f;

		for (size_t i = 0; i < 52; i+=2)
		{
			grid[i].xyzw[0] = x1;
			grid[i].xyzw[1] = y1;
			grid[i].xyzw[2] = 0;
			grid[i].xyzw[3] = 1;
			
			y1 *= -1;
			grid[i+1].xyzw[0] = x1;
			grid[i+1].xyzw[1] = y1;
			grid[i+1].xyzw[3] = 1;
			grid[i+1].xyzw[2] = 0;

			x1 += 0.04;
			y1 *= -1;
		}



		x1 = -0.5f;
		y1 = +0.5f;

		for (size_t i = 0; i < 52; i+=2)
		{
			grid[i + 52].xyzw[0] = x1;
			grid[i + 52].xyzw[1] = y1;
			grid[i + 52].xyzw[2] = 0;
			grid[i + 52].xyzw[3] = 1;

			
			x1 *= -1;

			grid[i + 53].xyzw[0] = x1;
			grid[i + 53].xyzw[1] = y1;
			grid[i + 53].xyzw[2] = 0;
			grid[i + 53].xyzw[3] = 1;

			y1 -= 0.04;
			x1 *= -1;
		}

		return grid;
	}

	void InitializeConstantBuffer(ID3D11Device* creator)
	{

		Shader_Vars shader_V = shader_vars;

		CreateConstantBuffer(creator, &shader_V, sizeof(Shader_Vars) );
	}

	void CreateConstantBuffer(ID3D11Device* creator, const void* data, unsigned int sizeInBytes)
	{
		D3D11_SUBRESOURCE_DATA bData = { data, 0, 0 };
		CD3D11_BUFFER_DESC bDesc(sizeInBytes, D3D11_BIND_CONSTANT_BUFFER,D3D11_USAGE_DYNAMIC,D3D11_CPU_ACCESS_WRITE);
	 	APP_DEPRECATED_HRESULT hr = creator->CreateBuffer(&bDesc, &bData, constantbuffer.GetAddressOf());
	}

	void CreateVertexBuffer(ID3D11Device* creator, const void* data, unsigned int sizeInBytes)
	{
		D3D11_SUBRESOURCE_DATA bData = { data, 0, 0 };
		CD3D11_BUFFER_DESC bDesc(sizeInBytes, D3D11_BIND_VERTEX_BUFFER);
		APP_DEPRECATED_HRESULT hr = creator->CreateBuffer(&bDesc, &bData, vertexBuffer.GetAddressOf());
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
		// TODO: Part 1C 

		D3D11_INPUT_ELEMENT_DESC attributes[1];

		attributes[0].SemanticName = "POSITION";
		attributes[0].SemanticIndex = 0;
		attributes[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		attributes[0].InputSlot = 0;
		attributes[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		attributes[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		attributes[0].InstanceDataStepRate = 0;

		creator->CreateInputLayout(attributes, ARRAYSIZE(attributes),
			vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
			vertexFormat.GetAddressOf());


		// original float2 vertex
		/*D3D11_INPUT_ELEMENT_DESC attributes[1];

		attributes[0].SemanticName = "POSITION";
		attributes[0].SemanticIndex = 0;
		attributes[0].Format = DXGI_FORMAT_R32G32_FLOAT;
		attributes[0].InputSlot = 0;
		attributes[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		attributes[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		attributes[0].InstanceDataStepRate = 0;

		creator->CreateInputLayout(attributes, ARRAYSIZE(attributes),
			vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
			vertexFormat.GetAddressOf());*/

	}


public:
	void Render()
	{
		
		PipelineHandles curHandles = GetCurrentPipelineHandles();
		SetUpPipeline(curHandles);
		// TODO: Part 1B 
		// TODO: Part 1D 
		// TODO: Part 3D 
		//for (size_t i = 0; i < 1; i++)
		for (size_t i = 0; i < gridMatrices.size(); i++)
		{
			D3D11_MAPPED_SUBRESOURCE GPUbuffer;
			curHandles.context->Map(constantbuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &GPUbuffer);
			//*((GW::MATH::GMATRIXF*)(GPUbuffer.pData)) = gridMatrices[i];
			shader_vars.Sworld_matrix = gridMatrices[i];
			memcpy(GPUbuffer.pData, &shader_vars, sizeof(shader_vars));
			curHandles.context->Unmap(constantbuffer.Get(), 0);
			curHandles.context->Draw(104, 0);
		}
		
		
		ReleasePipelineHandles(curHandles);

		
	}


	// TODO: Part 4B 
	void UpdateCamera()
	{
		
		timer.Signal();
		totaltime = timer.Delta();
		float camera_speed = 0.3f;

		float perframeSpeed = camera_speed * totaltime;

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

		float rotateSpeed = 3.14f * totaltime * 7;

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
		

		matrixProxy.InverseF(camera_matrix, shader_vars.Sview_matrix);
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
		//TODO: Part 2E 
		handles.context->VSSetConstantBuffers(0,1, constantbuffer.GetAddressOf());
		handles.context->IASetInputLayout(vertexFormat.Get());
		handles.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST); //TODO: Part 1B 
	}

	void SetRenderTargets(PipelineHandles handles)
	{
		ID3D11RenderTargetView* const views[] = { handles.targetView };
		handles.context->OMSetRenderTargets(ARRAYSIZE(views), views, handles.depthStencil);
	}

	void SetVertexBuffers(PipelineHandles handles)
	{
		// TODO: Part 1C 
		const UINT strides[] = { sizeof(float) * 4 };
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
