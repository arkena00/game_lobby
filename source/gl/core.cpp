#include <gl/core.hpp>

#include <dpp/dpp.h>

namespace gl
{
    core::core()
        : bot_{ "MTEwMDMwNDQ2ODI0NDk3NTY3Nw.Gqwwxy.ptXZ6NHQXTh2sQea448QRnaYrnOVk_bLi0t9ZY", dpp::i_default_intents | dpp::i_message_content }
    {
        bot_.on_log(dpp::utility::cout_logger());

        bot_.on_ready([&](const dpp::ready_t & event) {
            if (dpp::run_once<struct register_bot__commands>()) {
                bot_.global_command_create(dpp::slashcommand("make", "Make a lobby", bot_.me.id));
            }
        });

        bot_.on_slashcommand([this](const dpp::slashcommand_t& event) {
            if (event.command.get_command_name() == "make")
            {
                lobbies_.emplace_back(std::make_unique<gl::lobby>(bot_, event));

                event.reply(lobbies_.back()->make_message());
            }
        });

        bot_.on_select_click([this](const dpp::select_click_t& event) {
            auto [lobby_id, command_id] = parse_id(event.custom_id);
            auto* lobby_ptr = lobby(event.command.guild_id, lobby_id);

            if (!lobby_ptr)
            {
                event.get_original_response([&event](const dpp::confirmation_callback_t& m){
                    const auto& message = std::get<dpp::message>(m.value);
                    event.reply("Lobby not found gid:" + std::to_string(event.command.guild_id) + " id" + std::to_string(message.id));
                });
                event.reply();
                return;
            }

            if (command_id == "visibility")
            {

                lobby_ptr->set_visibility(gl::lobby_visibility::private_);
                event.reply(dpp::interaction_response_type::ir_update_message, lobby_ptr->make_message());
            }
            else if (command_id == "pings" && !event.values.empty())
            {
                //lobby_message.content += "<@&" + event.values[0] + ">";
                //event.reply(dpp::interaction_response_type::ir_update_message, lobby_message);
            }
            else if (command_id == "begin_time")
            {
                dpp::interaction_modal_response modal("my_modal", "Please enter stuff");

                modal.add_component(
                        dpp::component().
                        set_label("Begin time").
                        set_id("begin_time").
                        set_type(dpp::cot_text).
                        set_placeholder("21H00").
                        set_min_length(5).
                        set_max_length(5).
                        set_text_style(dpp::text_short)
                    );
                event.dialog(modal);
            }
            else event.reply();
        });

        bot_.on_button_click([this](const dpp::button_click_t& event) {
            auto [lobby_id, command_id] = parse_id(event.custom_id);

            if (auto* lobby_ptr = lobby(event.command.guild_id, lobby_id))
            {
                if (command_id == lobby_commands::make) lobby_ptr->make();
                else if (command_id == lobby_commands::join) lobby_ptr->join(gl::player{ event.command.usr.id });
                else if (command_id == lobby_commands::join_secondary) lobby_ptr->join(gl::player{ event.command.usr.id , player_availability::secondary});
                else if (command_id == lobby_commands::leave) lobby_ptr->leave(event.command.usr.id);
                else if (command_id == lobby_commands::cancel) event.delete_original_response();
                else if (command_id == "max_slots")
                {
                    dpp::interaction_modal_response modal(event.custom_id, "");
                    modal.add_component(
                            dpp::component().
                            set_label("Max slots").
                            set_id("test").
                            set_type(dpp::cot_text).
                            set_placeholder("Max slots").
                            set_min_length(1).
                            set_max_length(3).
                            set_text_style(dpp::text_short)
                        );

                    event.dialog(modal);
                    return;
                }
                event.reply();
            }
        });

        bot_.on_form_submit([&](const dpp::form_submit_t& event) {

            auto [lobby_id, command_id] = parse_id(event.custom_id);

            std::cout << "\nid: " << lobby_id << " cid: " << command_id;

            if (auto* lobby_ptr = lobby(event.command.guild_id, lobby_id))
            {
                if (command_id == "max_slots")
                {
                    std::string v = std::get<std::string>(event.components[0].components[0].value);
                    lobby_ptr->set_max_slots(std::stoi(v));
                    event.reply(dpp::interaction_response_type::ir_update_message, lobby_ptr->make_message());
                }
            }
        });
    }
    
    void core::run()
    {
        bot_.start(dpp::st_wait);
    }

    gl::lobby* core::lobby(dpp::snowflake guild_id, dpp::snowflake lobby_id)
    {
        for (const auto& lobby : lobbies_)
        {
            if (lobby->guild_id() == guild_id && lobby->id() == lobby_id) return lobby.get();
        }

        return nullptr;
    }

    std::tuple<uint64_t, std::string> core::parse_id(const std::string& custom_id)
    {
        auto sep = custom_id.find('|');
        uint64_t lobby_id = std::stoull(custom_id.substr(0, sep));
        std::string id = custom_id.substr(sep + 1);
        return { lobby_id, id };
    }
} // gl