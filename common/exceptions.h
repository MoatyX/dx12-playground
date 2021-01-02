#pragma once
#include <string>
#include <winerror.h>
#include <comdef.h>

/**
 * @brief simple class to display dx12 exceptions
 */
class dx12_exception
{
public:
    dx12_exception() = default;
    dx12_exception(const HRESULT hr, const std::wstring& function_name, const std::wstring& filename, const int line_number);

    std::wstring to_string() const;

private:
    HRESULT error_code_ = S_OK;
    std::wstring function_name_;
    std::wstring filename_;
    int line_number_ = -1;
};
