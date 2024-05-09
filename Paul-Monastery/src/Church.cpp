#include "pch.h"
#include "platform.h"

#include <d3dUtil.h>
#include <GeometryGenerator.h>
#include <FrameResource.h>
#include <RenderItem.h>
#include <Church.h>

using namespace DirectX;

#define CHURCH_BLOCK_RADIUS 5.0f
#define CHURCH_WALL_HEIGHT (CHURCH_BLOCK_RADIUS * 1.5f)
#define CHURCH_V_BLOCK_COUNT 29
#define CHURCH_H_BLOCK_COUNT 40
#define CHURCH_BLOCK_HEIGHT (CHURCH_WALL_HEIGHT / CHURCH_V_BLOCK_COUNT)
#define CHURCH_BLOCK_ANGLE (XM_2PI / CHURCH_H_BLOCK_COUNT)

extern int g_ObjCBIndex;

void Church::BuildGeometry(ID3D12Device* devicePtr,
	ID3D12GraphicsCommandList* commandListPtr,
	std::unordered_map<std::string,
	std::unique_ptr<MeshGeometry>>&geometries)
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData block = geoGen.CreateCylinder(CHURCH_BLOCK_RADIUS, CHURCH_BLOCK_HEIGHT, 0.0f, CHURCH_BLOCK_ANGLE, 4, 4);

	size_t totalSize = block.Vertices.size();
	std::vector<Vertex> vertices(totalSize);

	UINT k = 0;
	for (size_t i = 0; i < block.Vertices.size(); ++i, ++k)
	{
		auto& p = block.Vertices[i].Position;
		vertices[k].Pos = p;
		vertices[k].Normal = block.Vertices[i].Normal;
		vertices[k].TexC = block.Vertices[i].TexC;
	}


	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), std::begin(block.GetIndices16()), std::end(block.GetIndices16()));
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "churchGeo";

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
	gridSubmesh.IndexCount = (UINT)block.Indices32.size();
	gridSubmesh.StartIndexLocation = 0;
	gridSubmesh.BaseVertexLocation = 0;

	geo->DrawArgs["block"] = gridSubmesh;

	geometries[geo->Name] = std::move(geo);
}

void Church::BuildRenderItems(std::unordered_map<std::string,
	std::unique_ptr<MeshGeometry>>&geometries,
	std::unordered_map<std::string, std::unique_ptr<Material>>& materials,
	std::vector<std::unique_ptr<RenderItem>>& allRitems,
	std::vector<RenderItem*>& opaqueRenderItems)
{
	for (int j = 0; j < CHURCH_V_BLOCK_COUNT; j += 2)
	{
		for (int i = 0; i < CHURCH_H_BLOCK_COUNT; ++i)
		{
			auto blockRitem = std::make_unique<RenderItem>();
			XMStoreFloat4x4(&blockRitem->World, XMMatrixRotationY(CHURCH_BLOCK_ANGLE * i) *
				XMMatrixTranslation(0.0f, CHURCH_BLOCK_HEIGHT * j, 0.0f));
			XMStoreFloat4x4(&blockRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
			blockRitem->ObjCBIndex = g_ObjCBIndex++;
			blockRitem->Geo = geometries["churchGeo"].get();
			blockRitem->Mat = materials["churchBlock0"].get();
			blockRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			blockRitem->IndexCount = blockRitem->Geo->DrawArgs["block"].IndexCount;
			blockRitem->StartIndexLocation = blockRitem->Geo->DrawArgs["block"].StartIndexLocation;
			blockRitem->BaseVertexLocation = blockRitem->Geo->DrawArgs["block"].BaseVertexLocation;

			opaqueRenderItems.push_back(blockRitem.get());
			allRitems.push_back(std::move(blockRitem));
		}

		for (int i = 0; i < CHURCH_H_BLOCK_COUNT; ++i)
		{
			auto blockRitem = std::make_unique<RenderItem>();
			XMStoreFloat4x4(&blockRitem->World, XMMatrixRotationY(CHURCH_BLOCK_ANGLE * i + CHURCH_BLOCK_ANGLE / 2) *
				XMMatrixTranslation(0.0f, CHURCH_BLOCK_HEIGHT * (j + 1), 0.0f));
			XMStoreFloat4x4(&blockRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
			blockRitem->ObjCBIndex = g_ObjCBIndex++;
			blockRitem->Geo = geometries["churchGeo"].get();
			blockRitem->Mat = materials["churchBlock0"].get();
			blockRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			blockRitem->IndexCount = blockRitem->Geo->DrawArgs["block"].IndexCount;
			blockRitem->StartIndexLocation = blockRitem->Geo->DrawArgs["block"].StartIndexLocation;
			blockRitem->BaseVertexLocation = blockRitem->Geo->DrawArgs["block"].BaseVertexLocation;

			opaqueRenderItems.push_back(blockRitem.get());
			allRitems.push_back(std::move(blockRitem));
		}
	}
	
}
