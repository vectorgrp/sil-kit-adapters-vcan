// SPDX-FileCopyrightText: Copyright 2025 Vector Informatik GmbH
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <string>
#include <cstring>

namespace AdapterUtils {
// set CAN FD dlc field as a non-linear function of size of data field
// see https://elearning.vector.com/mod/page/view.php?id=368
constexpr int CalculateDLC(const uint16_t& dataFieldSize)
{
    return (dataFieldSize <= 8)    ? dataFieldSize
           : (dataFieldSize <= 12) ? 9
           : (dataFieldSize <= 16) ? 10
           : (dataFieldSize <= 20) ? 11
           : (dataFieldSize <= 24) ? 12
           : (dataFieldSize <= 32) ? 13
           : (dataFieldSize <= 48) ? 14
           : (dataFieldSize <= 64) ? 15
                                   : -1;
}

// Extract error codes and returns corresponding error strings
inline std::string ExtractErrorMessage(int errorCode)
{
    const char* errorMessage = strerror(errorCode);
    if (errorMessage != nullptr)
    {
        return ("\t(" + std::string(errorMessage) + ")");
    }
    else
    {
        return "";
    }
}

} // namespace AdapterUtils
