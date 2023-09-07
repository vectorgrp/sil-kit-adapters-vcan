// Copyright (c) Vector Informatik GmbH. All rights reserved.

#pragma once

namespace adapters 
{
    template <class exception>
    void throwIf(bool b)
    {
        if (b)
            throw exception();
    }

    enum ReturnCode
    {
        NO_ERROR = 0,
        CLI_ERROR,
        CONFIGURATION_ERROR,
        OTHER_ERROR
    };
}