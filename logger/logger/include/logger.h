#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_LOGGER_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_LOGGER_H

#include <iostream>

class logger
{

public:

    enum class severity
    {
        trace,
        debug,
        information,
        warning,
        error,
        critical
    };

public:

    virtual ~logger() noexcept = default;

public:

    virtual const logger *log(
        std::string const &message,
        logger::severity severity) const noexcept = 0;

public:

    const logger *trace(
        const std::string &message) const noexcept;

    const logger* debug(
        const std::string& message) const noexcept;

    const logger* information(
        const std::string& message) const noexcept;

    const logger* warning(
        const std::string& message) const noexcept;

    const logger* error(
        const std::string& message) const noexcept;

    const logger* critical(
        const std::string& message) const noexcept;


protected:

    static std::string severity_to_string(
        logger::severity severity);

    static std::string current_datetime_to_string() noexcept;

};

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_LOGGER_H