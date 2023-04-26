#pragma once

#include <gl/lobby.hpp>
#include <dpp/cluster.h>

namespace gl
{
    class core
    {
    public:
        core();

        gl::lobby* lobby(dpp::snowflake guild_id, dpp::snowflake lobby_id);

        void run();

    protected:
        static std::tuple<uint64_t, std::string> parse_id(const std::string&);

    private:
        dpp::cluster bot_;

        std::vector<std::unique_ptr<gl::lobby>> lobbies_;
    };
} // gl