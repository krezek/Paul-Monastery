#ifndef _RENDER_ITEM_H_
#define _RENDER_ITEM_H_

struct RenderItem
{
	RenderItem() = default;

	DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

	int NumFramesDirty = gNumFrameResources;

	UINT ObjCBIndex = -1;

	MeshGeometry* Geo = nullptr;
	Material* Mat = nullptr;

	DirectX::BoundingBox Bounds;

	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;

	UINT SkinnedCBIndex = -1;
};

enum class RenderLayer : int
{
	Sky = 0,
	Fixed,
	Count
};

#endif /* _RENDER_ITEM_H_ */
