#pragma once

#include <dpp/snowflake.h>
#include <chrono>
#include <string>

namespace gl
{
    enum class player_group { primary, secondary, waiting };
    struct player
    {
        dpp::snowflake id;
        player_group group = player_group::primary;
        std::chrono::utc_clock::time_point notify_time;
        bool notify_primary = false;

        std::string str() const
        {
            return "<@" + std::to_string(id) + ">";
        }
    };
} // gl