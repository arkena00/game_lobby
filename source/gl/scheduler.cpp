#include <gl/scheduler.hpp>

#include <gl/core.hpp>
#include <gl/lobby.hpp>
#include <gl/utility.hpp>
#include <dpp/appcommand.h>

namespace gl
{
    scheduler::scheduler(gl::core& core) : core_{ core } {}

    void scheduler::add(const gl::lobby* lobby, const dpp::interaction& source_command, int duration, bool notify_primary)
    {
        auto lobby_time_point = lobby->settings.begin_time;
        auto notification_time = lobby_time_point - std::chrono::minutes{ duration };

         if (auto* player = lobby->player(source_command.member.user_id))
         {
            std::lock_guard<std::mutex> lock{ mutex };

            player->notify_time = notification_time;
            player->notify_primary = notify_primary;
         }
    }

    void scheduler::run()
    {
        while (true)
        {
            for (int i = 0; i < core_.lobbies().size(); ++i)
            {
                auto& lobby = core_.lobbies()[i];
                auto gmt_now = gl::gmt_time(std::chrono::system_clock::now(), lobby->settings.gmt);

                // delete old lobby
                if (lobby->has_expired())
                {
                    core_.del_lobby(lobby->id());
                    --i;
                    continue;
                }

                for (const auto& player : lobby->players())
                {
                    if (player->notify_time == std::chrono::system_clock::time_point{}) continue;

                    auto notify_time = gl::gmt_time(player->notify_time, lobby->settings.gmt);

                    if (gmt_now >= notify_time)
                    {
                        auto utc_time = std::chrono::duration_cast<std::chrono::seconds>(lobby->settings.begin_time.time_since_epoch()).count();

                        dpp::message message;
                        std::string gid = std::to_string(lobby->view_message().guild_id);
                        std::string cid = std::to_string(lobby->view_message().channel_id);
                        std::string mid = std::to_string(lobby->view_message().id);
                        std::string message_url = "https://discord.com/channels/" + gid + "/" + cid + "/" + mid;

                        message.content = "The lobby ** " + message_url + " ** will start soon **<t:" + std::to_string(utc_time) + ":R>**";
                        core_.notify(player->id, message);

                        std::lock_guard<std::mutex> lock{ mutex };
                        player->notify_time = {};
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }
} // gl