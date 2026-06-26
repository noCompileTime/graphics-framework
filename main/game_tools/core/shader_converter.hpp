#pragma once

namespace core
{
    class ShaderConverter // TODO remove the source, have just a header?
    {
    public:
        static auto convert_each(const std::filesystem::path& input, const std::filesystem::path& output, int32_t seconds) -> void;
        static auto convert_file(const std::filesystem::path& input, const std::filesystem::path& output, int32_t seconds) -> void;

        ShaderConverter() = delete;
    };
}