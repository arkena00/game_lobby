#pragma once

#include <dpp/appcommand.h>
#include <dpp/snowflake.h>
#include <chrono>
#include <vector>

namespace gl
{
    class core;
    class lobby;

    class scheduler
    {
    public:
        explicit scheduler(gl::core& core);

        void add(const gl::lobby* lobby, const dpp::interaction& source_command, int duration, bool notify_primary = false);
        [[noreturn]] void run();

    private:
        gl::core& core_;

        static inline std::mutex mutex{};
    };
} // gl