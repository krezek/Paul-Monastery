#include "pch.h"
#include "platform.h"

#include <d3dUtil.h>
#include <GeometryGenerator.h>
#include <FrameResource.h>
#include <RenderItem.h>
#include <Monastery.h>

Monastery::Monastery()
{
	_Church = std::make_unique<Church>();
}

void Monastery::BuildGeometry(ID3D12Device* devicePtr, 
	ID3D12GraphicsCommandList* commandListPtr, 
	std::unordered_map<std::string, 
	std::unique_ptr<MeshGeometry>>& geometries)
{
	_Church->BuildGeometry(devicePtr, commandListPtr, geometries);
}

void Monastery::BuildRenderItems(std::unordered_map<std::string, 
	std::unique_ptr<MeshGeometry>>& geometries, 
	std::unordered_map<std::string, std::unique_ptr<Material>>& materials, 
	std::vector<std::unique_ptr<RenderItem>>& allRitems, 
	std::vector<RenderItem*>& opaqueRenderItems)
{
	_Church->BuildRenderItems(geometries, materials, allRitems, opaqueRenderItems);
}
