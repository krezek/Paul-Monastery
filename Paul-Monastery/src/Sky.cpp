#include "pch.h"
#include "platform.h"

#include <d3dUtil.h>
#include <GeometryGenerator.h>
#include <FrameResource.h>
#include <RenderItem.h>
#include <Sky.h>

extern int g_ObjCBIndex;

void Sky::BuildGeometry(ID3D12Device* devicePtr,
	ID3D12GraphicsCommandList* commandListPtr, 
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>>& geometries)
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.2f, 50, 50);

	size_t totalSize = sphere.Vertices.size();
	std::vector<Vertex> vertices(totalSize);

	UINT k = 0;
	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		auto& p = sphere.Vertices[i].Position;
		vertices[k].Pos = p;
		vertices[k].Normal = sphere.Vertices[i].Normal;
		vertices[k].TexC = sphere.Vertices[i].TexC;
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "skyGeo";

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

	SubmeshGeometry sphereSubmesh;
	sphereSubmesh.IndexCount = (UINT)sphere.Indices32.size();
	sphereSubmesh.StartIndexLocation = 0;
	sphereSubmesh.BaseVertexLocation = 0;

	geo->DrawArgs["sphere"] = sphereSubmesh;

	geometries[geo->Name] = std::move(geo);
}

void Sky::BuildRenderItems(std::unordered_map<std::string, std::unique_ptr<MeshGeometry>>& geometries,
	std::unordered_map<std::string, std::unique_ptr<Material>>& materials,
	std::vector<std::unique_ptr<RenderItem>>& allRitems, std::vector<RenderItem*>& skyRenderItems)
{
	auto skyRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&skyRitem->World, DirectX::XMMatrixScaling(5000.0f, 5000.0f, 5000.0f));
	skyRitem->ObjCBIndex = g_ObjCBIndex++;
	skyRitem->Geo = geometries["skyGeo"].get();
	skyRitem->Mat = materials["sky0"].get();
	skyRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skyRitem->IndexCount = skyRitem->Geo->DrawArgs["sphere"].IndexCount;
	skyRitem->StartIndexLocation = skyRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
	skyRitem->BaseVertexLocation = skyRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;

	skyRenderItems.push_back(skyRitem.get());
	allRitems.push_back(std::move(skyRitem));
}
