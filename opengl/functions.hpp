#pragma once

namespace opengl
{
    class Functions
    {
    public:
        static auto init() -> void;

    private:
        static auto init_core_functions()   -> void;

        static auto init_buffer_functions() -> void;
    };
}