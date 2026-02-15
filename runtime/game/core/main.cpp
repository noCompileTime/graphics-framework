#include "core/file.hpp"
#include "core/time.hpp"

#include "core/input_manager.hpp"
#include "core/platform_factory.hpp"
#include "core/window_manager.hpp"

#include "opengl/constants/buffer.hpp"
#include "opengl/constants/commands.hpp"
#include "opengl/constants/common.hpp"
#include "opengl/constants/pipeline.hpp"
#include "opengl/constants/shader.hpp"
#include "opengl/constants/texture.hpp"
#include "opengl/constants/texture_sampler.hpp"

#include "opengl/functions.hpp"

#include "opengl/commands.hpp"
#include "opengl/pipeline.hpp"
#include "opengl/shader.hpp"
#include "opengl/texture.hpp"
#include "opengl/texture_sampler.hpp"
#include "opengl/vertex_array.hpp"

#include "tools/shaders_converter.hpp"

#include "images/tga_image.hpp"
#include "models/obj_model.hpp"

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
    tools::ShadersConverter::convert_each(BASE_SHADERS_PATH, "./", 3600);

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

    core::InputManager input_manager;

    window_manager.window_input().callbacks.on_key_press = [&](const core::input::code key, const core::input::state state) noexcept
    {
        input_manager.update(key, state);
    };

    window_manager.window_input().callbacks.on_btn_press = [&](const core::input::code btn, const core::input::state state) noexcept
    {
        input_manager.update(btn, state);
    };

    window_manager.window_events().callbacks.on_close = [&] noexcept
    {
        window_active = false;
    };

    window_manager.window().show();

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

    opengl::ShaderStage base_model_shader_vert { opengl::constants::vertex_shader };
    base_model_shader_vert.create();
    base_model_shader_vert.source(core::File::read("base_model_shader.vert", std::ios::binary));

    opengl::ShaderStage base_model_shader_frag { opengl::constants::fragment_shader };
    base_model_shader_frag.create();
    base_model_shader_frag.source(core::File::read("base_model_shader.frag", std::ios::binary));

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

    opengl::Shader base_model_shader;
    base_model_shader.create();
    base_model_shader.attach(base_model_shader_vert);
    base_model_shader.attach(base_model_shader_frag);
    base_model_shader.link();

    auto [cube_vertices, cube_elements] = models::ObjModel::load("base_cube.obj").geometry;

    opengl::Buffer cube_vbo;
    cube_vbo.create();
    cube_vbo.storage(core::data::make_buffer(cube_vertices), opengl::constants::default_usage);

    opengl::Buffer cube_ebo;
    cube_ebo.create();
    cube_ebo.storage(core::data::make_buffer(cube_elements), opengl::constants::default_usage);

    opengl::VertexArray cube_vao;
    cube_vao.create();
    cube_vao.attach_vertices(cube_vbo, sizeof(core::vertex::type::model));
    cube_vao.attach_elements(cube_ebo);

    cube_vao.attach({ 0, 3, opengl::constants::type_float, offsetof(core::vertex::type::model, position.x) });
    cube_vao.attach({ 1, 2, opengl::constants::type_float, offsetof(core::vertex::type::model, texcoord.x) });
    cube_vao.attach({ 2, 3, opengl::constants::type_float, offsetof(core::vertex::type::model,   normal.x) });

    opengl::Buffer object_vbo;
    object_vbo.create();
    object_vbo.storage(core::data::make_buffer(vertices), opengl::constants::default_usage);

    opengl::VertexArray object_vao;
    object_vao.create();
    object_vao.attach_vertices(object_vbo, sizeof(math::vec3));
    object_vao.attach({ 0, 3, opengl::constants::type_float, offsetof(math::vec3, x) });

    opengl::TextureSampler base_sampler;
    base_sampler.create();
    base_sampler.parameter(opengl::constants::texture_min_filter, opengl::constants::nearest);
    base_sampler.parameter(opengl::constants::texture_mag_filter, opengl::constants::nearest);

    auto base_image = images::TgaImage::load("base_albedo.tga");

    opengl::Texture square_texture { opengl::constants::texture_2d };
    square_texture.create();
    square_texture.storage(base_image, opengl::constants::rgb8, opengl::constants::default_levels);
    square_texture.upload (base_image, opengl::constants::rgb,  opengl::constants::default_level);

    math::mat4 transform { 1.0f };

    opengl::Buffer transform_ubo;
    transform_ubo.create();
    transform_ubo.storage(core::data::make_buffer(&transform), opengl::constants::dynamic_draw);
    transform_ubo.bind(opengl::constants::uniform_buffer, std::to_underlying(core::data::binding::buffer::transform));

    auto aspect_ratio = static_cast<float>(window_width) /
                        static_cast<float>(window_height);

    core::data::camera base_camera_data;
    base_camera_data.view.translation({ 0.0f, 0.0f, -2.5f });
    base_camera_data.projection.perspective(math::radians(45.0f), aspect_ratio, 0.1f, 100.0f);

    opengl::Buffer camera_ubo;
    camera_ubo.create();
    camera_ubo.storage(make_buffer(&base_camera_data), opengl::constants::default_usage);
    camera_ubo.bind(opengl::constants::uniform_buffer, std::to_underlying(core::data::binding::buffer::camera));

    opengl::Buffer material_ubo;
    material_ubo.create();
    material_ubo.storage(sizeof(math::rgb), opengl::constants::dynamic_draw);
    material_ubo.bind(opengl::constants::uniform_buffer, std::to_underlying(core::data::binding::buffer::material));

    math::mat4 ground_transform({ 5.0f, 0.5f, 1.0f });
               ground_transform.translation({ 0.0f, -(size - quarter_size), 0.0f });

    math::mat4 left_wall_transform(wall_scale);
               left_wall_transform.translation({ -1.375f, -quarter_size, 0.0f });

    math::mat4 right_wall_transform(wall_scale);
               right_wall_transform.translation({ 1.375f, -quarter_size, 0.0f });

    core::Object object;

    input_manager.input_actions().assign(core::input::code::key_right, [&] noexcept
    {
        object.roll(1.0f);
    });

    input_manager.input_actions().assign(core::input::code::key_left, [&] noexcept
    {
        object.roll(-1.0f);
    });

    opengl::Pipeline::enable(opengl::constants::depth_test);
    opengl::Pipeline::enable(opengl::constants::cull_face);

    core::Time time;
               time.start();

    while (window_active)
    {
        time.tick();

        window_manager.update();
         input_manager.update();

                object.update();

        opengl::Commands::clear(0.105f, 0.235f, 0.325f);
        opengl::Commands::clear(opengl::constants::color_buffer | opengl::constants::depth_buffer);

        base_model_shader.bind();

          base_sampler.bind(std::to_underlying(core::data::binding::texture::albedo));
        square_texture.bind(std::to_underlying(core::data::binding::texture::albedo));

            cube_vao.bind();

        math::quat quat_rotation;
        quat_rotation.rotation({ 0.0f, 1.0f, 0.0f }, core::Time::total_time());

        auto quat_matrix = quat_rotation.matrix();

        transform_ubo.upload(core::data::make_buffer(&quat_matrix), opengl::constants::default_offset);

        opengl::Commands::draw_elements(opengl::constants::triangles, cube_elements.size(), opengl::constants::default_offset);

        base_shader.bind();
         object_vao.bind();

        transform_ubo.upload(core::data::make_buffer(&ground_transform), opengl::constants::default_offset);
         material_ubo.upload(core::data::make_buffer(&ground_rgb), opengl::constants::default_offset);

        opengl::Commands::draw_vertices(opengl::constants::triangles, vertices.size(), opengl::constants::default_offset);

        transform_ubo.upload(core::data::make_buffer(&left_wall_transform), opengl::constants::default_offset);

        opengl::Commands::draw_vertices(opengl::constants::triangles, vertices.size(), opengl::constants::default_offset);

        transform_ubo.upload(core::data::make_buffer(&right_wall_transform), opengl::constants::default_offset);

        opengl::Commands::draw_vertices(opengl::constants::triangles, vertices.size(), opengl::constants::default_offset);

        window_manager.window_context().update();
    }

    transform_ubo.destroy();
     material_ubo.destroy();
       camera_ubo.destroy();

    cube_vbo.destroy();
    cube_ebo.destroy();
    cube_vao.destroy();

    object_vbo.destroy();
    object_vao.destroy();

    base_sprite_shader.destroy();
    base_shader.destroy();

    window_manager.release();

    return 0;
}