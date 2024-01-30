#ifndef _FIXED_H_
#define _FIXED_H_

class Fixed
{
public:
	Fixed() = delete;
	~Fixed() = delete;

	static void BuildGeometry(ID3D12Device* devicePtr,
		ID3D12GraphicsCommandList* commandListPtr,
		std::unordered_map<std::string, std::unique_ptr<MeshGeometry>>& geometries);

	static void BuildRenderItems(std::unordered_map<std::string, std::unique_ptr<MeshGeometry>>& geometries,
		std::unordered_map<std::string, std::unique_ptr<Material>>& materials,
		std::vector<std::unique_ptr<RenderItem>>& allRitems,
		std::vector<RenderItem*>& fixedRenderItems);
};


#endif /* _FIXED_H_ */
