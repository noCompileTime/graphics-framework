#pragma once

#include "types.hpp"

namespace opengl::functions
{
    using  PFNGLCREATEBUFFERS = auto(APIENTRY*)(GLsizei n, GLuint* buffers) -> void;

    inline PFNGLCREATEBUFFERS create_buffers;
}