#include "pch.h"
#include "platform.h"


#include <GraphicsWindow.h>
#include <WUtil.h>
#include <d3dUtil.h>
#include <FrameResource.h>
#include <DDSTextureLoader.h>

using namespace DirectX;

#define TEXTURE_PATH L"Textures\\"
#define SHADER_PATH L"Shaders\\"
#define MODELS_PATH L"Models\\"

//#define TEXTURE_PATH L"\\ProgramData\\rezek\\"
//#define SHADER_PATH L"\\ProgramData\\rezek\\"
//#define MODELS_PATH L"\\ProgramData\\rezek\\"

const int gNumFrameResources = 3;

LRESULT GraphicsWindow::OnCreate()
{
	return 0;
}

void GraphicsWindow::InitDirect3D()
{
	AbstractWindow::InitDirect3D();

	ThrowIfFailed(_CommandList->Reset(_DirectCmdListAlloc.Get(), nullptr));

	_CbvSrvDescriptorSize = _d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	LoadTextures();
	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildBoardGeometry();
	BuildMaterials();
	BuildRenderItems();
	BuildFrameResources();
	BuildDescriptorHeaps();
	BuildPSOs();

	ThrowIfFailed(_CommandList->Close());
	ID3D12CommandList* cmdsLists[] = { _CommandList.Get() };
	_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();
}

void GraphicsWindow::Draw()
{
	auto cmdListAlloc = _CurrFrameResource->CmdListAlloc;

	ThrowIfFailed(cmdListAlloc->Reset());

	ThrowIfFailed(_CommandList->Reset(cmdListAlloc.Get(), _PSOs["opaque"].Get()));

	_CommandList->RSSetViewports(1, &_ScreenViewport);
	_CommandList->RSSetScissorRects(1, &_ScissorRect);

	_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	_CommandList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::LightSteelBlue, 0, nullptr);
	_CommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	_CommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { _SrvDescriptorHeap.Get() };
	_CommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	_CommandList->SetGraphicsRootSignature(_RootSignature.Get());

	auto passCB = _CurrFrameResource->PassCB->Resource();
	_CommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

	_CommandList->SetPipelineState(_PSOs["opaque"].Get());
	DrawRenderItems(_CommandList.Get(), _RitemLayer[(int)RenderLayer::Opaque]);

	_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(_CommandList->Close());

	ID3D12CommandList* cmdsLists[] = { _CommandList.Get() };
	_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	RenderUI();

	ThrowIfFailed(_SwapChain->Present(0, 0));
	_CurrBackBuffer = (_CurrBackBuffer + 1) % SwapChainBufferCount;

	_CurrFrameResource->Fence = ++_CurrentFence;

	_CommandQueue->Signal(_Fence.Get(), _CurrentFence);
}

void GraphicsWindow::Update()
{
	UpdateCamera(_game_timer);
	UpdateFixedCamera(_game_timer);

	_CurrFrameResourceIndex = (_CurrFrameResourceIndex + 1) % gNumFrameResources;
	_CurrFrameResource = _FrameResources[_CurrFrameResourceIndex].get();

	if (_CurrFrameResource->Fence != 0 && _Fence->GetCompletedValue() < _CurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(_Fence->SetEventOnCompletion(_CurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	UpdateObjectCBs(_game_timer);
	UpdateMaterialCBs(_game_timer);
	UpdateMainPassCB(_game_timer);
}

LRESULT GraphicsWindow::OnResize()
{
	AbstractWindow::OnResize();

	DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f * DirectX::XM_PI, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&_Proj, P);

	return 0;
}

LRESULT GraphicsWindow::OnMouseDown(WPARAM btnState, int x, int y)
{
	if (GetKeyState(VK_CONTROL) < 0)
	{
		_LastMousePos.x = x;
		_LastMousePos.y = y;

		SetCapture(_hWnd);
	}
	else
	{
	}

	return 0;
}

LRESULT GraphicsWindow::OnMouseUp(WPARAM btnState, int x, int y)
{
	if (GetKeyState(VK_CONTROL) < 0)
	{
		ReleaseCapture();
	}

	KillTimer(Window(), IDT_TIMER_UP);
	KillTimer(Window(), IDT_TIMER_DOWN);
	KillTimer(Window(), IDT_TIMER_LEFT);
	KillTimer(Window(), IDT_TIMER_RIGHT);
	KillTimer(Window(), IDT_TIMER_IN);
	KillTimer(Window(), IDT_TIMER_OUT);

	return 0;
}

LRESULT GraphicsWindow::OnMouseMove(WPARAM btnState, int x, int y)
{
	if (GetCapture() != _hWnd)
		return 0;
	
	if (GetKeyState(VK_CONTROL) < 0)
	{
		if ((btnState & MK_LBUTTON) != 0)
		{
			float dx = DirectX::XMConvertToRadians(0.25f * static_cast<float>(x - _LastMousePos.x));
			float dy = DirectX::XMConvertToRadians(0.25f * static_cast<float>(y - _LastMousePos.y));

			_Theta += dx;
			_Phi += dy;

			_Phi = MathHelper::Clamp(_Phi, 0.1f, DirectX::XM_PI - 0.1f);
		}
		else if ((btnState & MK_RBUTTON) != 0)
		{
			float dx = 0.2f * static_cast<float>(x - _LastMousePos.x);
			float dy = 0.2f * static_cast<float>(y - _LastMousePos.y);

			_Radius += dx - dy;

			_Radius = MathHelper::Clamp(_Radius, 5.0f, 150.0f);
		}

		_LastMousePos.x = x;
		_LastMousePos.y = y;
	}
	
	return 0;
}

void GraphicsWindow::LoadTextures()
{
	auto tileTex = std::make_unique<Texture>();
	tileTex->Name = "tileTex";
	tileTex->Filename = TEXTURE_PATH L"tile.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(_d3dDevice.Get(),
		_CommandList.Get(), tileTex->Filename.c_str(),
		tileTex->Resource, tileTex->UploadHeap));


	_Textures[tileTex->Name] = std::move(tileTex);
}

void GraphicsWindow::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable0;
	texTable0.Init(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		1,
		0);

	CD3DX12_DESCRIPTOR_RANGE texTable1;
	texTable1.Init(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		1,
		1);

	CD3DX12_ROOT_PARAMETER slotRootParameter[6];

	slotRootParameter[0].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[1].InitAsConstantBufferView(0);
	slotRootParameter[2].InitAsConstantBufferView(1);
	slotRootParameter[3].InitAsConstantBufferView(2);
	slotRootParameter[4].InitAsConstantBufferView(3);
	slotRootParameter[5].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL);

	auto staticSamplers = GetStaticSamplers();

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(6, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(_d3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(_RootSignature.GetAddressOf())));
}

void GraphicsWindow::BuildShadersAndInputLayout()
{
	_Shaders["opaqueVS"] = d3dUtil::LoadBinary(SHADER_PATH L"default_vs.cso");
		//d3dUtil::CompileShader(SHADER_PATH L"Default.hlsl", nullptr, "VS", "vs_5_1");
	_Shaders["opaquePS"] = d3dUtil::LoadBinary(SHADER_PATH L"default_ps.cso");
		//d3dUtil::CompileShader(SHADER_PATH L"Default.hlsl", nullptr, "PS", "ps_5_1");

	_InputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}


void GraphicsWindow::BuildBoardGeometry()
{
	
}

void GraphicsWindow::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	//
	// PSO for opaque objects.
	//
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { _InputLayout.data(), (UINT)_InputLayout.size() };
	opaquePsoDesc.pRootSignature = _RootSignature.Get();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(_Shaders["opaqueVS"]->GetBufferPointer()),
		_Shaders["opaqueVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(_Shaders["opaquePS"]->GetBufferPointer()),
		_Shaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	//opaquePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = _BackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = _4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = _4xMsaaState ? (_4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = _DepthStencilFormat;
	ThrowIfFailed(_d3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&_PSOs["opaque"])));
}

void GraphicsWindow::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		_FrameResources.push_back(std::make_unique<FrameResource>(_d3dDevice.Get(),
			1, (UINT)_AllRitems.size(), (UINT)_Materials.size()));
	}
}

void GraphicsWindow::BuildRenderItems()
{
}

void GraphicsWindow::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));
	
	auto objectCB = _CurrFrameResource->ObjectCB->Resource();
	auto matCB = _CurrFrameResource->MaterialCB->Resource();
	
	for (size_t i = 0; i < ritems.size(); ++i)
	{
		auto ri = ritems[i];

		cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
		cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(_SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		tex.Offset(ri->Mat->DiffuseSrvHeapIndex, _CbvSrvDescriptorSize);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex * matCBByteSize;

		cmdList->SetGraphicsRootDescriptorTable(0, tex);
		cmdList->SetGraphicsRootConstantBufferView(1, objCBAddress);
		cmdList->SetGraphicsRootConstantBufferView(3, matCBAddress);
		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}

void GraphicsWindow::UpdateCamera(const GameTimer& gt)
{
	_EyePos.x = _Radius * sinf(_Phi) * cosf(_Theta);
	_EyePos.z = _Radius * sinf(_Phi) * sinf(_Theta);
	_EyePos.y = _Radius * cosf(_Phi);

	DirectX::XMVECTOR pos = DirectX::XMVectorSet(_EyePos.x, _EyePos.y, _EyePos.z, 1.0f);
	DirectX::XMVECTOR target = DirectX::XMVectorZero();
	DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&_View, view);
}

void GraphicsWindow::UpdateFixedCamera(const GameTimer& gt)
{
	DirectX::XMVECTOR pos = DirectX::XMVectorSet(0.0f, 0.0f, -1.0f, 1.0f);
	DirectX::XMVECTOR target = DirectX::XMVectorZero();
	DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&_FixedView, view);
}

void GraphicsWindow::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = _CurrFrameResource->ObjectCB.get();
	for (auto& e : _AllRitems)
	{
		if (e->NumFramesDirty > 0)
		{
			DirectX::XMMATRIX world = XMLoadFloat4x4(&e->World);
			DirectX::XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			e->NumFramesDirty--;
		}
	}
}

void GraphicsWindow::UpdateMaterialCBs(const GameTimer& gt)
{
	auto currMaterialCB = _CurrFrameResource->MaterialCB.get();
	for (auto& e : _Materials)
	{
		Material* mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			DirectX::XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

			mat->NumFramesDirty--;
		}
	}
}

void GraphicsWindow::UpdateMainPassCB(const GameTimer& gt)
{
	DirectX::XMMATRIX view = XMLoadFloat4x4(&_View);
	DirectX::XMMATRIX proj = XMLoadFloat4x4(&_Proj);
	DirectX::XMMATRIX fixedView = XMLoadFloat4x4(&_FixedView);

	DirectX::XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	DirectX::XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	DirectX::XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	DirectX::XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&_MainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&_MainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&_MainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&_MainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&_MainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&_MainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	XMStoreFloat4x4(&_MainPassCB.FixedView, XMMatrixTranspose(fixedView));
	_MainPassCB.EyePosW = _EyePos;
	_MainPassCB.RenderTargetSize = DirectX::XMFLOAT2((float)_ClientWidth, (float)_ClientHeight);
	_MainPassCB.InvRenderTargetSize = DirectX::XMFLOAT2(1.0f / _ClientWidth, 1.0f / _ClientHeight);
	_MainPassCB.NearZ = 1.0f;
	_MainPassCB.FarZ = 1000.0f;
	_MainPassCB.TotalTime = _game_timer.TotalTime();
	_MainPassCB.DeltaTime = _game_timer.DeltaTime();
	_MainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	_MainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	_MainPassCB.Lights[0].Strength = { 0.8f, 0.8f, 0.8f };
	_MainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	_MainPassCB.Lights[1].Strength = { 0.4f, 0.4f, 0.4f };
	_MainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	_MainPassCB.Lights[2].Strength = { 0.2f, 0.2f, 0.2f };

	auto currPassCB = _CurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, _MainPassCB);
}


void GraphicsWindow::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(_d3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&_SrvDescriptorHeap)));

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(_SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	auto tileTex = _Textures["tileTex"]->Resource;
	
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = tileTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = tileTex->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	_d3dDevice->CreateShaderResourceView(tileTex.Get(), &srvDesc, hDescriptor);
}

void GraphicsWindow::BuildMaterials()
{
	auto tile0 = std::make_unique<Material>();
	tile0->Name = "tile0";
	tile0->MatCBIndex = 0;
	tile0->DiffuseSrvHeapIndex = 0;
	tile0->DiffuseAlbedo = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	tile0->FresnelR0 = DirectX::XMFLOAT3(0.02f, 0.02f, 0.02f);
	tile0->Roughness = 0.3f;

	
	_Materials["tile0"] = std::move(tile0);
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GraphicsWindow::GetStaticSamplers()
{
	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		0.0f,
		8);

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		0.0f,
		8);

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp };
}


LRESULT GraphicsWindow::OnTimer_Up()
{
	_Phi += 0.008f;
	_Phi = MathHelper::Clamp(_Phi, 0.1f, DirectX::XM_PI - 0.1f);
	return 0;
}

LRESULT GraphicsWindow::OnTimer_Down()
{
	_Phi -= 0.008f;
	_Phi = MathHelper::Clamp(_Phi, 0.1f, DirectX::XM_PI - 0.1f);
	return 0;
}

LRESULT GraphicsWindow::OnTimer_Left()
{
	_Theta += 0.008f;
	return 0;
}

LRESULT GraphicsWindow::OnTimer_Right()
{
	_Theta -= 0.008f;
	return 0;
}

LRESULT GraphicsWindow::OnTimer_Zoomin()
{
	_Radius -= 1.0f;

	_Radius = MathHelper::Clamp(_Radius, 5.0f, 150.0f);
	return 0;
}

LRESULT GraphicsWindow::OnTimer_Zoomout()
{
	_Radius += 1.0f;

	_Radius = MathHelper::Clamp(_Radius, 5.0f, 150.0f);
	return 0;
}
