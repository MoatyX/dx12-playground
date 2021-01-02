#pragma once
#include "dx12.h"


#define UTILITY_GENERAL_STR_SIZE	512

std::wstring ansi_to_wstring(const std::string& str);

/// <summary>
/// Helper function for acquiring the first available hardware adapter that supports Direct3D 12.
/// If no such adapter can be found, *ppAdapter will be set to nullptr.
/// </summary>
/// <param name="p_factory"></param>
/// <param name="pp_adapter"></param>
/// <param name="high_performance_adapter"></param>
void get_hardware_adapter(IDXGIFactory1* p_factory, IDXGIAdapter1** pp_adapter, const bool high_performance_adapter);

/// <summary>
/// Helper function for uploading data from the CPU onto a Default Heap on the GPU using an intermediate Upload Buffer
/// </summary>
/// <returns>reference to the resource on the default heap on the GPU</returns>
Microsoft::WRL::ComPtr<ID3D12Resource> create_default_buffer(ID3D12Device* gpu, ID3D12GraphicsCommandList* cmd_list, const void* data, uint64_t size_byte,
	Microsoft::WRL::ComPtr<ID3D12Resource>& upload_buffer);
