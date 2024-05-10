#include "pch.h"
#include "platform.h"

#include <d3dUtil.h>
#include <GeometryGenerator.h>
#include <FrameResource.h>
#include <RenderItem.h>
#include <Church.h>

using namespace DirectX;

#define CHURCH_BLOCK_RADIUS 5.0f
#define CHURCH_WALL_HEIGHT (CHURCH_BLOCK_RADIUS * 1.8f)
#define CHURCH_V_BLOCK_COUNT 30
#define CHURCH_H_BLOCK_COUNT 40
#define CHURCH_BLOCK_HEIGHT (CHURCH_WALL_HEIGHT / CHURCH_V_BLOCK_COUNT)
#define CHURCH_BLOCK_ANGLE (XM_2PI / CHURCH_H_BLOCK_COUNT)

#define CHURCH_ROOF_THICKNESS 1.5f
#define CHURCH_DOME_RADIUS (CHURCH_BLOCK_RADIUS - CHURCH_ROOF_THICKNESS)
#define CHURCH_DOME_SECTOR_DR 0.5f
#define CHURCH_DOME_SECTOR_THICKNESS 0.5f

extern int g_ObjCBIndex;

void Church::BuildGeometry(ID3D12Device* devicePtr,
	ID3D12GraphicsCommandList* commandListPtr,
	std::unordered_map<std::string,
	std::unique_ptr<MeshGeometry>>&geometries)
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData block = geoGen.CreateCylinder(CHURCH_BLOCK_RADIUS, CHURCH_BLOCK_HEIGHT, 0.0f, CHURCH_BLOCK_ANGLE, 4, 4);
	GeometryGenerator::MeshData dome = geoGen.CreateDome(CHURCH_DOME_RADIUS, XM_PIDIV2, 50, 50);
	GeometryGenerator::MeshData roofRing = geoGen.CreateRing(CHURCH_BLOCK_RADIUS, CHURCH_ROOF_THICKNESS, 0.0f, XM_2PI, 50, 4);
	GeometryGenerator::MeshData domeSector = geoGen.CreateSector(CHURCH_DOME_RADIUS + CHURCH_DOME_SECTOR_DR, CHURCH_DOME_SECTOR_DR, 0.0f, XM_PIDIV2, CHURCH_DOME_SECTOR_THICKNESS, 50, 2, 2);

	size_t totalSize = block.Vertices.size() +
		dome.Vertices.size() +
		roofRing.Vertices.size() +
		domeSector.Vertices.size();
	std::vector<Vertex> vertices(totalSize);

	UINT k = 0;
	for (size_t i = 0; i < block.Vertices.size(); ++i, ++k)
	{
		auto& p = block.Vertices[i].Position;
		vertices[k].Pos = p;
		vertices[k].Normal = block.Vertices[i].Normal;
		vertices[k].TexC = block.Vertices[i].TexC;
	}

	for (size_t i = 0; i < dome.Vertices.size(); ++i, ++k)
	{
		auto& p = dome.Vertices[i].Position;
		vertices[k].Pos = p;
		vertices[k].Normal = dome.Vertices[i].Normal;
		vertices[k].TexC = dome.Vertices[i].TexC;
	}

	for (size_t i = 0; i < roofRing.Vertices.size(); ++i, ++k)
	{
		auto& p = roofRing.Vertices[i].Position;
		vertices[k].Pos = p;
		vertices[k].Normal = roofRing.Vertices[i].Normal;
		vertices[k].TexC = roofRing.Vertices[i].TexC;
	}

	for (size_t i = 0; i < domeSector.Vertices.size(); ++i, ++k)
	{
		auto& p = domeSector.Vertices[i].Position;
		vertices[k].Pos = p;
		vertices[k].Normal = domeSector.Vertices[i].Normal;
		vertices[k].TexC = domeSector.Vertices[i].TexC;
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), std::begin(block.GetIndices16()), std::end(block.GetIndices16()));
	indices.insert(indices.end(), std::begin(dome.GetIndices16()), std::end(dome.GetIndices16()));
	indices.insert(indices.end(), std::begin(roofRing.GetIndices16()), std::end(roofRing.GetIndices16()));
	indices.insert(indices.end(), std::begin(domeSector.GetIndices16()), std::end(domeSector.GetIndices16()));
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

	SubmeshGeometry domeSubmesh;
	domeSubmesh.IndexCount = (UINT)dome.Indices32.size();
	domeSubmesh.StartIndexLocation = (UINT)block.Indices32.size();
	domeSubmesh.BaseVertexLocation = (UINT)block.Vertices.size();

	geo->DrawArgs["dome"] = domeSubmesh;

	SubmeshGeometry roofRingSubmesh;
	roofRingSubmesh.IndexCount = (UINT)roofRing.Indices32.size();
	roofRingSubmesh.StartIndexLocation = (UINT)block.Indices32.size() + 
		(UINT)dome.Indices32.size();
	roofRingSubmesh.BaseVertexLocation = (UINT)block.Vertices.size() +
		(UINT)dome.Vertices.size();

	geo->DrawArgs["roofRing"] = roofRingSubmesh;

	SubmeshGeometry domeSectorSubmesh;
	domeSectorSubmesh.IndexCount = (UINT)domeSector.Indices32.size();
	domeSectorSubmesh.StartIndexLocation = (UINT)block.Indices32.size() +
		(UINT)dome.Indices32.size() + 
		(UINT)roofRing.Indices32.size();
	domeSectorSubmesh.BaseVertexLocation = (UINT)block.Vertices.size() +
		(UINT)dome.Vertices.size() +
		(UINT)roofRing.Vertices.size();

	geo->DrawArgs["domeSector"] = domeSectorSubmesh;


	geometries[geo->Name] = std::move(geo);
}

void Church::BuildRenderItems_Roof(std::unordered_map<std::string, std::unique_ptr<MeshGeometry>>& geometries,
	std::unordered_map<std::string, std::unique_ptr<Material>>& materials,
	std::vector<std::unique_ptr<RenderItem>>& allRitems,
	std::vector<RenderItem*>& opaqueRenderItems)
{
	// add Wall objects
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

void Church::BuildRenderItems_Wall(std::unordered_map<std::string, std::unique_ptr<MeshGeometry>>& geometries,
	std::unordered_map<std::string, std::unique_ptr<Material>>& materials,
	std::vector<std::unique_ptr<RenderItem>>& allRitems,
	std::vector<RenderItem*>& opaqueRenderItems)
{
	// add Dome object
	auto domeRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&domeRitem->World, XMMatrixTranslation(0.0f, CHURCH_BLOCK_HEIGHT * CHURCH_V_BLOCK_COUNT, 0.0f));
	XMStoreFloat4x4(&domeRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	domeRitem->ObjCBIndex = g_ObjCBIndex++;
	domeRitem->Geo = geometries["churchGeo"].get();
	domeRitem->Mat = materials["churchDome0"].get();
	domeRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	domeRitem->IndexCount = domeRitem->Geo->DrawArgs["dome"].IndexCount;
	domeRitem->StartIndexLocation = domeRitem->Geo->DrawArgs["dome"].StartIndexLocation;
	domeRitem->BaseVertexLocation = domeRitem->Geo->DrawArgs["dome"].BaseVertexLocation;

	opaqueRenderItems.push_back(domeRitem.get());
	allRitems.push_back(std::move(domeRitem));

	// add roof ring object
	auto roofRingRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&roofRingRitem->World, XMMatrixTranslation(0.0f, CHURCH_BLOCK_HEIGHT * CHURCH_V_BLOCK_COUNT, 0.0f));
	XMStoreFloat4x4(&roofRingRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	roofRingRitem->ObjCBIndex = g_ObjCBIndex++;
	roofRingRitem->Geo = geometries["churchGeo"].get();
	roofRingRitem->Mat = materials["churchDome0"].get();
	roofRingRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	roofRingRitem->IndexCount = roofRingRitem->Geo->DrawArgs["roofRing"].IndexCount;
	roofRingRitem->StartIndexLocation = roofRingRitem->Geo->DrawArgs["roofRing"].StartIndexLocation;
	roofRingRitem->BaseVertexLocation = roofRingRitem->Geo->DrawArgs["roofRing"].BaseVertexLocation;

	opaqueRenderItems.push_back(roofRingRitem.get());
	allRitems.push_back(std::move(roofRingRitem));

	// add dome sectors
	for (int i = 0; i < 4; ++i)
	{
		auto domeSectorRitem = std::make_unique<RenderItem>();
		XMStoreFloat4x4(&domeSectorRitem->World, XMMatrixTranslation(0.0f, -CHURCH_DOME_SECTOR_THICKNESS / 2, 0.0f) *
			XMMatrixRotationX(-XM_PIDIV2) *
			XMMatrixRotationY(XM_PIDIV2 * i) *
			XMMatrixTranslation(0.0f, CHURCH_BLOCK_HEIGHT * CHURCH_V_BLOCK_COUNT, 0.0f));
		XMStoreFloat4x4(&domeSectorRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
		domeSectorRitem->ObjCBIndex = g_ObjCBIndex++;
		domeSectorRitem->Geo = geometries["churchGeo"].get();
		domeSectorRitem->Mat = materials["churchBlock0"].get();
		domeSectorRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		domeSectorRitem->IndexCount = domeSectorRitem->Geo->DrawArgs["domeSector"].IndexCount;
		domeSectorRitem->StartIndexLocation = domeSectorRitem->Geo->DrawArgs["domeSector"].StartIndexLocation;
		domeSectorRitem->BaseVertexLocation = domeSectorRitem->Geo->DrawArgs["domeSector"].BaseVertexLocation;

		opaqueRenderItems.push_back(domeSectorRitem.get());
		allRitems.push_back(std::move(domeSectorRitem));
	}
}


void Church::BuildRenderItems(std::unordered_map<std::string,
	std::unique_ptr<MeshGeometry>>&geometries,
	std::unordered_map<std::string, std::unique_ptr<Material>>& materials,
	std::vector<std::unique_ptr<RenderItem>>& allRitems,
	std::vector<RenderItem*>& opaqueRenderItems)
{
	BuildRenderItems_Wall(geometries, materials, allRitems, opaqueRenderItems);
	BuildRenderItems_Roof(geometries, materials, allRitems, opaqueRenderItems);
}
