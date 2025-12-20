#pragma once

#include "constants.hpp"

namespace core
{
    class Object
    {
    public:
        auto   roll(float direction) -> void;
        auto update()                -> void;

        auto matrix()  const -> const math::mat4&;

    private:
        auto roll_step(const math::vec2& pivot, float ps0, float t, float direction) const -> std::pair<math::vec2, float>;

        math::mat4 _matrix { 1.0f };

        math::vec2 _position { };
        math::vec2 _pivot    { };

        float        _rotation { };
        float  _start_rotation { };
        float  _roll_direction { };
        float  _animation_time { };

        int32_t     _square_index { 0 };
        int32_t _max_square_index { 2 };

        bool  _rolling { };

        static constexpr auto radius { size / math::sqrt2 };
    };
}