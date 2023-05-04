#pragma once

#include <gl/lobby.hpp>
#include <dpp/cluster.h>
#include <gl/database.hpp>

namespace gl
{
    class core
    {
    public:
        core(const std::string& token);

        gl::lobby* lobby(dpp::snowflake guild_id, dpp::snowflake lobby_id);

        void run();

        gl::database& database() { return database_; }
        dpp::cluster& bot() { return bot_; }

    protected:
        static std::tuple<uint64_t, std::string> parse_id(const std::string&);

    private:
        gl::database database_;
        dpp::cluster bot_;

        std::vector<std::unique_ptr<gl::lobby>> lobbies_;
    };
} // gl