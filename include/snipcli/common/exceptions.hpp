#pragma once

#include <stdexcept>
#include <string>

namespace snipcli::common {

/**
 * @brief Base exception class for all SnipCLI runtime domain errors.
 */
class SnipException : public std::runtime_error {
public:
    explicit SnipException(const std::string& message) : std::runtime_error(message) {}
};

/**
 * @brief Exception thrown when snippet business rules or validations fail.
 */
class ValidationException : public SnipException {
public:
    explicit ValidationException(const std::string& message)
        : SnipException("Validation Error: " + message) {}
};

/**
 * @brief Exception thrown during file I/O or JSON serialization failures.
 */
class StorageException : public SnipException {
public:
    explicit StorageException(const std::string& message)
        : SnipException("Storage Error: " + message) {}
};

/**
 * @brief Exception thrown when CLI argument parsing or command dispatch fails.
 */
class CommandException : public SnipException {
public:
    explicit CommandException(const std::string& message)
        : SnipException("Command Error: " + message) {}
};

}  // namespace snipcli::common
