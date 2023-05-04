#pragma once

#include <dpp/snowflake.h>
#include <string>

namespace gl
{
    enum class player_priority { primary, secondary };
    struct player
    {
        dpp::snowflake id;
        player_priority priority = player_priority::primary;

        std::string str() const
        {
            return "<@" + std::to_string(id) + ">";
        }
    };
} // gl