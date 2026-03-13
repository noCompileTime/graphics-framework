#include "object.hpp"

#include "core/time.hpp"

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

        if (std::abs(next_index_x) > _max_square_index ||
            std::abs(next_index_z) > _max_square_index)
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

        _rolling        = true;
        _roll_direction = direction;
        _animation_time = 0.0f;
    }

    auto Object::update() -> void
    {
        if (!_rolling)
        {
            return;
        }

        _animation_time += Time::delta_time() * 2.0f;

        const auto t = std::clamp(_animation_time, 0.0f, 1.0f);

        const auto [center, rotation] = roll_step(_pivot, _start_rotation, t, _roll_direction);

        if (t >= 1.0f)
        {
            _rolling        = false;
            _animation_time = 0.0f;
            _position       = center;
            _rotation       = rotation;
        }

        math::vec3 axis {};

        if (_roll_direction.x != 0.0f)
        {
            axis = { 0.0f, 0.0f, -_roll_direction.x };
        }
        else
        {
            axis = { _roll_direction.y, 0.0f, 0.0f };
        }

        math::quat quat_rotation;
        quat_rotation.rotation(axis, rotation);

        _matrix = quat_rotation.matrix();
        _matrix.translation({ center.x, center.y, center.z });
    }

    auto Object::matrix() const -> const math::mat4&
    {
        return _matrix;
    }

    auto Object::roll_step(const math::vec3& pivot, const float ps0, const float t, const math::vec2& direction) const -> std::pair<math::vec3, float>
    {
        constexpr auto half_pi = math::pi * 0.5f;
        const auto theta = t * half_pi;

        const auto dir_x = direction.x;
        const auto dir_z = direction.y;

        math::vec3 center { };
        auto psi = ps0;

        if (dir_x != 0.0f)
        {
            const auto base_phi = dir_x > 0.0f ? 3.0f * math::pi * 0.25f
                                               : math::pi * 0.25f;
            const auto phi = base_phi - theta * dir_x;
            psi = ps0 - theta * dir_x;

            center =
            {
                pivot.x + radius * std::cos(phi),
                pivot.y + radius * std::sin(phi),
                pivot.z
            };
        }
        else
        {
            const auto base_phi = dir_z > 0.0f ? 3.0f * math::pi * 0.25f
                                               : math::pi * 0.25f;
            const auto phi = base_phi - theta * dir_z;
            psi = ps0 - theta * dir_z;

            center =
            {
                pivot.x,
                pivot.y + radius * std::sin(phi),
                pivot.z + radius * std::cos(phi)
            };
        }

        return { center, psi };
    }
}