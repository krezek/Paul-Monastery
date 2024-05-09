#include "pch.h"
#include "platform.h"

#include <d3dUtil.h>
#include <GeometryGenerator.h>
#include <FrameResource.h>
#include <RenderItem.h>
#include <Monastery.h>

static int g_obj_idx = 7;

void Monastery::BuildGeometry(ID3D12Device* devicePtr, 
	ID3D12GraphicsCommandList* commandListPtr, 
	std::unordered_map<std::string, 
	std::unique_ptr<MeshGeometry>>& geometries)
{
	GeometryGenerator geoGen;
	//GeometryGenerator::MeshData grid = geoGen.CreateGrid(12.0f, 12.0f, 12, 12);
	GeometryGenerator::MeshData grid = geoGen.CreateRing(12.0f, 6.0f, 0.0f, DirectX::XM_PI, 12, 12);
	
	size_t totalSize = grid.Vertices.size();
	std::vector<Vertex> vertices(totalSize);

	UINT k = 0;
	for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		auto& p = grid.Vertices[i].Position;
		vertices[k].Pos = p;
		vertices[k].Normal = grid.Vertices[i].Normal;
		vertices[k].TexC = grid.Vertices[i].TexC;
	}

	
	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "monasteryGeo";

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

	SubmeshGeometry gridSubmesh;
	gridSubmesh.IndexCount = (UINT)grid.Indices32.size();
	gridSubmesh.StartIndexLocation = 0;
	gridSubmesh.BaseVertexLocation = 0;

	geo->DrawArgs["grid"] = gridSubmesh;

	geometries[geo->Name] = std::move(geo);
}

void Monastery::BuildRenderItems(std::unordered_map<std::string, 
	std::unique_ptr<MeshGeometry>>& geometries, 
	std::unordered_map<std::string, std::unique_ptr<Material>>& materials, 
	std::vector<std::unique_ptr<RenderItem>>& allRitems, 
	std::vector<RenderItem*>& opaqueRenderItems)
{
	auto gridRitem = std::make_unique<RenderItem>();
	gridRitem->World = MathHelper::Identity4x4();
	XMStoreFloat4x4(&gridRitem->TexTransform, DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f));
	gridRitem->ObjCBIndex = g_obj_idx++;
	gridRitem->Geo = geometries["monasteryGeo"].get();
	gridRitem->Mat = materials["ground0"].get();
	gridRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
	gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

	opaqueRenderItems.push_back(gridRitem.get());
	allRitems.push_back(std::move(gridRitem));
}
