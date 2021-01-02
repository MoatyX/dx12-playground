#include "utility.h"

std::wstring ansi_to_wstring(const std::string& str)
{
	WCHAR buffer[UTILITY_GENERAL_STR_SIZE];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, UTILITY_GENERAL_STR_SIZE);
	return std::wstring(buffer);
}

// Helper function for acquiring the first available hardware adapter that supports Direct3D 12.
// If no such adapter can be found, *ppAdapter will be set to nullptr.
void get_hardware_adapter(IDXGIFactory1* p_factory, IDXGIAdapter1** pp_adapter, const bool high_performance_adapter)
{
	*pp_adapter = nullptr;

	Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
	Microsoft::WRL::ComPtr<IDXGIFactory6> factory6;
	if (SUCCEEDED(p_factory->QueryInterface(IID_PPV_ARGS(&factory6))))
	{
		for (UINT adapterIndex = 0;
			DXGI_ERROR_NOT_FOUND != factory6->EnumAdapterByGpuPreference(
				adapterIndex,
				high_performance_adapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
				IID_PPV_ARGS(&adapter));
			++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				// If you want a software adapter, pass in "/warp" on the command line.
				continue;
			}

			// Check to see whether the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}
	}
	else
	{
		for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != p_factory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				// If you want a software adapter, pass in "/warp" on the command line.
				continue;
			}

			// Check to see whether the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}
	}

	*pp_adapter = adapter.Detach();
}

Microsoft::WRL::ComPtr<ID3D12Resource> create_default_buffer(ID3D12Device* gpu, ID3D12GraphicsCommandList* cmd_list,
	const void* data, uint64_t size_byte, Microsoft::WRL::ComPtr<ID3D12Resource>& upload_buffer)
{
	Microsoft::WRL::ComPtr<ID3D12Resource> default_buffer;

	//create the default heap buffer
	auto default_heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto default_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(size_byte);
	THROW_IF_FAILED(gpu->CreateCommittedResource(&default_heap_prop, D3D12_HEAP_FLAG_NONE, &default_buffer_desc,
		D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(default_buffer.GetAddressOf())));

	//create an intermediate upload heap, so the CPU can initialise the data in the buffer, since default heaps is accessed only by GPU
	auto upload_heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto upload_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(size_byte);
	THROW_IF_FAILED(gpu->CreateCommittedResource(&upload_heap_prop, D3D12_HEAP_FLAG_NONE, &upload_buffer_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(upload_buffer.GetAddressOf())));

	//describe the data we want to copy into the default buffer
	D3D12_SUBRESOURCE_DATA data_desc = {
		data,
		static_cast<LONG_PTR>(size_byte)
	};

	//tell the gpu that the default buffer will be copied to.
	auto default_common_copy_transition = CD3DX12_RESOURCE_BARRIER::Transition(default_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	cmd_list->ResourceBarrier(1, &default_common_copy_transition);

	//next copy the data from the CPU memory to the upload buffer. (this task is appended to the commands list)
	UpdateSubresources<1>(cmd_list, default_buffer.Get(), upload_buffer.Get(), 0, 0, 1, &data_desc);

	//now after data has been copied from the upload buffer to the default buffer, tell the gpu to change the default buffer to a gpu-read-only buffer
	auto default_copy_read_transition = CD3DX12_RESOURCE_BARRIER::Transition(default_buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
	cmd_list->ResourceBarrier(1, &default_copy_read_transition);

	//the last 3 calls are appended to the GPU. so all buffers must be kept alive until the copying have been done. thankfully with ComPtr, this is handled for us automatically
	return default_buffer;
}
