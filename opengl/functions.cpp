#include "functions.hpp"

namespace opengl
{
    auto Functions::init() -> void
    {
        init_core_functions();

        init_buffer_functions();
    }

    auto Functions::init_core_functions() -> void
    {
        if (const auto instance = LoadLibrary("opengl32.dll"))
        {
            functions::clear       = reinterpret_cast<functions::PFNGLCLEARPROC> (GetProcAddress(instance, "glClear"));
            functions::clear_color = reinterpret_cast<functions::PFNGLCLEARCOLOR>(GetProcAddress(instance, "glClearColor"));
        }
    }

    auto Functions::init_buffer_functions() -> void
    {
        functions::create_buffers = reinterpret_cast<functions::PFNGLCREATEBUFFERS>(wglGetProcAddress("glCreateBuffers"));
    }
}