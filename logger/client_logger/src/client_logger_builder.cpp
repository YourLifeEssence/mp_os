#include <not_implemented.h>

#include "../include/client_logger_builder.h"

#include <client_logger.h>
#include "windows.h"


client_logger_builder::client_logger_builder() {}

client_logger_builder::client_logger_builder(
    const client_logger_builder& other) :
    files(other.files),
    outputFormat(other.outputFormat) {}

client_logger_builder& client_logger_builder::operator=(
    const client_logger_builder& other)
{
    if (this != &other)
    {
        files.clear();
        files = other.files;
        outputFormat = other.outputFormat;
    }
    return *this;
}

client_logger_builder::client_logger_builder(
    client_logger_builder&& other) noexcept :
    files(std::move(other.files)),
    outputFormat(std::move(other.outputFormat)) {}

client_logger_builder& client_logger_builder::operator=(
    client_logger_builder&& other) noexcept
{
    if (this != &other) 
    {
        files.clear();
        files = std::move(other.files);
        outputFormat = std::move(other.outputFormat);
    }
    return *this;
}

client_logger_builder::~client_logger_builder() noexcept
{
    files.clear();
}

logger_builder* client_logger_builder::add_output_format(const std::string& format) 
{
    if (format.empty()) 
        throw std::logic_error("Format cant be empty");

    outputFormat = format;
    return this;
}

logger_builder* client_logger_builder::add_file_stream(
    std::string const& stream_file_path,
    logger::severity severity)
{
    if (stream_file_path.empty()) 
        throw std::logic_error("Path cant be empty");

    TCHAR lpBuffer[512];
    TCHAR** lpFilePart = nullptr;

    GetFullPathName(stream_file_path.c_str(), 512, lpBuffer, lpFilePart);

    std::string absoluteFilePath(lpBuffer);
    std::map<std::string, unsigned char>::iterator i;
    i = files.find(absoluteFilePath);

    if (i != files.end()) 
    {
        if (((i->second >> static_cast<int>(severity)) & 1)) 
            return this;

        i->second ^= (1 << static_cast<int>(severity));
        return this;
    }
    files.emplace(absoluteFilePath, 1 << static_cast<int>(severity));
    return this;
}

logger_builder* client_logger_builder::add_console_stream(
    logger::severity severity)
{
    std::map<std::string, unsigned char>::iterator i;
    i = files.find("cerr");
    if (i != files.end()) 
    {
        if (((i->second >> static_cast<int>(severity)) & 1)) return this;
        i->second ^= (1 << static_cast<int>(severity));
        return this;
    }
    files.emplace("cerr", (1 << static_cast<int>(severity)));
    return this;
}

logger_builder* client_logger_builder::transform_with_configuration(
    const std::string& configuration_file_path,
    const std::string& configuration_path)
{
    throw not_implemented("logger_builder* client_logger_builder::transform_with_configuration(std::string const &configuration_file_path, std::string const &configuration_path)", "your code should be here...");
}

logger_builder* client_logger_builder::clear()
{
    files.clear();
    return this;
}

logger* client_logger_builder::build() const
{
    return new client_logger(files, outputFormat);
}