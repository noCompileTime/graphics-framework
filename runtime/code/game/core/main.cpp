#include "core/file.hpp"
#include "core/time.hpp"
#include "core/utility.hpp"

#include "core/data/camera.hpp"
#include "core/data/light.hpp"
#include "core/data/material.hpp"
#include "core/data/transform.hpp"

#include "core/input_manager.hpp"
#include "core/platform_factory.hpp"
#include "core/shader_converter.hpp"
#include "core/window_manager.hpp"

#include "binding/buffer.hpp"
#include "binding/texture.hpp"

#include "opengl/commands.hpp"
#include "opengl/framebuffer.hpp"
#include "opengl/functions.hpp"
#include "opengl/pipeline.hpp"
#include "opengl/pipeline_debug.hpp"
#include "opengl/renderbuffer.hpp"
#include "opengl/sampler.hpp"
#include "opengl/shader.hpp"
#include "opengl/vertex_array.hpp"

#include "opengl/constants/buffer.hpp"
#include "opengl/constants/commands.hpp"
#include "opengl/constants/common.hpp"
#include "opengl/constants/framebuffer.hpp"
#include "opengl/constants/pipeline.hpp"
#include "opengl/constants/renderbuffer.hpp"
#include "opengl/constants/sampler.hpp"
#include "opengl/constants/shader_stage.hpp"
#include "opengl/constants/texture.hpp"

#include "core/geometry/gizmo.hpp"
#include "core/geometry/primitive.hpp"
#include "core/geometry/sprite.hpp"

#include "images/tga_image.hpp"
#include "models/obj_model.hpp"

#include "object.hpp"

#include "math/functions.hpp"
#include "math/mat4_inverse.hpp"

#include "math/aabb.hpp"

struct ray
{
    math::vec3 origin;
    math::vec3 direction;
};

// TODO make it more generic with the width and height (maybe viewport?)
auto ray_to_world(const math::vec2& point, const int32_t window_width, const int32_t window_height, const core::data::camera& camera) noexcept
{
    const math::vec2 ndc
    {
               2.0f * point.x / static_cast<float>(window_width) - 1.0f,
        1.0f - 2.0f * point.y / static_cast<float>(window_height)
    };

    const auto inverse_matrix = inverse(camera.projection * camera.view);

    auto origin  = inverse_matrix * math::vec4 { ndc.x, ndc.y, -1.0f, 1.0f };
    auto finish  = inverse_matrix * math::vec4 { ndc.x, ndc.y,  1.0f, 1.0f };

         origin /= origin.w;
         finish /= finish.w;

    auto direction = static_cast<math::vec3>(finish - origin);
         direction.normalize();

    return ray
    {
        static_cast<math::vec3>(origin), direction
    };
}

auto intersects(const ray& ray, const math::aabb& aabb) noexcept
{
    const math::vec3 inverse_direction
    {
        1.0f / ray.direction.x,
        1.0f / ray.direction.y,
        1.0f / ray.direction.z
    };

    const auto [x0, y0, z0] = (aabb.min - ray.origin) * inverse_direction;
    const auto [x1, y1, z1] = (aabb.max - ray.origin) * inverse_direction;

    const math::vec3 tmin
    {
        math::min(x0, x1),
        math::min(y0, y1),
        math::min(z0, z1)
    };

    const math::vec3 tmax
    {
        math::max(x0, x1),
        math::max(y0, y1),
        math::max(z0, z1)
    };

    const auto near = math::max(math::max(tmin.x, tmin.y), tmin.z); // TODO rename this?
    const auto far  = math::min(math::min(tmax.x, tmax.y), tmax.z); // TODO rename this?

    return far >= math::max(near, 0.0f);
}

auto main() -> int32_t
{
    core::ShaderConverter::convert_each(BASE_SHADERS_PATH, "shaders", 3600);

    constexpr auto window_width  { 1280 };
    constexpr auto window_height {  720 };

    core::window::settings window_settings
    {
        "Game Framework", window_width, window_height, 0
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

    auto view_scale  = 1.0f;
    auto view_width  = static_cast<float>(window_width)  * view_scale;
    auto view_height = static_cast<float>(window_height) * view_scale;

    auto [view_vertices, view_elements] = core::geometry::Sprite::create(view_width, view_height);

    opengl::Buffer view_vbo;
    view_vbo.create();
    view_vbo.storage(core::as_bytes(view_vertices), opengl::constants::static_draw);

    opengl::Buffer view_ebo;
    view_ebo.create();
    view_ebo.storage(core::as_bytes(view_elements), opengl::constants::static_draw);

    opengl::VertexArray view_vao;
    view_vao.create();
    view_vao.attach(view_vbo, sizeof(core::geometry::vertex::sprite));
    view_vao.attach(view_ebo);
    view_vao.attach({ 0, offsetof(core::geometry::vertex::sprite, position), 2, opengl::constants::float_type });
    view_vao.attach({ 1, offsetof(core::geometry::vertex::sprite, texcoord), 2, opengl::constants::float_type });

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
    cube_vao.attach(cube_vbo, sizeof(core::geometry::vertex::model));
    cube_vao.attach(cube_ebo);
    cube_vao.attach({ 0, offsetof(core::geometry::vertex::model, position), 3, opengl::constants::float_type });
    cube_vao.attach({ 1, offsetof(core::geometry::vertex::model,   normal), 3, opengl::constants::float_type });
    cube_vao.attach({ 2, offsetof(core::geometry::vertex::model, texcoord), 2, opengl::constants::float_type });

    opengl::Buffer ground_vbo;
    ground_vbo.create();
    ground_vbo.storage(core::as_bytes(ground_vertices), opengl::constants::static_draw);

    opengl::Buffer ground_ebo;
    ground_ebo.create();
    ground_ebo.storage(core::as_bytes(ground_elements), opengl::constants::static_draw);

    opengl::VertexArray ground_vao;
    ground_vao.create();
    ground_vao.attach(ground_vbo, sizeof(core::geometry::vertex::model));
    ground_vao.attach(ground_ebo);
    ground_vao.attach({ 0, offsetof(core::geometry::vertex::model, position), 3, opengl::constants::float_type });
    ground_vao.attach({ 1, offsetof(core::geometry::vertex::model,   normal), 3, opengl::constants::float_type });
    ground_vao.attach({ 2, offsetof(core::geometry::vertex::model, texcoord), 2, opengl::constants::float_type });

    auto [axis_vertices, axis_elements] = core::geometry::Gizmo::axis({ 10.0f, 10.0f, 10.0f });

    opengl::Buffer axis_vbo;
    axis_vbo.create();
    axis_vbo.storage(core::as_bytes(axis_vertices), opengl::constants::static_draw);

    opengl::Buffer axis_ebo;
    axis_ebo.create();
    axis_ebo.storage(core::as_bytes(axis_elements), opengl::constants::static_draw);

    opengl::VertexArray axis_vao;
    axis_vao.create();
    axis_vao.attach(axis_vbo, sizeof(core::geometry::vertex::basic));
    axis_vao.attach(axis_ebo);
    axis_vao.attach({ 0, offsetof(core::geometry::vertex::basic, position), 3, opengl::constants::float_type });
    axis_vao.attach({ 1, offsetof(core::geometry::vertex::basic,    extra), 3, opengl::constants::float_type });

    //auto [debug_vertices, debug_elements] = core::Primitives::create_bounding_sphere(32, 0.5f, { 1.0f, 1.0f, 1.0f });
    auto [debug_vertices, debug_elements] = core::geometry::Primitive::bounding_box({ 0.505f, 0.505f, 0.505f }, { 1.0f, 0.0f, 1.0f });

    opengl::Buffer debug_vbo;
    debug_vbo.create();
    debug_vbo.storage(core::as_bytes(debug_vertices), opengl::constants::static_draw);

    opengl::Buffer debug_ebo;
    debug_ebo.create();
    debug_ebo.storage(core::as_bytes(debug_elements), opengl::constants::static_draw);

    opengl::VertexArray debug_vao;
    debug_vao.create();
    debug_vao.attach(debug_vbo, sizeof(core::geometry::vertex::basic));
    debug_vao.attach(debug_ebo);
    debug_vao.attach({ 0, offsetof(core::geometry::vertex::basic, position), 3, opengl::constants::float_type });
    debug_vao.attach({ 1, offsetof(core::geometry::vertex::basic,    extra), 3, opengl::constants::float_type });

    opengl::Sampler base_sampler;
    base_sampler.create();
    base_sampler.parameter(opengl::constants::min_filter, opengl::constants::nearest);
    base_sampler.parameter(opengl::constants::mag_filter, opengl::constants::nearest);

    const auto logo_image = images::TgaImage::load("logo.tga");

    opengl::Texture logo_texture { opengl::constants::texture_2d };
    logo_texture.create();
    logo_texture.storage(logo_image.width, logo_image.height, opengl::constants::rgba8, 1);
    logo_texture.upload(logo_image.width, logo_image.height, opengl::constants::rgba, 0, opengl::constants::unsigned_byte, logo_image.pixels);

    auto [logo_vertices, logo_elements] = core::geometry::Sprite::create(logo_image.width, logo_image.height);

    opengl::Buffer logo_vbo;
    logo_vbo.create();
    logo_vbo.storage(core::as_bytes(logo_vertices), opengl::constants::static_draw);

    opengl::Buffer logo_ebo;
    logo_ebo.create();
    logo_ebo.storage(core::as_bytes(logo_elements), opengl::constants::static_draw);

    opengl::VertexArray logo_vao;
    logo_vao.create();
    logo_vao.attach(logo_vbo, sizeof(core::geometry::vertex::sprite));
    logo_vao.attach(logo_ebo);
    logo_vao.attach({ 0, offsetof(core::geometry::vertex::sprite, position), 2, opengl::constants::float_type });
    logo_vao.attach({ 1, offsetof(core::geometry::vertex::sprite, texcoord), 2, opengl::constants::float_type });

    auto base_image = images::TgaImage::load("base_cube_albedo.tga");

    opengl::Texture base_texture { opengl::constants::texture_2d };
    base_texture.create();
    base_texture.storage(base_image.width, base_image.height, opengl::constants::rgb8, 1);
    base_texture.upload(base_image.width, base_image.height, opengl::constants::rgb, 0, opengl::constants::unsigned_byte, base_image.pixels);

    opengl::Texture game_view_texture { opengl::constants::texture_2d };
    game_view_texture.create();
    game_view_texture.storage(view_width, view_height, opengl::constants::rgb8, 1);

    opengl::Renderbuffer game_view_rbo;
    game_view_rbo.create();
    game_view_rbo.storage(view_width, view_height, opengl::constants::depth24);

    opengl::Framebuffer game_view_fbo;
    game_view_fbo.create();
    game_view_fbo.attach(game_view_texture, opengl::constants::color_attachment0, 0);
    game_view_fbo.attach(game_view_rbo, opengl::constants::depth_attachment);

    assert(game_view_fbo.complete());

    math::quat x_view_rotation;
    math::quat y_view_rotation;

    x_view_rotation.rotation({ 1.0f, 0.0f, 0.0f }, math::radians(-45.0f));
    y_view_rotation.rotation({ 0.0f, 1.0f, 0.0f }, math::radians(-45.0f));

    math::vec3 camera_position { 0.0f, 0.0f, 5.0f };

    //auto view_matrix = (x_view_rotation * y_view_rotation).matrix();
    auto view_matrix = static_cast<math::mat4>(x_view_rotation);
         view_matrix.translate(camera_position);

    core::data::camera camera_data; // TODO rename this with scene_ or base_ or even game_
    camera_data.view = inverse_rigid(view_matrix);
    camera_data.projection.perspective(math::radians(45.0f), view_width / view_height, 0.1f, 100.0f);

    core::data::camera view_camera_data;
    view_camera_data.projection.ortho(0.0f, static_cast<float>(window_width), static_cast<float>(window_height), 0.0f);

    constexpr core::data::light light_data
    {
        { 1.0f,  1.0f, 1.0f }, 0.35f,
        { 1.0f, -1.0f, 0.0f }
    };

    opengl::Buffer light_ubo;
    light_ubo.create();
    light_ubo.storage(sizeof(core::data::light), opengl::constants::dynamic_draw);
    light_ubo.bind(opengl::constants::uniform_buffer, core::as_base(core::binding::buffer::light));

    opengl::Buffer camera_ubo;
    camera_ubo.create();
    camera_ubo.storage(sizeof(core::data::camera), opengl::constants::dynamic_draw);
    camera_ubo.bind(opengl::constants::uniform_buffer, core::as_base(core::binding::buffer::camera));

    opengl::Buffer transform_ubo;
    transform_ubo.create();
    transform_ubo.storage(sizeof(core::data::transform), opengl::constants::dynamic_draw);
    transform_ubo.bind(opengl::constants::uniform_buffer, core::as_base(core::binding::buffer::transform));

    opengl::Buffer material_ubo;
    material_ubo.create();
    material_ubo.storage(sizeof(core::data::material), opengl::constants::dynamic_draw);
    material_ubo.bind(opengl::constants::uniform_buffer, core::as_base(core::binding::buffer::material));

    math::mat4 ground_transform { 1.0f };

    core::Object object;

    constexpr math::aabb object_aabb
    {
        { -0.25f, -0.25f, -0.25f },
        {  0.25f,  0.25f,  0.25f }
    };

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

    int32_t mouse_x { };
    int32_t mouse_y { };

    window_manager.input().callbacks.on_mouse_motion = [&] (const int32_t x, const int32_t y) noexcept
    {
        mouse_x = x;
        mouse_y = y;
    };

    constexpr float color[] { 0.2745f, 0.5176f, 0.1961f, 1.0f };

    opengl::Pipeline::enable(opengl::constants::blend_mode);
    opengl::Pipeline::enable(opengl::constants::multisample); // TODO this also needs to be enabled per framebuffer? or we need it just in the game_view?
    opengl::Pipeline::enable(opengl::constants::cull_test);   // TODO this also needs to be enabled per framebuffer? or we need it just in the game_view?

    opengl::Pipeline::blend(opengl::constants::src_alpha, opengl::constants::one_minus_src_alpha);

    core::Time time;
    time.start();

    while (window_active)
    {
         time.tick();

       window_manager.update();
        input_manager.update();

        object.update();

        auto point = math::vec2 { static_cast<float>(mouse_x), static_cast<float>(mouse_y) };

        if (const auto ray = ray_to_world(point, window_width, window_height, camera_data); intersects(ray, object_aabb))
        {
            // TODO do something
        }

        game_view_fbo.bind();

        opengl::Commands::viewport(0, 0, static_cast<int32_t>(view_width), static_cast<int32_t>(view_height));

        game_view_fbo.clear(color, 0);
        game_view_fbo.clear(1.0f);

        opengl::Pipeline::enable(opengl::constants::depth_test);

        model_shader.bind();

         light_ubo.upload(core::as_bytes(light_data), offsetof(core::data::light, color));

        camera_ubo.upload(core::as_bytes(camera_data), offsetof(core::data::camera, view));

        base_sampler.bind(core::as_base(core::binding::texture::albedo));
        base_texture.bind(core::as_base(core::binding::texture::albedo));

        cube_vao.bind();

        transform_ubo.upload(core::as_bytes(object.matrix()), offsetof(core::data::transform, model));

        opengl::Commands::draw_elements(opengl::constants::triangles, 0, cube_elements.size());

        ground_vao.bind();

        transform_ubo.upload(core::as_bytes(ground_transform), offsetof(core::data::transform, model));

        opengl::Commands::draw_elements(opengl::constants::triangles, 0, ground_elements.size());

        debug_shader.bind();

        //axis_vao.bind();

        //opengl::Commands::draw_elements(opengl::constants::lines, 0, axis_elements.size());

        debug_vao.bind();

        opengl::Commands::draw_elements(opengl::constants::lines, 0, debug_elements.size());

        opengl::Framebuffer default_fbo;
                            default_fbo.bind();

        opengl::Commands::viewport(0, 0, window_width, window_height);
        opengl::Commands::clear(opengl::constants::color_buffer);

        opengl::Pipeline::disable(opengl::constants::depth_test); // TODO maybe this is not necesary anymore?

        sprite_shader.bind();

        game_view_texture.bind(core::as_base(core::binding::texture::albedo));

        camera_ubo.upload(core::as_bytes(view_camera_data), offsetof(core::data::camera, view));

        math::mat4 game_view_matrix { 1.0f };

        transform_ubo.upload(core::as_bytes(game_view_matrix), offsetof(core::data::transform, model));

        view_vao.bind();

        opengl::Commands::draw_elements(opengl::constants::triangles, 0, view_elements.size() * core::geometry::element::triangle::size);

        logo_texture.bind(core::as_base(core::binding::texture::albedo));

        math::mat4 logo_transform { 0.5f };

        transform_ubo.upload(core::as_bytes(logo_transform), offsetof(core::data::transform, model));

        logo_vao.bind();

        opengl::Commands::draw_elements(opengl::constants::triangles, 0, logo_elements.size() * core::geometry::element::triangle::size);

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