#pragma once

#include "types.hpp"

namespace opengl::functions
{
    using  PFNGLCLEARPROC  = auto(APIENTRY*)(GLbitfield mask)                                         -> void;
    using  PFNGLCLEARCOLOR = auto(APIENTRY*)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) -> void;

    inline PFNGLCLEARPROC  clear;
    inline PFNGLCLEARCOLOR clear_color;
}