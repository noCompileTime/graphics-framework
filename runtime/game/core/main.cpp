#include "core/file.hpp"
#include "core/input_manager.hpp"
#include "core/platform_factory.hpp"
#include "core/time.hpp"
#include "core/window_manager.hpp"

#include "opengl/commands.hpp"
#include "opengl/shader.hpp"
#include "opengl/texture.hpp"
#include "opengl/texture_sampler.hpp"
#include "opengl/vertex_array.hpp"

#include "opengl/functions.hpp"
#include "opengl/headers.hpp"

#include "tools/shaders_converter.hpp"

#include "images/tga_image.hpp"

#include "object.hpp"

const std::vector<core::vertex::type::basic> vertices
{
    { -half_size, -half_size, 0.0f },
    {  half_size, -half_size, 0.0f },
    {  half_size,  half_size, 0.0f },

    { -half_size, -half_size, 0.0f },
    {  half_size,  half_size, 0.0f },
    { -half_size,  half_size, 0.0f }
};

auto main() -> int32_t
{
    tools::ShadersConverter::convert_each(BASE_SHADERS_PATH, "/", 0);

    constexpr auto window_width  { 1280 };
    constexpr auto window_height {  720 };

    constexpr core::window::configuration window_configuration
    {
        "Graphics Framework",
         window_width,
         window_height
    };
                   auto window_active { true };
    core::WindowManager window_manager;
                        window_manager.init(core::PlatformFactory::create(), window_configuration);

    window_manager.window_events().callbacks.on_close = [&]
    {
        window_active = false;
    };

    window_manager.window().show();

    core::InputManager input_manager;

    window_manager.window_input().callbacks.on_key_press = [&](const core::input::code key, const core::input::state state)
    {
        input_manager.update(key, state);
    };

    window_manager.window_input().callbacks.on_btn_press = [&](const core::input::code btn, const core::input::state state)
    {
        input_manager.update(btn, state);
    };

    input_manager.input_actions().set_action(core::input::code::key_escape, [&]
    {
        window_active = false;
    });

    opengl::Functions::init();

    /* shaders */

    opengl::ShaderStage base_shader_vert { opengl::constants::vertex_shader };
    base_shader_vert.create();
    base_shader_vert.source(core::File::read("base_shader.vert", std::ios::binary));

    opengl::ShaderStage base_shader_frag { opengl::constants::fragment_shader };
    base_shader_frag.create();
    base_shader_frag.source(core::File::read("base_shader.frag", std::ios::binary));

    opengl::ShaderStage base_sprite_shader_vert { opengl::constants::vertex_shader };
    base_sprite_shader_vert.create();
    base_sprite_shader_vert.source(core::File::read("base_sprite_shader.vert", std::ios::binary));

    opengl::ShaderStage base_sprite_shader_frag { opengl::constants::fragment_shader };
    base_sprite_shader_frag.create();
    base_sprite_shader_frag.source(core::File::read("base_sprite_shader.frag", std::ios::binary));

    opengl::Shader base_shader;
    base_shader.create();
    base_shader.attach(base_shader_vert);
    base_shader.attach(base_shader_frag);
    base_shader.link();

    opengl::Shader base_sprite_shader;
    base_sprite_shader.create();
    base_sprite_shader.attach(base_sprite_shader_vert);
    base_sprite_shader.attach(base_sprite_shader_frag);
    base_sprite_shader.link();

    std::vector<core::vertex::type::sprite> square_vertices
    {
        { { -half_size, -half_size }, { 0.0f, 0.0f } },
        { {  half_size, -half_size }, { 1.0f, 0.0f } },
        { {  half_size,  half_size }, { 1.0f, 1.0f } },
        { { -half_size,  half_size }, { 0.0f, 1.0f } }
    };

    std::vector<uint32_t> square_elements
    {
        2, 1, 0,
        0, 3, 2
    };

    opengl::Buffer square_vbo;
    square_vbo.create();
    square_vbo.storage(core::data::make_buffer(square_vertices), 0);

    opengl::Buffer square_ebo;
    square_ebo.create();
    square_ebo.storage(core::data::make_buffer(square_elements), 0);

    opengl::VertexArray square_vao;
    square_vao.create();
    square_vao.attach_vertices(square_vbo, sizeof(core::vertex::type::sprite));
    square_vao.attach_elements(square_ebo);

    square_vao.attach({ 0, 2, opengl::constants::float_type, offsetof(core::vertex::type::sprite, position.x) });
    square_vao.attach({ 1, 2, opengl::constants::float_type, offsetof(core::vertex::type::sprite, texcoord.x) });

    opengl::Buffer object_vbo;
    object_vbo.create();
    object_vbo.storage(core::data::make_buffer(vertices), 0);

    opengl::VertexArray object_vao;
    object_vao.create();
    object_vao.attach_vertices(object_vbo, sizeof(math::vec3));
    object_vao.attach({ 0, 3, opengl::constants::float_type, offsetof(math::vec3, x) });

    opengl::TextureSampler base_sampler;
    base_sampler.create();
    base_sampler.parameter(opengl::constants::texture_min_filter, opengl::constants::nearest);
    base_sampler.parameter(opengl::constants::texture_mag_filter, opengl::constants::nearest);

    auto [pixels, width, height, channels] = images::TgaImage::load("squares.tga");

    opengl::Texture square_texture { opengl::constants::texture_2d };
    square_texture.create();
    square_texture.storage(width, height, opengl::constants::rgb8, 1);
    square_texture.upload (width, height, opengl::constants::rgb,  0, pixels.data());

    math::mat4 transform { 1.0f };

    opengl::Buffer transform_ubo;
    transform_ubo.create();
    transform_ubo.storage(core::data::make_buffer(&transform), opengl::constants::dynamic_draw);
    transform_ubo.bind(opengl::constants::uniform_buffer, std::to_underlying(core::data::buffer_location::transform));

    auto aspect_ratio = static_cast<float>(window_width) /
                        static_cast<float>(window_height);
    math::mat4 view { 1.0f };
               view.translation({ 0.0f, 0.0f, -2.5f });

    math::mat4 projection { 1.0f };
               projection.perspective(math::radians(45.0f), aspect_ratio, 0.1f, 100.0f);

    std::vector camera_matrices
    {
        view,
        projection
    };

    opengl::Buffer camera_ubo;
    camera_ubo.create();
    camera_ubo.storage(core::data::make_buffer(camera_matrices), 0);
    camera_ubo.bind(opengl::constants::uniform_buffer, std::to_underlying(core::data::buffer_location::camera));

    opengl::Buffer material_ubo;
    material_ubo.create();
    material_ubo.storage(core::data::make_null_buffer<math::rgb>(), opengl::constants::dynamic_draw);
    material_ubo.bind(opengl::constants::uniform_buffer, std::to_underlying(core::data::buffer_location::material));

    math::mat4 ground_transform({ 5.0f, 0.5f, 1.0f });
               ground_transform.translation({ 0.0f, -(size - quarter_size), 0.0f });

    math::mat4 left_wall_transform(wall_scale);
               left_wall_transform.translation({ -1.375f, -quarter_size, 0.0f });

    math::mat4 right_wall_transform(wall_scale);
               right_wall_transform.translation({ 1.375f, -quarter_size, 0.0f });

    core::Object  object;

    input_manager.input_actions().set_action(core::input::code::key_right, [&]
    {
        object.roll(1.0f);
    });

    input_manager.input_actions().set_action(core::input::code::key_left, [&]
    {
        object.roll(-1.0f);
    });

    core::Time time;
               time.start();

    while (window_active)
    {
        time.tick();

        window_manager.update();
         input_manager.update();

                object.update();

        opengl::Commands::clear(0.105f, 0.235f, 0.325f, 1.0f);
        opengl::Commands::clear(opengl::constants::color_buffer);

           base_sprite_shader.bind();

          base_sampler.bind(std::to_underlying(core::data::texture_location::albedo));
        square_texture.bind(std::to_underlying(core::data::texture_location::albedo));

            square_vao.bind();

        transform_ubo.upload(core::data::make_buffer(&object.matrix()), 0);

        opengl::Commands::draw_elements(opengl::constants::triangles, square_elements.size(), 0);

        base_shader.bind();
         object_vao.bind();

        transform_ubo.upload(core::data::make_buffer(&ground_transform), 0);
         material_ubo.upload(core::data::make_buffer(&ground_rgb), 0);

        opengl::Commands::draw_vertices(opengl::constants::triangles, vertices.size(), 0);

        transform_ubo.upload(core::data::make_buffer(&left_wall_transform), 0);

        opengl::Commands::draw_vertices(opengl::constants::triangles, vertices.size(), 0);

        transform_ubo.upload(core::data::make_buffer(&right_wall_transform), 0);

        opengl::Commands::draw_vertices(opengl::constants::triangles, vertices.size(), 0);

        window_manager.window_context().update();
    }

    transform_ubo.destroy();
       camera_ubo.destroy();

    square_vbo.destroy();
    square_ebo.destroy();
    square_vao.destroy();

    window_manager.release();

    return 0;
}