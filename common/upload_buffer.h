#pragma once
#include <wrl.h>
#include <d3d12.h>
#include "d3dx12.h"
#include "macros.h"

template<typename T>
class upload_buffer
{
public:
	upload_buffer(ID3D12Device* gpu, UINT elements_count);

	//disable copying and assigning-to operations
	upload_buffer(const upload_buffer& rhs) = delete;
	upload_buffer& operator= (const upload_buffer& rhs) = delete;

	~upload_buffer();

	/// <summary>
	/// returns a pointer to the Upload Buffer Resource
	/// </summary>
	ID3D12Resource* get_resource() const;

	/// <summary>
	/// copy data from the CPU memory to the GPU
	/// </summary>
	/// <param name="data"></param>
	void copy_data(const T* data);

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_upload_buffer_;
	UINT8* m_mapped_data_;
	int m_elements_count_;
	UINT m_size;
};

template <typename T>
upload_buffer<T>::upload_buffer(ID3D12Device* gpu, UINT elements_count)
{
	m_elements_count_ = elements_count;
	m_size = sizeof(T);
	m_mapped_data_ = nullptr;

	//create this resource on the GPU
	auto heap_property = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto res_desc = CD3DX12_RESOURCE_DESC::Buffer(256 * elements_count);
	THROW_IF_FAILED(gpu->CreateCommittedResource(&heap_property, D3D12_HEAP_FLAG_NONE, &res_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_upload_buffer_)));

	//permanently map this resource to a memory space on the CPU. so any data written to it, is reflected on the GPU.
	//this also has a performance benefit, since we are basically using this resource as write-only and we dont keep mapping/unmapping frequently
	THROW_IF_FAILED(m_upload_buffer_->Map(0, nullptr, reinterpret_cast<void**>(&m_mapped_data_)));
}

template <typename T>
upload_buffer<T>::~upload_buffer()
{
	if(m_upload_buffer_ == nullptr) return;

	m_upload_buffer_->Unmap(0, nullptr);
	m_mapped_data_ = nullptr;
}

template <typename T>
ID3D12Resource* upload_buffer<T>::get_resource() const
{
	return m_upload_buffer_.Get();
}

template <typename T>
void upload_buffer<T>::copy_data(const T* data)
{
	memcpy(m_mapped_data_, data, 256);
	//memset(m_mapped_data_, 0x00007ff64521d100, m_size);
}

