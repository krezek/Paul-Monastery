#ifndef _MONASTERY_H_
#define _MONASTERY_H_

class Monastery
{
public:
	Monastery() = delete;
	~Monastery() = delete;

	static void BuildGeometry(ID3D12Device* devicePtr,
		ID3D12GraphicsCommandList* commandListPtr,
		std::unordered_map<std::string, std::unique_ptr<MeshGeometry>>& geometries);

	static void BuildRenderItems(std::unordered_map<std::string, std::unique_ptr<MeshGeometry>>& geometries,
		std::unordered_map<std::string, std::unique_ptr<Material>>& materials,
		std::vector<std::unique_ptr<RenderItem>>& allRitems,
		std::vector<RenderItem*>& opaqueRenderItems);
};


#endif /* _MONASTERY_H_ */
