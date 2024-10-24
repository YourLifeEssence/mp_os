#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_CLIENT_LOGGER_BUILDER_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_CLIENT_LOGGER_BUILDER_H

#include <logger_builder.h>
#include<map>
#include <vector>

class client_logger_builder final:
    public logger_builder
{
private:

    std::map<std::string, unsigned char> files;
    std::string outputFormat;

public:

    client_logger_builder();

    client_logger_builder(
        const client_logger_builder& other);

    client_logger_builder &operator=(
        const client_logger_builder& other);

    client_logger_builder(
        client_logger_builder &&other) noexcept;

    client_logger_builder &operator=(
        client_logger_builder &&other) noexcept;

    ~client_logger_builder() noexcept override;

public:

    logger_builder *add_file_stream(
        const std::string& stream_file_path,
        logger::severity severity) override;

    logger_builder *add_console_stream(
        logger::severity severity) override;

    logger_builder* transform_with_configuration(
        const std::string& configuration_file_path,
        const std::string& configuration_path) override;

    logger_builder* add_output_format(const std::string&);

    logger_builder *clear() override;

    [[nodiscard]] logger *build() const override;

};

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_CLIENT_LOGGER_BUILDER_H