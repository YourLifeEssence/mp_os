#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_CLIENT_LOGGER_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_CLIENT_LOGGER_H

#include <logger.h>
#include "client_logger_builder.h"

#include <map>

class client_logger final:
    public logger
{
public:

    struct stream {
        std::ofstream* _str;
        size_t countLoggers;

        explicit stream(const std::string &);

        stream(const stream&) = delete;
        stream& operator = (const stream &) = delete;

        stream(stream &&) noexcept;
        stream& operator = (stream &&) noexcept;

        ~stream();

        std::ofstream& operator << (const std::string&) const;
    };
    
private:

    friend client_logger_builder;

private:

    static std::map<std::string, stream> _streams;
    std::map<std::string, unsigned char> _severities;
    std::string _outputFormat;

    explicit client_logger(const std::map<std::string, unsigned char>&, const std::string&);

    static void decrement_Stream(std::map<std::string, unsigned char>::iterator&);
    static std::string current_Date_toString() noexcept;
    static std::string current_Time_toString() noexcept;
    std::string get_OutputString(const std::string&, logger::severity) const;
    void check_Unique_File(const client_logger&);

public:

    client_logger(
        client_logger const &other);

    client_logger &operator=(
        client_logger const &other);

    client_logger(
        client_logger &&other) noexcept;

    client_logger &operator=(
        client_logger &&other) noexcept;

    ~client_logger() noexcept final;

public:

    [[nodiscard]] logger const *log(
        const std::string &message,
        logger::severity severity) const noexcept override;

};

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_CLIENT_LOGGER_H