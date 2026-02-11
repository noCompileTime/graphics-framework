#pragma once

constexpr auto             size =        0.5f; // TODO move this in object? same for platforms?
constexpr auto        half_size = size * 0.5f;
constexpr auto     quarter_size = size * 0.25f;

constexpr math::rgb  object_rgb { 1.000f, 0.773f, 0.059f };
constexpr math::rgb  ground_rgb { 0.388f, 0.639f, 0.380f };

constexpr math::vec3 wall_scale { 0.5f, 1.5f, 1.0f };