#include "core/file.hpp"
#include "core/input_manager.hpp"
#include "core/platform_factory.hpp"
#include "core/platform_functions.hpp"
#include "core/platform_time.hpp"
#include "core/window_manager.hpp"

#include "opengl/commands.hpp"
#include "opengl/shader.hpp"
#include "opengl/texture.hpp"
#include "opengl/vertex_array.hpp"

#include "opengl/functions.hpp"
#include "opengl/headers.hpp"

#include "tools/shaders_converter.hpp"

#include "images/tga_image.hpp"

auto main() -> int32_t
{
         tools::ShadersConverter::convert_each(BASE_SHADERS_PATH, "shaders");

                       const auto factory = core::PlatformFactory::create();

    core::PlatformFunctions::init(factory);

    const auto platform_monitor = factory->create_platform_monitor();
               platform_monitor->init();

    constexpr auto window_width  { 1280 };
    constexpr auto window_height {  720 };

    constexpr core::window::configuration window_configuration
    {
        "Graphics Framework",
         window_width,  //platform_monitor->width
         window_height  //platform_monitor->height
    };
                   auto window_active { true };
    core::WindowManager window_manager;
                        window_manager.init(factory, window_configuration);

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

    input_manager.input_actions().set_action(core::input::code::key_escape, [&window_active]
    {
        window_active = false;
    });

    opengl::Functions::init();

    opengl::ShaderStage base_shader_vert;
    base_shader_vert.type(opengl::constants::vertex_shader);
    base_shader_vert.create();
    base_shader_vert.source(core::File::read("shaders/base_sprite_shader.vert", std::ios::binary));

    opengl::ShaderStage base_shader_frag;
    base_shader_frag.type(opengl::constants::fragment_shader);
    base_shader_frag.create();
    base_shader_frag.source(core::File::read("shaders/base_sprite_shader.frag", std::ios::binary));

    opengl::Shader base_shader;
    base_shader.create();
    base_shader.attach(base_shader_vert);
    base_shader.attach(base_shader_frag);
    base_shader.link();

    std::vector<core::vertex::sprite> square_vertices
    {
        { { -0.5f, -0.5f }, { 0.0f, 0.0f } },
        { {  0.5f, -0.5f }, { 1.0f, 0.0f } },
        { {  0.5f,  0.5f }, { 1.0f, 1.0f } },
        { { -0.5f,  0.5f }, { 0.0f, 1.0f } }
    };

    std::vector<uint32_t> square_elements
    {
        2, 1, 0,
        0, 3, 2
    };

    opengl::Buffer square_vbo;
    square_vbo.create();
    square_vbo.storage(core::buffer::make_data(square_vertices), 0);

    opengl::Buffer square_ebo;
    square_ebo.create();
    square_ebo.storage(core::buffer::make_data(square_elements), 0);

    opengl::VertexArray square_vao;
    square_vao.create();
    square_vao.attach_vertices (square_vbo, sizeof(core::vertex::sprite));
    square_vao.attach_elements (square_ebo);

    square_vao.attach_attribute({ 0, 2, opengl::constants::float_type, offsetof(core::vertex::sprite, position.x) });
    square_vao.attach_attribute({ 1, 2, opengl::constants::float_type, offsetof(core::vertex::sprite, texcoord.x) });

    auto [width, height, pixels] = images::TgaImage::load("chess.tga");

    opengl::Texture square_texture;
    square_texture.type(opengl::constants::texture_2d);
    square_texture.create();
    square_texture.storage(width, height, opengl::constants::rgb8, 1);
    square_texture.update (width, height, opengl::constants::rgb,  0, pixels.data());

    math::mat4 transform;

    opengl::Buffer transform_ubo;
    transform_ubo.create();
    transform_ubo.bind_base(opengl::constants::uniform_buffer, core::buffer::transform);
    transform_ubo.storage(core::buffer::make_data(&transform), 0);

    auto aspect_ratio = static_cast<float>(window_width) /
                        static_cast<float>(window_height);
    math::mat4 view;
               view.translate({ 0.0f, 0.0f, -3.0f });

    math::mat4 projection;
             //projection.ortho(-aspect_ratio, aspect_ratio, -1.0f, 1.0f);
               projection.perspective(45.0f, aspect_ratio, 0.1f, 100.0f);

    std::vector camera_matrices
    {
        view,
        projection
    };

    opengl::Buffer camera_ubo;
    camera_ubo.create();
    camera_ubo.bind_base(opengl::constants::uniform_buffer, core::buffer::camera);
    camera_ubo.storage(core::buffer::make_data(camera_matrices), 0);

    opengl::Commands::clear(0.5f, 0.5f, 0.5f, 1.0f);

    core::PlatformTime platform_time;
                       platform_time.start();

    while (window_active)
    {
         platform_time.tick();

        window_manager.update();
         input_manager.update();

        opengl::Commands::clear(opengl::constants::color_buffer);

        base_shader.bind();

        square_texture.bind(0);

         square_vao.bind();

        opengl::Commands::draw_elements(opengl::constants::triangles, square_elements.size(), 0);

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