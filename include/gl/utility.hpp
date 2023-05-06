#pragma once

#include <gl/lobby.hpp>
#include <string>
#include <iomanip>
#include <iostream>

namespace gl
{
    inline std::string to_string(const std::chrono::system_clock::time_point& time_point, const std::string& format = "%d/%m/%Y %H:%M")
    {
        if (time_point == std::chrono::system_clock::time_point{}) return "";
        return std::vformat("{:" + format + "}", std::make_format_args(time_point));
    }

    inline std::string to_string(lobby_access access)
    {
        switch (access)
        {
            case lobby_access::private_: return "private";
            case lobby_access::public_: return "public";
        }
    }

    inline auto to_time_point(const std::string& str_time, const std::string& format = "%d/%m/%Y %H:%M")
    {
        std::stringstream ss{ str_time };
        std::chrono::system_clock::time_point time_point;
        ss >> std::chrono::parse(format, time_point);
        return time_point;
    }

    inline auto gmt_time(const std::chrono::system_clock::time_point& time_point, int gmt = 0)
    {
        return time_point + std::chrono::hours{ gmt };
    }

    inline std::string make_id(lobby* lobby_ptr, const std::string& id)
    {
        return std::to_string(lobby_ptr->id()) + "|" + id;
    }

    inline void parse_slots(const std::string& str_slots, gl::lobby_settings& settings)
    {
        auto sep_pos = str_slots.find('-');
        if (sep_pos != std::string::npos)
        {
            settings.min_slots = std::stoi(str_slots.substr(0, sep_pos));
            settings.max_slots = std::stoi(str_slots.substr(sep_pos + 1));
        }
        else
        {
            int slots = std::stoi(str_slots);
            settings.min_slots = slots;
            settings.max_slots = slots;
        }
    }

    inline void parse_time(const std::string& str_time, gl::lobby_settings& settings)
    {
        auto sep_pos = str_time.find('-');

        if (sep_pos != std::string::npos)
        {
            //settings.begin_time = str_time.substr(0, sep_pos);
            //settings.end_time = str_time.substr(sep_pos + 1);
        }
        else
        {
            //settings.begin_time = str_time;
            //settings.end_time = "";
        }
    }
} // gl