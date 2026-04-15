#include "core/file.hpp"
#include "core/primitives.hpp"
#include "core/time.hpp"
#include "core/utility.hpp"

#include "core/input_manager.hpp"
#include "core/platform_factory.hpp"
#include "core/window_manager.hpp"

#include "core/shader_converter.hpp"

#include "core/binding/buffer.hpp"
#include "core/binding/texture.hpp"

#include "opengl/commands.hpp"
#include "opengl/functions.hpp"
#include "opengl/headers.hpp" // TODO to be removed at some point
#include "opengl/pipeline.hpp"
#include "opengl/pipeline_debug.hpp"
#include "opengl/sampler.hpp"
#include "opengl/shader.hpp"
#include "opengl/texture.hpp"
#include "opengl/vertex_array.hpp"

#include "images/tga_image.hpp"
#include "models/obj_model.hpp"

#include "object.hpp"

auto main() -> std::int32_t
{
    core::ShaderConverter::convert_each(BASE_SHADERS_PATH, "shaders", 3600);

    constexpr auto window_width  { 1280 };
    constexpr auto window_height {  720 };

    core::window::settings window_settings
    {
        PROJECT_NAME, window_width, window_height, 0
    };

    //window_settings.debug = true;

                   auto window_active { true };
    core::WindowManager window_manager;
                        window_manager.init(core::PlatformFactory::create(), window_settings);

    core::InputManager input_manager;

    window_manager.input().callbacks.on_btn_update = [&](const core::input::code code, const core::input::state state) noexcept
    {
        input_manager.state().update(code, state);
    };

    window_manager.events().callbacks.on_close = [&] noexcept
    {
        window_active = false;
    };

    window_manager.window().show();

    // TODO begin - put this under some graphics class

    opengl::Functions::init();

    if (window_settings.debug)
    {
        opengl::PipelineDebug::enable();
    }

    // TODO end - put this under some graphics class

    /* shaders */

    opengl::ShaderStage base_shader_vertex { opengl::constants::vertex_shader };
    base_shader_vertex.create();
    base_shader_vertex.source(core::File::read("shaders/base_shader.vert"));

    opengl::ShaderStage base_shader_fragment { opengl::constants::fragment_shader };
    base_shader_fragment.create();
    base_shader_fragment.source(core::File::read("shaders/base_shader.frag"));

    opengl::ShaderStage debug_shader_vertex { opengl::constants::vertex_shader };
    debug_shader_vertex.create();
    debug_shader_vertex.source(core::File::read("shaders/base_debug_shader.vert"));

    opengl::ShaderStage debug_shader_fragment { opengl::constants::fragment_shader };
    debug_shader_fragment.create();
    debug_shader_fragment.source(core::File::read("shaders/base_debug_shader.frag"));

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

    opengl::Shader debug_shader;
    debug_shader.create();
    debug_shader.attach(debug_shader_vertex);
    debug_shader.attach(debug_shader_fragment);
    debug_shader.link();

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
    cube_vao.attach({ 1, 3, opengl::constants::float_type, offsetof(core::vertex::type::model,   normal.x) });
    cube_vao.attach({ 2, 2, opengl::constants::float_type, offsetof(core::vertex::type::model, texcoord.x) });

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
    ground_vao.attach({ 1, 3, opengl::constants::float_type, offsetof(core::vertex::type::model,   normal.x) });
    ground_vao.attach({ 2, 2, opengl::constants::float_type, offsetof(core::vertex::type::model, texcoord.x) });

    auto [axis_vertices, axis_elements] = core::Axis::create({ 10.0f, 10.0f, 10.0f });

    opengl::Buffer axis_vbo;
    axis_vbo.create();
    axis_vbo.storage(core::as_bytes(axis_vertices), opengl::constants::static_draw);

    opengl::Buffer axis_ebo;
    axis_ebo.create();
    axis_ebo.storage(core::as_bytes(axis_elements), opengl::constants::static_draw);

    opengl::VertexArray axis_vao;
    axis_vao.create();
    axis_vao.attach_vertices(axis_vbo, sizeof(core::vertex::type::editor));
    axis_vao.attach_elements(axis_ebo);

    axis_vao.attach({ 0, 3, opengl::constants::float_type, offsetof(core::vertex::type::editor, position.x) });
    axis_vao.attach({ 1, 3, opengl::constants::float_type, offsetof(core::vertex::type::editor, extra.x) });

    //auto [debug_vertices, debug_elements] = core::Primitives::create_bounding_sphere(32, 0.5f, { 1.0f, 1.0f, 1.0f });
    //auto [debug_vertices, debug_elements] = core::Primitives::create_bounding_box({ 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f });

    //opengl::Buffer debug_vbo;
    //debug_vbo.create();
    //debug_vbo.storage(core::as_bytes(debug_vertices), opengl::constants::static_draw);

    //opengl::Buffer debug_ebo;
    //debug_ebo.create();
    //debug_ebo.storage(core::as_bytes(debug_elements), opengl::constants::static_draw);

    //opengl::VertexArray debug_vao;
    //debug_vao.create();
    //debug_vao.attach_vertices(debug_vbo, sizeof(core::vertex::type::editor));
    //debug_vao.attach_elements(debug_ebo);

    //debug_vao.attach({ 0, 3, opengl::constants::float_type, offsetof(core::vertex::type::editor, position.x) });
    //debug_vao.attach({ 1, 3, opengl::constants::float_type, offsetof(core::vertex::type::editor, extra.x) });

    opengl::Sampler base_sampler;
    base_sampler.create();
    base_sampler.parameter(opengl::constants::min_filter, opengl::constants::nearest);
    base_sampler.parameter(opengl::constants::mag_filter, opengl::constants::nearest);

    auto base_image = images::TgaImage::load("base_cube_albedo.tga");

    opengl::Texture base_texture { opengl::constants::texture_2d };
    base_texture.create();
    base_texture.storage(base_image.width, base_image.height, opengl::constants::rgb8, 1);
    base_texture.upload(base_image.width, base_image.height, opengl::constants::rgb, 0, opengl::constants::unsigned_byte, base_image.pixels);

    math::quat x_view_rotation;
    math::quat y_view_rotation;

    x_view_rotation.rotation({ 1.0f, 0.0f, 0.0f }, math::radians( 45.0f));
    y_view_rotation.rotation({ 0.0f, 1.0f, 0.0f }, math::radians(-45.0f));

    //auto view_matrix = (x_view_rotation * y_view_rotation).matrix();
    auto view_matrix = x_view_rotation.matrix();
         view_matrix.translation({ 0.0f, 0.0f, -5.0f });

    core::data::camera camera_data;
    camera_data.view = view_matrix;
    camera_data.projection.perspective(math::radians(45.0f), static_cast<float>(window_width) / static_cast<float>(window_height), 0.1f, 100.0f);

    opengl::Buffer camera_ubo;
    camera_ubo.create();
    camera_ubo.storage(core::as_bytes(camera_data), opengl::constants::static_draw);
    camera_ubo.bind(opengl::constants::uniform_buffer, core::as_base(core::binding::buffer::camera));

    opengl::Buffer transform_ubo;
    transform_ubo.create();
    transform_ubo.storage(sizeof(math::mat4), opengl::constants::dynamic_draw);
    transform_ubo.bind(opengl::constants::uniform_buffer, core::as_base(core::binding::buffer::transform));

    opengl::Buffer material_ubo;
    material_ubo.create();
    material_ubo.storage(sizeof(math::rgb), opengl::constants::dynamic_draw);
    material_ubo.bind(opengl::constants::uniform_buffer, core::as_base(core::binding::buffer::material));

    math::mat4 ground_transform { 1.0f };

    core::Object object;

    input_manager.actions().assign(core::input::code::key_d, [&] noexcept
    {
        object.roll({ 1.0f, 0.0f });
    });

    input_manager.actions().assign(core::input::code::key_a, [&] noexcept
    {
        object.roll({ -1.0f, 0.0f });
    });

    input_manager.actions().assign(core::input::code::key_w, [&] noexcept
    {
        object.roll({ 0.0f, -1.0f });
    });

    input_manager.actions().assign(core::input::code::key_s, [&] noexcept
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
            opengl::Pipeline::polygon(opengl::constants::front_and_back, opengl::constants::fill_mode);
            //opengl::Pipeline::enable(opengl::constants::cull_test);
        }
        else
        {
            opengl::Pipeline::polygon(opengl::constants::front_and_back, opengl::constants::line_mode);
            //opengl::Pipeline::disable(opengl::constants::cull_test);
        }

        wireframe_mode = !wireframe_mode;
    });

    opengl::Pipeline::enable(opengl::constants::multisample);

    opengl::Pipeline::enable(opengl::constants::depth_test);
    opengl::Pipeline::enable(opengl::constants::cull_test);

    core::Time time;
    time.start();

    while (window_active)
    {
        time.tick();

        window_manager.update();
         input_manager.update();

                object.update();

        opengl::Commands::clear(0.2745f, 0.5176f, 0.1961f, 1.0f);
        opengl::Commands::clear(opengl::constants::color_buffer | opengl::constants::depth_buffer);

         model_shader.bind();

         base_sampler.bind(core::as_base(core::binding::texture::albedo));
         base_texture.bind(core::as_base(core::binding::texture::albedo));

             cube_vao.bind();

        transform_ubo.upload(core::as_bytes(object.matrix()), 0);

        opengl::Commands::draw_elements(opengl::constants::triangles, cube_elements.size(), opengl::constants::unsigned_int, 0);

           ground_vao.bind();

        transform_ubo.upload(core::as_bytes(ground_transform), 0);

        opengl::Commands::draw_elements(opengl::constants::triangles, ground_elements.size(), opengl::constants::unsigned_int, 0);

        debug_shader.bind();

        axis_vao.bind();

        opengl::Commands::draw_elements(opengl::constants::lines, axis_elements.size(), opengl::constants::unsigned_int, 0);

        //debug_vao.bind();

        //opengl::Commands::draw_elements(opengl::constants::lines, debug_elements.size(), opengl::constants::unsigned_int, 0);

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