#pragma once
#include "dx12.h"

class geometry
{
public:

	geometry() = default;
	geometry(UINT vertex_buffer_size_byte, UINT vertex_stride_byte, UINT index_buffer_size_byte);

	D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view() const;
	D3D12_INDEX_BUFFER_VIEW vertex_index_view() const;

public:
	std::string name;

	// System memory copies.  Use Blobs because the vertex/index format can be generic.
	// It is up to the client to cast appropriately.  
	Microsoft::WRL::ComPtr<ID3DBlob> vertex_buffer_cpu = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> index_buffer_cpu = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertex_buffer_gpu = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> index_buffer_gpu = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertex_buffer_uploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> index_buffer_uploader = nullptr;

	//data about the vertex buffer.
	UINT vertex_stride_byte = 0;
	UINT vertex_buffer_size_byte = 0;

	//data about the index buffer
	DXGI_FORMAT index_format = DXGI_FORMAT_R16_UINT;
	UINT index_buffer_size_byte = 0;
};

