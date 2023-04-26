#pragma once

#include <cstdint>
#include <gl/player.hpp>
#include <dpp/message.h>
#include <dpp/dispatcher.h>

namespace gl
{
    enum class lobby_visibility{ public_, private_ };

    namespace lobby_commands
    {
        static constexpr std::string make = "make";
        static constexpr std::string cancel = "cancel";

        static constexpr std::string join = "join";
        static constexpr std::string join_secondary = "join_secondary";
        static constexpr std::string leave = "leave";

        static constexpr std::string begin_time = "begin_time";
        static constexpr std::string visibility = "visibility";
        static constexpr std::string pinged_roles = "pinged_roles";
        static constexpr std::string players = "players";
    } // lobby_commands

    class lobby
    {
    public:
        explicit lobby(dpp::cluster&, dpp::slashcommand_t );

        void make();

        void join(gl::player);
        void leave(dpp::snowflake);

        void set_visibility(lobby_visibility visibility);
        void set_max_slots(int max_slots);

        dpp::snowflake id() const;
        dpp::snowflake guild_id() const;

        lobby_visibility visibility() const;

        const dpp::message& make_message() const { return make_message_; }
        const dpp::message& view_message() const { return view_message_; }

    protected:
        void build_make_message();
        void build_view_message();

        void refresh();
        std::string make_id(const std::string&);

    private:
        static inline uint64_t lobby_id = 0;

        dpp::cluster& bot_;
        dpp::slashcommand_t source_command_;
        dpp::message make_message_;
        dpp::message view_message_;

        uint64_t id_;
        int max_slots_ = 10;
        lobby_visibility visibility_;
        std::vector<gl::player> players_;
    };
} // gl