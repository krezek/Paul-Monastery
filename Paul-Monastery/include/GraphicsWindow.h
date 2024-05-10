#ifndef _GRAPHICS_WINDOW_H_
#define _GRAPHICS_WINDOW_H_

#include <AbstractWindow.h>
#include <MathHelper.h>
#include <UploadBuffer.h>
#include <FrameResource.h>
#include <RenderItem.h>
#include <Monastery.h>


class GraphicsWindow : public AbstractWindow
{
	friend BOOL BaseWindow::Create(PCTSTR lpWindowName, DWORD dwStyle, DWORD dwExStyle,
		int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu);
public:
	GraphicsWindow() = default;
	virtual ~GraphicsWindow() = default;

public:
	void InitDirect3D();

	void Draw();
	void Update();

	virtual LRESULT OnCreate(); 
	virtual LRESULT OnResize();
	virtual LRESULT OnMouseDown(WPARAM btnState, int x, int y);
	virtual LRESULT OnMouseUp(WPARAM btnState, int x, int y);
	virtual LRESULT OnMouseMove(WPARAM btnState, int x, int y);

	virtual LRESULT OnTimer_Up();
	virtual LRESULT OnTimer_Down();
	virtual LRESULT OnTimer_Left();
	virtual LRESULT OnTimer_Right();
	virtual LRESULT OnTimer_Zoomin();
	virtual LRESULT OnTimer_Zoomout();

protected:
	UINT _CbvSrvDescriptorSize = 0;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _SrvDescriptorHeap = nullptr;

	std::unordered_map<std::string, std::unique_ptr<Texture>> _Textures;
	std::unordered_map<std::string, std::unique_ptr<Material>> _Materials;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> _RootSignature = nullptr;

	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> _Shaders;
	std::vector<D3D12_INPUT_ELEMENT_DESC> _InputLayout;
	
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> _Geometries;
	std::unordered_map < std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState >> _PSOs;

	std::vector<std::unique_ptr<FrameResource>> _FrameResources;
	FrameResource* _CurrFrameResource = nullptr;
	int _CurrFrameResourceIndex = 0;

	// List of all the render items.
	std::vector<std::unique_ptr<RenderItem>> _AllRitems;
	std::vector<RenderItem*> _RitemLayer[(int)RenderLayer::Count];

	PassConstants _MainPassCB;

	DirectX::XMFLOAT3 _EyePos = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT4X4 _View = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 _Proj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 _FixedView = MathHelper::Identity4x4();

	float _Theta = 1.5f * DirectX::XM_PI;
	float _Phi = DirectX::XM_PIDIV2 - 0.5f;
	float _Radius = 30.0f;

protected:
	void UpdateCamera(const GameTimer& gt);
	void UpdateFixedCamera(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMaterialCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);
	
	void LoadTextures();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildGeometry();
	void BuildMaterials();
	void BuildDescriptorHeaps();
	void BuildPSOs();
	void BuildFrameResources();
	void BuildRenderItems();
	void BuildMonastery();

	void PickFixed(int sx, int sy);
	
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);
	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

	std::unique_ptr<Monastery> _Monastery;
};

#endif /* _GRAPHICS_WINDOW_H_ */
