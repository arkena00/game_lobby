#pragma once

#include <dpp/snowflake.h>
#include <string>

namespace gl
{
    enum class player_availability { primary, secondary };
    struct player
    {
        dpp::snowflake id;
        player_availability availability = player_availability::primary;

        std::string str() const
        {
            return "<@" + std::to_string(id) + ">";
        }
    };
} // gl