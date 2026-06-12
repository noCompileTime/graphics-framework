#include "object.hpp"

#include "core/time.hpp"

#include "math/functions.hpp"

namespace core
{
    auto Object::roll(const math::vec2& direction) -> void
    {
        if (_rolling)
        {
            return;
        }

        if ((direction.x == 0.0f && direction.y == 0.0f) ||
            (direction.x != 0.0f && direction.y != 0.0f))
        {
            return;
        }

        const auto next_index_x = _square_index_x + static_cast<int32_t>(direction.x);
        const auto next_index_z = _square_index_z + static_cast<int32_t>(direction.y);

        if (math::abs(next_index_x) > _max_square_index ||
            math::abs(next_index_z) > _max_square_index)
        {
            return;
        }

        _square_index_x = next_index_x;
        _square_index_z = next_index_z;

        _pivot =
        {
            _position.x + half_size * direction.x,
            _position.y - half_size,
            _position.z + half_size * direction.y
        };

        _rolling           = true;
        _start_orientation = _orientation;
        _roll_direction    = direction;
        _animation_time    = 0.0f;
    }

    auto Object::update() -> void
    {
        if (!_rolling)
        {
            return;
        }

        _animation_time += Time::delta_time() * 2.0f;

        const auto t = std::clamp(_animation_time, 0.0f, 1.0f);

        const auto [center, angle] = roll_step(_pivot, t, _roll_direction);

        const math::vec3 axis
        {
            _roll_direction.y, 0.0f,
            -_roll_direction.x
        };

        const auto delta_rotation = math::quat::rotation(axis, angle);

        auto orientation = delta_rotation * _start_orientation;
             orientation.normalize();

        if (t >= 1.0f)
        {
            _rolling        = false;
            _animation_time = 0.0f;
            _position       = center;
            _orientation    = orientation;
        }

        _matrix = static_cast<math::mat4>(orientation);
        _matrix.translation({ center.x, center.y, center.z });
    }

    auto Object::matrix() const -> const math::mat4&
    {
        return _matrix;
    }

    auto Object::roll_step(const math::vec3& pivot, const float t, const math::vec2& direction) -> std::pair<math::vec3, float>
    {
        constexpr auto half_pi = math::pi * 0.5f;
        const auto theta = t * half_pi;

        const auto dir_x = direction.x;
        const auto dir_z = direction.y;

        math::vec3 center;

        if (dir_x != 0.0f)
        {
            const auto base_phi = dir_x > 0.0f ? 3.0f * math::pi * 0.25f
                                               :        math::pi * 0.25f;
            const auto phi = base_phi - theta * dir_x;

            center =
            {
                pivot.x + radius * math::cos(phi),
                pivot.y + radius * math::sin(phi),
                pivot.z
            };
        }
        else
        {
            const auto base_phi = dir_z > 0.0f ? 3.0f * math::pi * 0.25f
                                               :        math::pi * 0.25f;
            const auto phi = base_phi - theta * dir_z;

            center =
            {
                pivot.x,
                pivot.y + radius * math::sin(phi),
                pivot.z + radius * math::cos(phi)
            };
        }

        return { center, theta };
    }
}