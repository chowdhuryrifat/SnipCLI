#pragma once

#include <cstdlib>
#include <optional>
#include <string>

namespace snipcli::test {

/**
 * @brief RAII helper to temporarily set or unset an environment variable during tests,
 * restoring the prior environment state upon destruction.
 */
class ScopedEnv {
public:
    ScopedEnv(std::string name, std::optional<std::string> newValue) : m_name(std::move(name)) {
        const char* current = std::getenv(m_name.c_str());
        if (current != nullptr) {
            m_originalValue = std::string(current);
        }
        apply(newValue);
    }

    ~ScopedEnv() { apply(m_originalValue); }

    ScopedEnv(const ScopedEnv&) = delete;
    ScopedEnv& operator=(const ScopedEnv&) = delete;
    ScopedEnv(ScopedEnv&&) = delete;
    ScopedEnv& operator=(ScopedEnv&&) = delete;

private:
    void apply(const std::optional<std::string>& val) {
        if (val.has_value()) {
#if defined(_WIN32)
            _putenv_s(m_name.c_str(), val->c_str());
#else
            setenv(m_name.c_str(), val->c_str(), 1);
#endif
        } else {
#if defined(_WIN32)
            _putenv_s(m_name.c_str(), "");
#else
            unsetenv(m_name.c_str());
#endif
        }
    }

    std::string m_name;
    std::optional<std::string> m_originalValue;
};

}  // namespace snipcli::test
