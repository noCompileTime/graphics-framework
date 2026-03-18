#include "core/file.hpp"
#include "core/time.hpp"
#include "core/utility.hpp"

#include "core/input_manager.hpp"
#include "core/platform_factory.hpp"
#include "core/window_manager.hpp"

#include "core/binding/buffer.hpp"
#include "core/binding/texture.hpp"

#include "opengl/constants/buffer.hpp"
#include "opengl/constants/commands.hpp"
#include "opengl/constants/common.hpp"
#include "opengl/constants/pipeline.hpp"
#include "opengl/constants/pipeline_debug.hpp"
#include "opengl/constants/shader.hpp"
#include "opengl/constants/texture.hpp"
#include "opengl/constants/texture_sampler.hpp"

#include "opengl/commands.hpp"
#include "opengl/functions.hpp"
#include "opengl/pipeline.hpp"
#include "opengl/pipeline_debug.hpp"
#include "opengl/shader.hpp"
#include "opengl/texture.hpp"
#include "opengl/texture_sampler.hpp"
#include "opengl/vertex_array.hpp"

#include "tools/shaders_converter.hpp"

#include "images/tga_image.hpp"
#include "models/obj_model.hpp"

#include "object.hpp"

auto main() -> int32_t
{
    tools::ShadersConverter::convert_each(BASE_SHADERS_PATH, "shaders", 3600);

    constexpr auto window_width  { 1280 };
    constexpr auto window_height {  720 };

    core::window::configuration window_configuration
    {
       "Game Framework",
        window_width,
        window_height
    };

    //window_configuration.debug = true;

                   auto window_active { true };
    core::WindowManager window_manager;
                        window_manager.init(core::PlatformFactory::create(), window_configuration);

    core::InputManager  input_manager;

    window_manager.input().callbacks.on_key_press = [&](const core::input::code key, const core::input::state state) noexcept
    {
        input_manager.state().update(key, state);
    };

    window_manager.input().callbacks.on_btn_press = [&](const core::input::code btn, const core::input::state state) noexcept
    {
        input_manager.state().update(btn, state);
    };

    window_manager.events().callbacks.on_close = [&] noexcept
    {
        window_active = false;
    };

    window_manager.window().show();

    opengl::Functions::init();

    if (window_configuration.debug)
    {
        opengl::PipelineDebug::init();
    }

    /* shaders */

    opengl::ShaderStage base_shader_vertex { opengl::constants::vertex_shader };
    base_shader_vertex.create();
    base_shader_vertex.source(core::File::read("shaders/base_shader.vert"));

    opengl::ShaderStage base_shader_fragment { opengl::constants::fragment_shader };
    base_shader_fragment.create();
    base_shader_fragment.source(core::File::read("shaders/base_shader.frag"));

    opengl::ShaderStage sprite_shader_vertex { opengl::constants::vertex_shader };
    sprite_shader_vertex.create();
    sprite_shader_vertex.source(core::File::read("shaders/base_sprite_shader.vert"));

    opengl::ShaderStage sprite_shader_fragment { opengl::constants::fragment_shader };
    sprite_shader_fragment.create();
    sprite_shader_fragment.source(core::File::read("shaders/base_sprite_shader.frag"));

    opengl::ShaderStage model_shader_vertex { opengl::constants::vertex_shader };
    model_shader_vertex.create();
    model_shader_vertex.source(core::File::read("shaders/base_model_shader.vert"));

    opengl::ShaderStage model_shader_fragment { opengl::constants::fragment_shader };
    model_shader_fragment.create();
    model_shader_fragment.source(core::File::read("shaders/base_model_shader.frag"));

    opengl::Shader base_shader;
    base_shader.create();
    base_shader.attach(base_shader_vertex);
    base_shader.attach(base_shader_fragment);
    base_shader.link();

    opengl::Shader sprite_shader;
    sprite_shader.create();
    sprite_shader.attach(sprite_shader_vertex);
    sprite_shader.attach(sprite_shader_fragment);
    sprite_shader.link();

    opengl::Shader model_shader;
    model_shader.create();
    model_shader.attach(model_shader_vertex);
    model_shader.attach(model_shader_fragment);
    model_shader.link();

    auto [base_geometries] = models::ObjModel::load("base_scene_model.obj");

    auto [  cube_vertices,   cube_elements] = base_geometries[0];
    auto [ground_vertices, ground_elements] = base_geometries[1];

    opengl::Buffer cube_vbo;
    cube_vbo.create();
    cube_vbo.storage(core::as_bytes(cube_vertices), opengl::constants::static_draw);

    opengl::Buffer cube_ebo;
    cube_ebo.create();
    cube_ebo.storage(core::as_bytes(cube_elements), opengl::constants::static_draw);

    opengl::VertexArray cube_vao;
    cube_vao.create();
    cube_vao.attach_vertices(cube_vbo, sizeof(core::vertex::type::model));
    cube_vao.attach_elements(cube_ebo);

    cube_vao.attach({ 0, 3, opengl::constants::float_type, offsetof(core::vertex::type::model, position.x) });
    cube_vao.attach({ 1, 2, opengl::constants::float_type, offsetof(core::vertex::type::model, texcoord.x) });
    cube_vao.attach({ 2, 3, opengl::constants::float_type, offsetof(core::vertex::type::model,   normal.x) });

    opengl::Buffer ground_vbo;
    ground_vbo.create();
    ground_vbo.storage(core::as_bytes(ground_vertices), opengl::constants::static_draw);

    opengl::Buffer ground_ebo;
    ground_ebo.create();
    ground_ebo.storage(core::as_bytes(ground_elements), opengl::constants::static_draw);

    opengl::VertexArray ground_vao;
    ground_vao.create();
    ground_vao.attach_vertices(ground_vbo, sizeof(core::vertex::type::model));
    ground_vao.attach_elements(ground_ebo);

    ground_vao.attach({ 0, 3, opengl::constants::float_type, offsetof(core::vertex::type::model, position.x) });
    ground_vao.attach({ 1, 2, opengl::constants::float_type, offsetof(core::vertex::type::model, texcoord.x) });
    ground_vao.attach({ 2, 3, opengl::constants::float_type, offsetof(core::vertex::type::model,   normal.x) });

    opengl::TextureSampler base_sampler;
    base_sampler.create();
    base_sampler.parameter(opengl::constants::min_filter, opengl::constants::nearest);
    base_sampler.parameter(opengl::constants::mag_filter, opengl::constants::nearest);

    auto base_image = images::TgaImage::load("base_cube_albedo.tga");

    opengl::Texture base_texture { opengl::constants::texture_2d };
    base_texture.create();
    base_texture.storage(base_image.width, base_image.height, opengl::constants::rgb8, 1);
    base_texture.upload(base_image.width, base_image.height, opengl::constants::rgb, 0, base_image.pixels);

    opengl::Buffer transform_ubo;
    transform_ubo.create();
    transform_ubo.storage(sizeof(math::mat4), opengl::constants::dynamic_draw);
    transform_ubo.bind(opengl::constants::uniform_buffer, core::as_base(core::binding::buffer::transform));

    auto aspect_ratio = static_cast<float>(window_width) /
                        static_cast<float>(window_height);

    core::data::camera camera_data;
    camera_data.view.translation({ 0.0f, -0.25f, -3.5f });
    camera_data.projection.perspective(math::radians(45.0f), aspect_ratio, 0.1f, 100.0f);

    opengl::Buffer camera_ubo;
    camera_ubo.create();
    camera_ubo.storage(core::as_bytes(camera_data), opengl::constants::static_draw);
    camera_ubo.bind(opengl::constants::uniform_buffer, core::as_base(core::binding::buffer::camera));

    opengl::Buffer material_ubo;
    material_ubo.create();
    material_ubo.storage(sizeof(math::rgb), opengl::constants::dynamic_draw);
    material_ubo.bind(opengl::constants::uniform_buffer, core::as_base(core::binding::buffer::material));

    math::mat4 ground_transform { 1.0f };

    core::Object object;

    input_manager.actions().assign(core::input::code::key_right, [&] noexcept
    {
        object.roll({ 1.0f, 0.0f });
    });

    input_manager.actions().assign(core::input::code::key_left, [&] noexcept
    {
        object.roll({ -1.0f, 0.0f });
    });

    input_manager.actions().assign(core::input::code::key_up, [&] noexcept
    {
        object.roll({ 0.0f, -1.0f });
    });

    input_manager.actions().assign(core::input::code::key_down, [&] noexcept
    {
        object.roll({ 0.0f, 1.0f });
    });

    input_manager.actions().assign(core::input::code::key_escape, [&] noexcept
    {
       window_active = false;
    });

    auto wireframe_mode { false };

    input_manager.actions().assign(core::input::code::key_tab, [&] noexcept
    {
        if (wireframe_mode)
        {
            opengl::PipelineDebug::polygon_mode(opengl::constants::fill_mode);
            //opengl::Pipeline::enable(opengl::constants::cull_test);
        }
        else
        {
            opengl::PipelineDebug::polygon_mode(opengl::constants::line_mode);
            //opengl::Pipeline::disable(opengl::constants::cull_test);
        }

        wireframe_mode = !wireframe_mode;
    });

    opengl::Pipeline::enable(opengl::constants::depth_test);
    opengl::Pipeline::enable(opengl::constants:: cull_test);

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

         model_shader.bind();

         base_sampler.bind(core::as_base(core::binding::texture::albedo));
         base_texture.bind(core::as_base(core::binding::texture::albedo));

             cube_vao.bind();

        transform_ubo.upload(core::as_bytes(object.matrix()), 0);

        opengl::Commands::draw_elements(opengl::constants::triangles, cube_elements.size(), 0);

           ground_vao.bind();

        transform_ubo.upload(core::as_bytes(ground_transform), 0);

        opengl::Commands::draw_elements(opengl::constants::triangles, ground_elements.size(), 0);

        window_manager.context().update();
    }

     transform_ubo.destroy();
      material_ubo.destroy();
        camera_ubo.destroy();

          cube_vbo.destroy();
          cube_ebo.destroy();
          cube_vao.destroy();

        ground_vbo.destroy();
        ground_ebo.destroy();
        ground_vao.destroy();

     sprite_shader.destroy();
      model_shader.destroy();
       base_shader.destroy();

    window_manager.release();

    return 0;
}