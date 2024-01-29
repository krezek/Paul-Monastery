#ifndef _UPLOAD_BUFFER_H_
#define _UPLOAD_BUFFER_H_

#include <d3dUtil.h>

template<typename T>
class UploadBuffer
{
public:
    UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer) :
        _IsConstantBuffer(isConstantBuffer)
    {
        _ElementByteSize = sizeof(T);

        if (isConstantBuffer)
            _ElementByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(T));

        ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(_ElementByteSize * elementCount),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&_UploadBuffer)));

        ThrowIfFailed(_UploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&_MappedData)));
    }

    UploadBuffer(const UploadBuffer& rhs) = delete;
    UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
    ~UploadBuffer()
    {
        if (_UploadBuffer != nullptr)
            _UploadBuffer->Unmap(0, nullptr);

        _MappedData = nullptr;
    }

    ID3D12Resource* Resource()const
    {
        return _UploadBuffer.Get();
    }

    void CopyData(int elementIndex, const T& data)
    {
        memcpy(&_MappedData[elementIndex * _ElementByteSize], &data, sizeof(T));
    }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> _UploadBuffer;
    BYTE* _MappedData = nullptr;

    UINT _ElementByteSize = 0;
    bool _IsConstantBuffer = false;
};
#endif /* _UPLOAD_BUFFER_H_ */
