#include "shader_converter.hpp"

namespace core
{
    auto ShaderConverter::convert_each(const std::filesystem::path& input, const std::filesystem::path& output, const int32_t seconds) -> void
    {
      assert(is_directory(input));

        if (!is_directory(output))
        {
            create_directory(output);
        }

        for (const auto& entry : std::filesystem::directory_iterator(input))
        {
            if (is_regular_file(entry))
            {
                convert_file(entry.path(), output, seconds);
            }
        }
    }

    auto ShaderConverter::convert_file(const std::filesystem::path& input, const std::filesystem::path& output, const int32_t seconds) -> void
    {
          assert(is_directory(output));

            const auto out  = output / input.filename();
        if (const auto ext  = input.extension().string(); ext == ".vert" || ext == ".frag") // TODO make a set for extensions?
        {
            const auto  now_time = std::filesystem::file_time_type::clock::now();
            const auto last_time = last_write_time(input);

            if (const auto age = std::chrono::duration_cast<std::chrono::seconds>(now_time - last_time).count();
                           age < seconds || seconds == -1)
            {
                const auto opt = "-Os"; // TODO someday try this with -O should give you better performance?
                const auto cmd = std::format("glslangvalidator -V -G {} -S {} -o {} {}", opt, ext.substr(1), out.string(), input.generic_string());
                const auto err = std::system(cmd.c_str());

                   assert(!err);
            }
        }
    }
}