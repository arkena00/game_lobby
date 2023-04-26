#include <gl/lobby.hpp>

#include <dpp/cluster.h>
#include <dpp/colors.h>
#include <utility>

namespace gl
{
    lobby::lobby(dpp::cluster& bot, dpp::slashcommand_t source_command)
        : bot_{ bot }
        , source_command_{ std::move(source_command) }
        , id_{ ++lobby_id }
    {
        build_make_message();
    }

    void lobby::build_make_message()
    {
        make_message_ = dpp::message();
        make_message_.channel_id = source_command_.command.channel_id;
        make_message_.content = "**Make a lobby**";

        make_message_.add_component(
            dpp::component().
                        add_component(
                        dpp::component().set_label("Max slots: " + std::to_string(max_slots_)).
                        set_style(dpp::cos_primary).
                        set_id(make_id("test"))
                    )
        );

        // visibility
        make_message_.add_component(
                dpp::component().add_component(
                    dpp::component().set_type(dpp::cot_selectmenu).
                    set_label("Visibility").
                    set_placeholder("Visibility").
                    add_select_option(dpp::select_option("Public", "public", "Players can join the lobby").set_emoji("😄")).
                    add_select_option(dpp::select_option("Private", "private", "Players are manually added").set_emoji("🙂")).
                    set_default_value("private").
                    set_id(make_id(lobby_commands::visibility))
                )
        );

        // begin time
        make_message_.add_component(
                dpp::component().add_component(
                    dpp::component().set_type(dpp::cot_selectmenu).
                    set_placeholder("Begin time").
                    add_select_option(dpp::select_option("17H","value1","description1").set_emoji("😄")).
                    add_select_option(dpp::select_option("21H","value2","description2").set_emoji("🙂")).
                    add_select_option(dpp::select_option("Custom...", "custom", "").set_emoji("🙂")).
                    set_id(make_id(lobby_commands::begin_time))
                )
        );

        make_message_.add_component(
                dpp::component().add_component(
                    dpp::component().set_type(dpp::cot_user_selectmenu).
                    set_placeholder("Add player").
                    set_max_values(25).
                    set_id(make_id(lobby_commands::players))
                )
        );

        make_message_.add_component(
                dpp::component().add_component(
                    dpp::component().set_type(dpp::cot_role_selectmenu).
                    set_placeholder("Roles to ping").
                    set_max_values(25).
                    set_id(make_id(lobby_commands::pinged_roles))
                )
        );

        make_message_.add_component(
            dpp::component().
                     add_component(dpp::component().set_label("Make").
                        set_style(dpp::cos_success).
                        set_id(make_id(lobby_commands::make))
                    ).add_component(
                        dpp::component().set_label("Cancel").
                        set_style(dpp::cos_danger).
                        set_id(make_id(lobby_commands::cancel))
                    ).add_component(
                        dpp::component().set_label("Max slots: " + std::to_string(max_slots_)).
                        set_style(dpp::cos_primary).
                        set_id(make_id("max_slots"))
                    )
        );


    }
    
    void lobby::build_view_message()
    {
        auto mid = view_message_.id;
        view_message_ = dpp::message{};
        view_message_.id = mid;
        view_message_.channel_id = source_command_.command.channel_id;

        std::string description = R"(
**Host:** <@354033167407120394>
**Begin Time:** 21H00
**Duration:** 21H00
**Pinged:** <@&1090579377328246844>
)";

        std::string primary_players;
        std::string secondary_players;
        int i = 0;
        for (const auto& player : players_)
        {
            ++i;
            if (player.availability == player_availability::primary) primary_players += std::to_string(i) + ". " + player.str() + "\n";
            else secondary_players += std::to_string(i) + ". " + player.str() + "\n";
        }

        dpp::embed embed = dpp::embed().
        set_color(dpp::colors::wrx_blue).
        set_title("Among Us Lobby").
        set_author("GameLobby", "https://github.com/arkena00/", "https://avatars.githubusercontent.com/u/4370057?v=4").
        set_description(description).
        set_thumbnail("https://dpp.dev/DPP-Logo.png").
        add_field(
               ":white_check_mark: Primary",
               primary_players,
               true
        ).
        add_field(
                "Secondary",
                secondary_players,
                true
        ).
        set_footer(dpp::embed_footer().set_text("GameLobby v0.1.0\nJoin as secondary if you are not sure to be present").set_icon("https://dpp.dev/DPP-Logo.png")).
        set_timestamp(time(0));

        view_message_.add_embed(embed);

        view_message_.add_component(
            dpp::component().
            add_component(dpp::component().set_label("Join").set_emoji(":amongus:", 1085702291404902483, true).
                set_style(dpp::cos_success).
                set_id(make_id("join"))).
            add_component(dpp::component().set_label("Join as secondary").
                set_style(dpp::cos_primary).
                set_id(make_id("join_secondary"))).
            add_component(
                dpp::component().set_label("Leave").
                set_style(dpp::cos_danger).
                set_id(make_id("leave")))
         );

        view_message_.add_component(
            dpp::component().add_component(
                dpp::component().set_label("Remind me").set_emoji("🕙").
                set_style(dpp::cos_secondary).
                set_id(make_id("remind"))
            )
        );
    }

    void lobby::refresh()
    {
        build_view_message();
        bot_.message_edit(view_message());
    }

    std::string lobby::make_id(const std::string& id)
    {
        return std::to_string(id_) + "|" + id;
    }

    void lobby::make()
    {
        build_view_message();
        bot_.message_create(view_message(), [this](const dpp::confirmation_callback_t& r) {
            if (!r.is_error())
            {
                auto& message = std::get<dpp::message>(r.value);
                view_message_.id = message.id;
            }
        });
    }

    void lobby::join(gl::player player)
    {
        if (players_.end() != std::find_if(players_.begin(), players_.end(), [&player](const auto& p) { return p.id == player.id; })) return;
        players_.emplace_back(player);
        refresh();
    }

    void lobby::leave(dpp::snowflake user_id)
    {
        players_.erase(std::remove_if(players_.begin(), players_.end(), [user_id](const auto& p) { return p.id == user_id; }), players_.end());
        refresh();
    }

    void lobby::set_max_slots(int n)
    {
        max_slots_ = n;
        build_make_message();
    }
    void lobby::set_visibility(lobby_visibility visibility) { visibility_ = visibility; }

    dpp::snowflake lobby::id() const { return id_; }
    dpp::snowflake lobby::guild_id() const { return source_command_.command.guild_id; }
    lobby_visibility lobby::visibility() const { return visibility_; }
} // gl