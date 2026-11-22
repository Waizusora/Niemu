#pragma once

#include <string_view>

namespace niemu {

/// Returns the compile-time project version.
[[nodiscard]] std::string_view version() noexcept;

} // namespace niemu
