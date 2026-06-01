#pragma once

#include "vec2.hpp"

#include "mat4_utils.hpp"
#include "quat_utils.hpp"

constexpr auto      size = 0.5f;
constexpr auto half_size = 0.5f * size;

namespace core
{
    class Object
    {
    public:
        auto   roll(const math::vec2& direction) -> void;
        auto update() -> void;

        auto matrix() const -> const math::mat4&;

    private:
        auto roll_step(const math::vec3& pivot, float t, const math::vec2& direction) const -> std::pair<math::vec3, float>;

        math::mat4 _matrix { 1.0f };

        math::vec3 _position { };
        math::vec3 _pivot    { };

        math::quat _orientation       { 1.0f };
        math::quat _start_orientation { 1.0f };

        float  _animation_time { };

        math::vec2 _roll_direction { };
        int32_t _square_index_x { };
        int32_t _square_index_z { };

        int32_t _max_square_index { 2 };

        bool  _rolling { };

        static constexpr auto radius { size / math::sqrt2 };
    };
}