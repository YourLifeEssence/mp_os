#include <not_implemented.h>

#include "../include/client_logger.h"

#include <fstream>
#include <exception>>
#include <ios>
#include <ctime>
#include <iomanip>
#include <sstream>

std::map<std::string, client_logger::stream> client_logger::_streams = std::map<std::string, client_logger::stream>();


client_logger::stream::stream(const std::string& path) : _str(nullptr), countLoggers(1) 
{
    try 
    {
        _str = new std::ofstream;
        _str->open(path);
        if (!_str->is_open()) 
        {
            throw std::logic_error("Error open file");
        }
    }
    catch (const std::bad_alloc& e)
    {
        delete _str;
        _str = nullptr;
        std::cerr << e.what() << std::endl;
        throw;
    }
    catch (const std::exception& e)
    {
        delete _str;
        _str = nullptr;
        std::cerr << e.what() << std::endl;
        throw;
    }
}

client_logger::stream::stream(client_logger::stream&& other) noexcept : _str(other._str), countLoggers(other.countLoggers)
{
    other._str = nullptr;
    other.countLoggers = 0;
}

client_logger::stream& client_logger::stream::operator = (client_logger::stream&& other) noexcept
{
    if (this != &other)
    {
        _str->flush();
        _str->close();
        delete _str;
        _str = other._str;
        countLoggers = other.countLoggers;
        other._str = nullptr;
        other.countLoggers = 0;
    }
    return *this;
}

std::ofstream& client_logger::stream::operator << (const std::string& str) const
{
    if (_str) {
        *_str << str;
        _str->flush();
    }
    return *_str;
}

client_logger::stream::~stream()
{
    if (_str != nullptr)
    {
        _str->flush();
        _str->close();
        delete _str;
        _str = nullptr;
    }
    countLoggers = 0;
}

client_logger::client_logger(const std::map<std::string, unsigned char>& map, const std::string& outputFormat) :
    _severities(map),
    _outputFormat(outputFormat)
{
    if (map.empty())
    {
        throw std::logic_error("Map is empty");
    }

    if (outputFormat.empty())
    {
        _outputFormat = "[%t %d %s] %m";
        // throw std::logic_error("Format can't be empty");
    }

    auto iterPaths = map.begin();
    while (iterPaths != map.end())
    {
        if (iterPaths->first == "cerr")
        {
            ++iterPaths;
            continue;
        }
        auto iterStreams = _streams.find(iterPaths->first);
        if (iterStreams != _streams.end())
        {
            ++(iterStreams->second.countLoggers);
        }
        else
        {
            _streams.emplace(iterPaths->first, stream(iterPaths->first));
        }
        ++iterPaths;
    }
}

void client_logger::decrement_Stream(std::map<std::string, unsigned char>::iterator& iterPaths)
{
    auto iterStreams = _streams.find(iterPaths->first);
    if (iterStreams != _streams.end())
    {
        --(iterStreams->second.countLoggers);
        if (!(iterStreams->second.countLoggers))
        {
            iterStreams->second._str->close();
            delete iterStreams->second._str;
            iterStreams->second._str = nullptr;
            _streams.erase(iterStreams);
        }
    }
}

std::string client_logger::current_Date_toString() noexcept
{
    auto time = std::time(nullptr);
    std::ostringstream stream_str;
    stream_str << std::put_time(std::localtime(&time), "%H:%M:%S");
    return stream_str.str();
}

std::string client_logger::current_Time_toString() noexcept
{
    auto time = std::time(nullptr);
    std::ostringstream stream_str;
    stream_str << std::put_time(std::localtime(&time), "%d.%m.%Y");
    return stream_str.str();
}

std::string client_logger::get_OutputString(const std::string& message, logger::severity severity) const 
{
    std::string res;
    auto ptr = _outputFormat.begin();
    while (ptr != _outputFormat.end()) 
    {
        if (*ptr == '%')
        {
            ++ptr;
            if (ptr == _outputFormat.end()) 
            {
                throw std::logic_error("Format error!");
            }

            switch (*ptr) 
            {
            case 'd':
                res += current_Date_toString();
                break;
            case 't':
                res += current_Time_toString();
                break;
            case 's':
                res += logger::severity_to_string(severity);
                break;
            case 'm':
                res += message;
                break;
            default:
                throw std::logic_error("Unknown modificator");
                break;
            }
        }
        else res.push_back(*ptr);
        ++ptr;
    }
    return res;
}

void client_logger::check_Unique_File(const client_logger& other) {
    auto iterSeverity = _severities.begin();
    while (iterSeverity != _severities.end()) {
        if (other._severities.find(iterSeverity->first) == other._severities.end()) {
            decrement_Stream(iterSeverity);
        }
        ++iterSeverity;
    }
}


client_logger::client_logger(
    const client_logger &other) :
    _severities(other._severities)
{
    auto iterSeverities = _severities.begin();

    while (iterSeverities != _severities.end())
    {
        ++(_streams.find(iterSeverities->first)->second.countLoggers);
        ++iterSeverities;
    }
}
    
client_logger &client_logger::operator=(
    const client_logger &other) 
{
    if (this != &other)
    {
        check_Unique_File(other);
        _severities = other._severities;
        _outputFormat = other._outputFormat;
    }
    return *this;
}

client_logger::client_logger(
    client_logger &&other) noexcept :
    _severities(std::move(other._severities)),
    _outputFormat(std::move(other._outputFormat)) {}

client_logger &client_logger::operator=(
    client_logger &&other) noexcept
{
    if (this != &other) {
        check_Unique_File(other);
        _severities = std::move(other._severities);
        _outputFormat = std::move(other._outputFormat);
    }
    return *this;
}

client_logger::~client_logger() noexcept
{
    auto iterPaths = _severities.begin();
    while (iterPaths != _severities.end()) {
        decrement_Stream(iterPaths);
        ++iterPaths;
    }
}

const logger* client_logger::log(
    const std::string &text,
    logger::severity severity) const noexcept
{
    auto iterSev = _severities.begin();
    while (iterSev != _severities.end()) {
        if (((iterSev->second) >> static_cast<int>(severity)) & 1) {
            if (iterSev->first == "cerr") {
                std::cerr << get_OutputString(text, severity) << std::endl;
                ++iterSev;
                continue;
            }
            auto& target_stream = _streams.find(iterSev->first)->second;
            target_stream << get_OutputString(text, severity) << std::endl;
            target_stream._str->flush();
        }
        ++iterSev;
    }
    return this;
}