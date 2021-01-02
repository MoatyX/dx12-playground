#include "geometry.h"

geometry::geometry(const UINT vertex_buffer_size_byte, const UINT vertex_stride_byte, const UINT index_buffer_size_byte)
{
	this->vertex_buffer_size_byte = vertex_buffer_size_byte;
	this->vertex_stride_byte = vertex_stride_byte;
	this->index_buffer_size_byte = index_buffer_size_byte;
}

D3D12_VERTEX_BUFFER_VIEW geometry::vertex_buffer_view() const
{
	const D3D12_VERTEX_BUFFER_VIEW vb{
		vertex_buffer_gpu->GetGPUVirtualAddress(),
		vertex_buffer_size_byte,
		vertex_stride_byte
	};

	return vb;
}

D3D12_INDEX_BUFFER_VIEW geometry::vertex_index_view() const
{
	const D3D12_INDEX_BUFFER_VIEW ib{
		index_buffer_gpu->GetGPUVirtualAddress(),
		index_buffer_size_byte,
		index_format
	};

	return ib;
}
