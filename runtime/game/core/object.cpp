#include "object.hpp"

#include "core/time.hpp"

namespace core
{
    auto Object::roll(const float direction) -> void
    {
        if (_rolling)
        {
            return;
        }

        if (const auto next_index  =     _square_index + static_cast<int32_t>(direction);
              std::abs(next_index) > _max_square_index)
        {
            return;
        }
        else
        {
            _square_index = next_index;
        }

        _pivot =
        {
            _position.x + half_size * direction,
            _position.y - half_size
        };

        _rolling        =  true;
        _roll_direction =  direction;
        _animation_time =  0.0f;
        _start_rotation = _rotation;
    }

    auto Object::update() -> void
    {
        if (!_rolling)
        {
            return;
        }

        _animation_time += Time::delta() * 2.0f;

        const auto t = std::clamp(_animation_time, 0.0f, 1.0f);

        const auto [center, rotation] = roll_step(_pivot, _start_rotation, t, _roll_direction);

        if (t >= 1.0f)
        {
            _rolling        = false;
            _animation_time = 0.0f;
            _position       = center;
            _rotation       = rotation;
        }

        math::quat quat_rotation;
                   quat_rotation.rotation({ 0.0f, 0.0f, 1.0f }, rotation);
        _matrix =  quat_rotation.matrix();
        _matrix.translation({ center.x,
                              center.y, 0.0f });
    }

    auto Object::matrix() const -> const math::mat4&
    {
        return _matrix;
    }

    auto Object::roll_step(const math::vec2& pivot, const float ps0, const float t, const float direction) const -> std::pair<math::vec2, float>
    {
        constexpr auto half_pi = math::pi * 0.5f;
        const     auto theta   = t * half_pi;

        const auto base_phi = direction > 0.0f ? 3.0f * math::pi * 0.25f :
                                                        math::pi * 0.25f;
        const auto phi = base_phi - theta * direction;
        const auto psi =      ps0 - theta * direction;

        return
        {
            {
                pivot.x + radius * std::cos(phi),
                pivot.y + radius * std::sin(phi)
            },
            psi
        };
    }
}