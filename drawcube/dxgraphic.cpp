#pragma once
#include "DxGraphic.h"

DXGraphic::CDXGraphic::CDXGraphic()
{
}

DXGraphic::CDXGraphic::~CDXGraphic()
{
}

bool DXGraphic::CDXGraphic::CreateDeviceAndSwapChain(int w, int h)
{
	DXGI_SWAP_CHAIN_DESC desc = {
		.BufferDesc = { static_cast<UINT>(w), static_cast<UINT>(h), { 60, 1 }, swapchainformat, DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED, DXGI_MODE_SCALING_UNSPECIFIED },
		.SampleDesc = sampledesc,
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT,
		.BufferCount = swapchaincount,
		.OutputWindow = m_WindowHandle,
		.Windowed = TRUE,
		.SwapEffect = DXGI_SWAP_EFFECT_DISCARD,
		.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
	};

	if (FAILED(D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		0,
		0,
		nullptr,
		0,
		D3D11_SDK_VERSION,
		&desc,
		&swapchain.p,
		&device.p,
		&featurelevel,
		&context)))
		return false;

	return true;
}

bool DXGraphic::CDXGraphic::CreateRenderTarget()
{
	if (FAILED(swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backbuffer)))
		return false;

	if (FAILED(device->CreateRenderTargetView(backbuffer, nullptr, &rtv)))
		return false;

	return true;
}

bool DXGraphic::CDXGraphic::CreateDefaultRasterizerState()
{
	D3D11_RASTERIZER_DESC desc =
	{
		.FillMode = D3D11_FILL_SOLID,
		.CullMode = D3D11_CULL_BACK,
		.FrontCounterClockwise = TRUE,
		.DepthBias = 0,
		.DepthBiasClamp = 0.0f,
		.SlopeScaledDepthBias = 0.0f,
		.DepthClipEnable = TRUE,
		.ScissorEnable = FALSE,
		.MultisampleEnable = FALSE, 
		.AntialiasedLineEnable = FALSE
	};
	if (FAILED(device->CreateRasterizerState(&desc, &rs))) return false;

	return true;
}

bool DXGraphic::CDXGraphic::CreateDepthStencilState()
{
	D3D11_DEPTH_STENCIL_DESC desc =
	{
		.DepthEnable = TRUE,
		.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
		.DepthFunc = D3D11_COMPARISON_LESS,
		.StencilEnable = FALSE, 
		.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK,
		.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK,
		.FrontFace = {
			.StencilFailOp = D3D11_STENCIL_OP_KEEP,
			.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP,
			.StencilPassOp = D3D11_STENCIL_OP_KEEP,
			.StencilFunc = D3D11_COMPARISON_ALWAYS },
		.BackFace = {.StencilFailOp = D3D11_STENCIL_OP_KEEP,
			.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP,
			.StencilPassOp = D3D11_STENCIL_OP_KEEP,
			.StencilFunc = D3D11_COMPARISON_ALWAYS}
	};

	if (FAILED(device->CreateDepthStencilState(&desc, &dss))) return false;

	return true;
}

bool DXGraphic::CDXGraphic::CreateStencilBuffer(int w, int h)
{

	D3D11_TEXTURE2D_DESC texdesc =
	{
		.Width = static_cast<UINT>(w),
		.Height = static_cast<UINT>(h),
		.MipLevels = 1,
		.ArraySize = 1,
		.Format = DXGI_FORMAT_R24G8_TYPELESS,
		.SampleDesc = sampledesc,
		.Usage = D3D11_USAGE_DEFAULT,
		.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE
	};

	if (FAILED(device->CreateTexture2D(&texdesc, nullptr, &depthtex))) return false;


	D3D11_DEPTH_STENCIL_VIEW_DESC dsvdesc =
	{
		.Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
		.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D
	};

	if (FAILED(device->CreateDepthStencilView(depthtex, &dsvdesc, &dsv))) return false;

	return true;
}

bool DXGraphic::CDXGraphic::CreateShaderFromCompiledFiles()
{
	// std::wstring_view で文字列の先頭ポインタと長さだけを渡す
	auto WideStr2MultiByte = [](const std::wstring_view wstr) -> std::string
	{
		size_t size = ::WideCharToMultiByte(CP_OEMCP, 0, wstr.data(), -1, nullptr, 0, nullptr, nullptr);
		std::vector<char> buf;
		buf.resize(size);
		::WideCharToMultiByte(CP_OEMCP, 0, wstr.data(), -1, &buf.front(), static_cast<int>(size), nullptr, nullptr);
		std::string ret(&buf.front(), buf.size() - 1);
		return ret;
	};

	std::wstring filepath;
	filepath.resize(MAX_PATH);
	::GetModuleFileName(nullptr, &filepath.front(), MAX_PATH);
	::PathRemoveFileSpec(&filepath.front());

	// vertex shader
	std::string csofile = WideStr2MultiByte(filepath);
	csofile.append("\\vertexshader.cso");
	std::ifstream ifs(csofile, std::ios::in | std::ios::binary);
	if (ifs.fail()) return false;
	ifs.seekg(0, std::ifstream::end);
	size_t csosize = static_cast<size_t>(ifs.tellg());
	ifs.seekg(0, std::ifstream::beg);
	std::vector<char> csodata;
	csodata.resize(csosize);
	ifs.read(&csodata.front(), csosize);

	if (FAILED(device->CreateVertexShader(&csodata.front(), csosize, nullptr, &vertexshader.p)))
		return false;

	// 入力するデータのレイアウトを定義
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ 
			.SemanticName = "POSITION",
			.SemanticIndex = 0,
			.Format = DXGI_FORMAT_R32G32B32_FLOAT,
			.InputSlot = 0,
			.AlignedByteOffset = 0,
			.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
			.InstanceDataStepRate = 0
		},
		{
			.SemanticName = "COLOR",
			.SemanticIndex = 0,
			.Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
			.InputSlot = 0,
			.AlignedByteOffset = sizeof(float) * 3,
			.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
			.InstanceDataStepRate = 0 
		}
	};
	UINT num = ARRAYSIZE(layout);

	if (FAILED(device->CreateInputLayout(layout, num, &csodata.front(), csosize, &inputlayout.p)))
		return false;

	// geometry shader
	ifs.close();
	csodata.clear();
	csofile = WideStr2MultiByte(filepath);
	csofile.append("\\geometryshader.cso");
	ifs.open(csofile, std::ios::in | std::ios::binary);
	if (ifs.fail()) return false;
	ifs.seekg(0, std::ifstream::end);
	csosize = static_cast<size_t>(ifs.tellg());
	ifs.seekg(0, std::ifstream::beg);
	csodata.resize(csosize);
	ifs.read(&csodata.front(), csosize);

	if (FAILED(device->CreateGeometryShader(&csodata.front(), csosize, nullptr, &geometryshader.p)))
		return false;

	// pixel shader
	ifs.close();
	csofile = WideStr2MultiByte(filepath);
	csofile.append("\\pixelshader.cso");
	ifs.open(csofile, std::ios::in | std::ios::binary);
	if (ifs.fail())
		return false;
	ifs.seekg(0, std::ifstream::end);
	csosize = static_cast<size_t>(ifs.tellg());
	ifs.seekg(0, std::ifstream::beg);
	csodata.clear();
	csodata.resize(csosize);
	ifs.read(&csodata.front(), csosize);

	if (FAILED(device->CreatePixelShader(&csodata.front(), csosize, nullptr, &pixelshader.p)))
		return false;

	return true;
}

bool DXGraphic::CDXGraphic::CreateConstantBuffer()
{
	// TODO : Create CB
	D3D11_BUFFER_DESC matrixdesc =
	{
		sizeof(MatrixBuffer),
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_CONSTANT_BUFFER
	};

	if (FAILED(device->CreateBuffer(&matrixdesc, nullptr, &matrixbuffer)))
		return false;

	return true;
}

void DXGraphic::CDXGraphic::ReleaseComPtr()
{
	pixelshader.Release();
	geometryshader.Release();
	vertexshader.Release();
	inputlayout.Release();

	rs.Release();
	dss.Release();
	dsv.Release();
	depthtex.Release();

	rtv.Release();
	backbuffer.Release();

	swapchain.Release();
	context.Release();
	device.Release();
}

void DXGraphic::CDXGraphic::SetWindowHandle(HWND hWnd)
{
	m_WindowHandle = hWnd;
}

bool DXGraphic::CDXGraphic::InitD3D(int w, int h)
{
	if (w == 0 || h == 0)
		return false;

	if (!CreateDeviceAndSwapChain(w, h))
	{
		ReleaseComPtr();
		return false;
	}

	if (!CreateRenderTarget())
	{
		ReleaseComPtr();
		return false;
	}

	if (!CreateDefaultRasterizerState())
	{
		ReleaseComPtr();
		return false;
	}

	if (!CreateDepthStencilState())
	{
		ReleaseComPtr();
		return false;
	}

	if (!CreateStencilBuffer(w, h))
	{
		ReleaseComPtr();
		return false;
	}

	// レンダーターゲットに深度/ステンシルテクスチャを設定
	context->OMSetRenderTargets(1, &rtv.p, dsv);
	// ビューポートの設定
	D3D11_VIEWPORT vp[] =
	{
		{ 
			.TopLeftX = 0,
			.TopLeftY = 0, 
			.Width = static_cast<FLOAT>(w),
			.Height = static_cast<FLOAT>(h),
			.MinDepth = 0,
			.MaxDepth = 1.0f
		}
	};
	context->RSSetViewports(1, vp);

	if (!CreateShaderFromCompiledFiles())
	{
		ReleaseComPtr();
		return false;
	}

	if (!CreateConstantBuffer())
	{
		ReleaseComPtr();
		return false;
	}

	return true;
}

void DXGraphic::CDXGraphic::Render()
{
	UINT strides = sizeof(CoordColor);
	UINT offset = 0;

	if (context == nullptr) return;

	// バックバッファと深度バッファのクリア
	FLOAT backcolor[4] = { 1.f, 1.f, 1.f, 1.f };
	context->ClearRenderTargetView(rtv, backcolor);
	context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// 頂点データに渡すデータのレイアウトを設定
	context->IASetInputLayout(inputlayout);

	// 頂点シェーダー, ジオメトリシェーダー, ピクセルシェーダーの設定
	context->VSSetShader(vertexshader, nullptr, 0);
	context->GSSetShader(geometryshader, nullptr, 0);
	context->PSSetShader(pixelshader, nullptr, 0);

	// ラスタライザーステートを設定
	context->RSSetState(rs);

	MatrixBuffer matrixbuf = {
		// シェーダーでは列優先(column_major)で行列データを保持するため, 転置を行う
		DirectX::XMMatrixTranspose(d3dprojmatrix),
		DirectX::XMMatrixTranspose(d3dviewmatrix),
		DirectX::XMMatrixTranspose(d3dworldmatrix)
	};

	// マトリックスバッファの設定
	context->UpdateSubresource(matrixbuffer, 0, nullptr, &matrixbuf, 0, 0);
	context->VSSetConstantBuffers(0, 1, &matrixbuffer.p);
	context->GSSetConstantBuffers(0, 1, &matrixbuffer.p);


	// 深度・ステンシルバッファの使用方法を設定
	context->OMSetDepthStencilState(dss, 0);

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetVertexBuffers(0, 1, &vertexbuffer.p, &strides, &offset);
	context->IASetIndexBuffer(indexbuffer, DXGI_FORMAT_R32_UINT, 0);
	context->DrawIndexed(numindices, 0, 0);

	// 作成したプリミティブをウィンドウへ描画
	if (swapchain != nullptr)
		swapchain->Present(0, 0);
}

void DXGraphic::CDXGraphic::LoadSampleData(int w, int h)
{
	float nearz = 1 / 1000.0f;
	float farz = 10.0f;

	d3dprojmatrix = DirectX::XMMatrixPerspectiveFovRH(std::numbers::pi_v<float> / 4.0f, 1.0f * w / h, nearz, farz);

	float upsetz = m_camupset ? -1.0f : 1.0f;

	DirectX::XMVECTOR eye = DirectX::XMVectorSet(m_cameraposition[0], m_cameraposition[1], m_cameraposition[2], 0.0f);
	DirectX::XMVECTOR focus = DirectX::XMVectorSet(m_lookatpoint[0], m_lookatpoint[1], m_lookatpoint[2], 0.0f);
	DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 0.0f, upsetz, 0.0f);

	d3dviewmatrix = DirectX::XMMatrixLookAtRH(eye, focus, up);

	std::vector<float> vertexarray;
	for (const Vertex& v : InputData)
	{
		vertexarray.push_back(v.position[0]);
		vertexarray.push_back(v.position[1]);
		vertexarray.push_back(v.position[2]);

		vertexarray.push_back(v.color[0]);
		vertexarray.push_back(v.color[1]);
		vertexarray.push_back(v.color[2]);
		vertexarray.push_back(v.color[3]);
	}


	std::vector<int> indexarray;
	for (const Triangle& tri : InputTriangle)
	{
		indexarray.push_back(tri.indices[0]);
		indexarray.push_back(tri.indices[1]);
		indexarray.push_back(tri.indices[2]);
	}
	numindices = static_cast<UINT>(indexarray.size());

	vertexbuffer.Release();
	D3D11_BUFFER_DESC bdvertex =
	{
		.ByteWidth = static_cast<UINT>(sizeof(float) * vertexarray.size()),
		.Usage = D3D11_USAGE_DEFAULT,
		.BindFlags = D3D11_BIND_VERTEX_BUFFER
	};
	D3D11_SUBRESOURCE_DATA srdv = { &vertexarray.front() };
	device->CreateBuffer(&bdvertex, &srdv, &vertexbuffer.p);

	indexbuffer.Release();
	D3D11_BUFFER_DESC bdindex =
	{
		.ByteWidth = static_cast<UINT>(sizeof(int) * indexarray.size()),
		.Usage = D3D11_USAGE_DEFAULT,
		.BindFlags = D3D11_BIND_INDEX_BUFFER
	};
	D3D11_SUBRESOURCE_DATA srdind = { &indexarray.front() };
	device->CreateBuffer(&bdindex, &srdind, &indexbuffer.p);

	Render();
}

void DXGraphic::CDXGraphic::UpdateMatrices(int w, int h)
{
	if (w == 0 || h == 0) return;

	float nearz = 1 / 1000.0f;
	float farz = 10.0f;

	d3dprojmatrix = DirectX::XMMatrixPerspectiveFovRH(std::numbers::pi_v<float> / 4.0f, 1.0f * w / h, nearz, farz);
}

bool DXGraphic::CDXGraphic::ResizeView(int w, int h)
{
	if (w == 0 || h == 0 || device == nullptr)
		return false;


	ID3D11RenderTargetView* irtv = nullptr;
	context->OMSetRenderTargets(1, &irtv, nullptr);
	rtv.Release();
	backbuffer.Release();

	dsv.Release();
	depthtex.Release();

	if (FAILED(swapchain->ResizeBuffers(swapchaincount, w, h, swapchainformat, 0)))
		return false;

	if (!CreateRenderTarget())
	{
		ReleaseComPtr();
		return false;
	}

	if (!CreateStencilBuffer(w, h))
	{
		ReleaseComPtr();
		return false;
	}

	// レンダーターゲットに深度/ステンシルテクスチャを設定
	context->OMSetRenderTargets(1, &rtv.p, dsv);
	// ビューポートの設定
	D3D11_VIEWPORT vp[] =
	{
		{
			.TopLeftX = 0,
			.TopLeftY = 0,
			.Width = static_cast<FLOAT>(w),
			.Height = static_cast<FLOAT>(h),
			.MinDepth = 0,
			.MaxDepth = 1.0f
		}
	};
	context->RSSetViewports(1, vp);

	return true;
}

void DXGraphic::CDXGraphic::CameraRotateZ(float delta)
{
	DirectX::XMMATRIX rotZ = DirectX::XMMatrixRotationZ(delta);
	DirectX::XMVECTOR eye = DirectX::XMVectorSet(m_cameraposition[0], m_cameraposition[1], m_cameraposition[2], 0.0f);

	eye = DirectX::XMVector3Transform(eye, rotZ);
	m_cameraposition[0] = DirectX::XMVectorGetX(eye);
	m_cameraposition[1] = DirectX::XMVectorGetY(eye);
	m_cameraposition[2] = DirectX::XMVectorGetZ(eye);

	float upsetz = m_camupset ? -1.0f : 1.0f;
	DirectX::XMVECTOR focus = DirectX::XMVectorSet(m_lookatpoint[0], m_lookatpoint[1], m_lookatpoint[2], 0.0f);
	DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 0.0f, upsetz, 0.0f);

	d3dviewmatrix = DirectX::XMMatrixLookAtRH(eye, focus, up);
}
