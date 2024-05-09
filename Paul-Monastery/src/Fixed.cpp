#include "pch.h"
#include "platform.h"

#include <d3dUtil.h>
#include <GeometryGenerator.h>
#include <FrameResource.h>
#include <RenderItem.h>
#include <Fixed.h>

extern int g_ObjCBIndex;

RenderItem* Fixed::_newButton = nullptr;
RenderItem* Fixed::_upButton = nullptr;
RenderItem* Fixed::_downButton = nullptr;
RenderItem* Fixed::_leftButton = nullptr;
RenderItem* Fixed::_rightButton = nullptr;
RenderItem* Fixed::_zoominButton = nullptr;
RenderItem* Fixed::_zoomoutButton = nullptr;

void Fixed::BuildGeometry(ID3D12Device* devicePtr, 
	ID3D12GraphicsCommandList* commandListPtr, 
	std::unordered_map<std::string, 
	std::unique_ptr<MeshGeometry>>& geometries)
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData button = geoGen.CreateQuad(-1.0f, 1.0f, 2.0f, 2.0f, 0.0f);

	size_t totalSize = button.Vertices.size();
	std::vector<Vertex> vertices(totalSize);

	UINT k = 0;
	for (size_t i = 0; i < button.Vertices.size(); ++i, ++k)
	{
		auto& p = button.Vertices[i].Position;
		vertices[k].Pos = p;
		vertices[k].Normal = button.Vertices[i].Normal;
		vertices[k].TexC = button.Vertices[i].TexC;
	}

	DirectX::BoundingBox button_bounds;
	DirectX::BoundingBox::CreateFromPoints(button_bounds, button.Vertices.size(),
		&vertices[0].Pos, sizeof(Vertex));

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), std::begin(button.GetIndices16()), std::end(button.GetIndices16()));
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "fixedGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(devicePtr,
		commandListPtr, vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(devicePtr,
		commandListPtr, indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry buttonSubmesh;
	buttonSubmesh.IndexCount = (UINT)button.Indices32.size();
	buttonSubmesh.StartIndexLocation = 0;
	buttonSubmesh.BaseVertexLocation = 0;
	buttonSubmesh.Bounds = button_bounds;

	geo->DrawArgs["button"] = buttonSubmesh;

	geometries[geo->Name] = std::move(geo);
}

void Fixed::BuildRenderItems(std::unordered_map<std::string, 
	std::unique_ptr<MeshGeometry>>& geometries, 
	std::unordered_map<std::string, std::unique_ptr<Material>>& materials, 
	std::vector<std::unique_ptr<RenderItem>>& allRitems, 
	std::vector<RenderItem*>& fixedRenderItems)
{
	auto bUpButtonRitem = std::make_unique<RenderItem>();
	_upButton = bUpButtonRitem.get();
	XMStoreFloat4x4(&bUpButtonRitem->World, DirectX::XMMatrixScaling(0.02f, 0.02f, 0.1f) *
		DirectX::XMMatrixTranslation(0.45f, 0.28f, 0.0f));
	bUpButtonRitem->ObjCBIndex = g_ObjCBIndex++;
	bUpButtonRitem->Geo = geometries["fixedGeo"].get();
	bUpButtonRitem->Mat = materials["up0"].get();
	bUpButtonRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	bUpButtonRitem->IndexCount = bUpButtonRitem->Geo->DrawArgs["button"].IndexCount;
	bUpButtonRitem->StartIndexLocation = bUpButtonRitem->Geo->DrawArgs["button"].StartIndexLocation;
	bUpButtonRitem->BaseVertexLocation = bUpButtonRitem->Geo->DrawArgs["button"].BaseVertexLocation;
	bUpButtonRitem->Bounds = bUpButtonRitem->Geo->DrawArgs["button"].Bounds;

	fixedRenderItems.push_back(bUpButtonRitem.get());
	allRitems.push_back(std::move(bUpButtonRitem));

	auto bDownButtonRitem = std::make_unique<RenderItem>();
	_downButton = bDownButtonRitem.get();
	XMStoreFloat4x4(&bDownButtonRitem->World, DirectX::XMMatrixScaling(0.02f, 0.02f, 0.1f) *
		DirectX::XMMatrixTranslation(0.45f, 0.2f, 0.0f));
	bDownButtonRitem->ObjCBIndex = g_ObjCBIndex++;
	bDownButtonRitem->Geo = geometries["fixedGeo"].get();
	bDownButtonRitem->Mat = materials["down0"].get();
	bDownButtonRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	bDownButtonRitem->IndexCount = bDownButtonRitem->Geo->DrawArgs["button"].IndexCount;
	bDownButtonRitem->StartIndexLocation = bDownButtonRitem->Geo->DrawArgs["button"].StartIndexLocation;
	bDownButtonRitem->BaseVertexLocation = bDownButtonRitem->Geo->DrawArgs["button"].BaseVertexLocation;
	bDownButtonRitem->Bounds = bDownButtonRitem->Geo->DrawArgs["button"].Bounds;

	fixedRenderItems.push_back(bDownButtonRitem.get());
	allRitems.push_back(std::move(bDownButtonRitem));

	auto bLeftButtonRitem = std::make_unique<RenderItem>();
	_leftButton = bLeftButtonRitem.get();
	XMStoreFloat4x4(&bLeftButtonRitem->World, DirectX::XMMatrixScaling(0.02f, 0.02f, 0.1f) *
		DirectX::XMMatrixTranslation(0.4f, 0.24f, 0.0f));
	bLeftButtonRitem->ObjCBIndex = g_ObjCBIndex++;
	bLeftButtonRitem->Geo = geometries["fixedGeo"].get();
	bLeftButtonRitem->Mat = materials["left0"].get();
	bLeftButtonRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	bLeftButtonRitem->IndexCount = bLeftButtonRitem->Geo->DrawArgs["button"].IndexCount;
	bLeftButtonRitem->StartIndexLocation = bLeftButtonRitem->Geo->DrawArgs["button"].StartIndexLocation;
	bLeftButtonRitem->BaseVertexLocation = bLeftButtonRitem->Geo->DrawArgs["button"].BaseVertexLocation;
	bLeftButtonRitem->Bounds = bLeftButtonRitem->Geo->DrawArgs["button"].Bounds;

	fixedRenderItems.push_back(bLeftButtonRitem.get());
	allRitems.push_back(std::move(bLeftButtonRitem));

	auto bRightButtonRitem = std::make_unique<RenderItem>();
	_rightButton = bRightButtonRitem.get();
	XMStoreFloat4x4(&bRightButtonRitem->World, DirectX::XMMatrixScaling(0.02f, 0.02f, 0.1f) *
		DirectX::XMMatrixTranslation(0.5f, 0.24f, 0.0f));
	bRightButtonRitem->ObjCBIndex = g_ObjCBIndex++;
	bRightButtonRitem->Geo = geometries["fixedGeo"].get();
	bRightButtonRitem->Mat = materials["right0"].get();
	bRightButtonRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	bRightButtonRitem->IndexCount = bRightButtonRitem->Geo->DrawArgs["button"].IndexCount;
	bRightButtonRitem->StartIndexLocation = bRightButtonRitem->Geo->DrawArgs["button"].StartIndexLocation;
	bRightButtonRitem->BaseVertexLocation = bRightButtonRitem->Geo->DrawArgs["button"].BaseVertexLocation;
	bRightButtonRitem->Bounds = bRightButtonRitem->Geo->DrawArgs["button"].Bounds;

	fixedRenderItems.push_back(bRightButtonRitem.get());
	allRitems.push_back(std::move(bRightButtonRitem));

	auto bZoominButtonRitem = std::make_unique<RenderItem>();
	_zoominButton = bZoominButtonRitem.get();
	XMStoreFloat4x4(&bZoominButtonRitem->World, DirectX::XMMatrixScaling(0.02f, 0.02f, 0.1f) *
		DirectX::XMMatrixTranslation(0.55f, 0.28f, 0.0f));
	bZoominButtonRitem->ObjCBIndex = g_ObjCBIndex++;
	bZoominButtonRitem->Geo = geometries["fixedGeo"].get();
	bZoominButtonRitem->Mat = materials["zoomin0"].get();
	bZoominButtonRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	bZoominButtonRitem->IndexCount = bZoominButtonRitem->Geo->DrawArgs["button"].IndexCount;
	bZoominButtonRitem->StartIndexLocation = bZoominButtonRitem->Geo->DrawArgs["button"].StartIndexLocation;
	bZoominButtonRitem->BaseVertexLocation = bZoominButtonRitem->Geo->DrawArgs["button"].BaseVertexLocation;
	bZoominButtonRitem->Bounds = bZoominButtonRitem->Geo->DrawArgs["button"].Bounds;

	fixedRenderItems.push_back(bZoominButtonRitem.get());
	allRitems.push_back(std::move(bZoominButtonRitem));

	auto bZoomoutButtonRitem = std::make_unique<RenderItem>();
	_zoomoutButton = bZoomoutButtonRitem.get();
	XMStoreFloat4x4(&bZoomoutButtonRitem->World, DirectX::XMMatrixScaling(0.02f, 0.02f, 0.1f) *
		DirectX::XMMatrixTranslation(0.55f, 0.2f, 0.0f));
	bZoomoutButtonRitem->ObjCBIndex = g_ObjCBIndex++;
	bZoomoutButtonRitem->Geo = geometries["fixedGeo"].get();
	bZoomoutButtonRitem->Mat = materials["zoomout0"].get();
	bZoomoutButtonRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	bZoomoutButtonRitem->IndexCount = bZoomoutButtonRitem->Geo->DrawArgs["button"].IndexCount;
	bZoomoutButtonRitem->StartIndexLocation = bZoomoutButtonRitem->Geo->DrawArgs["button"].StartIndexLocation;
	bZoomoutButtonRitem->BaseVertexLocation = bZoomoutButtonRitem->Geo->DrawArgs["button"].BaseVertexLocation;
	bZoomoutButtonRitem->Bounds = bZoomoutButtonRitem->Geo->DrawArgs["button"].Bounds;

	fixedRenderItems.push_back(bZoomoutButtonRitem.get());
	allRitems.push_back(std::move(bZoomoutButtonRitem));
}
