#pragma once

#include <dpp/appcommand.h>
#include <dpp/message.h>
#include <gl/lobby.hpp>
#include <gl/utility.hpp>

namespace ui
{
    auto make_game_options(gl::lobby* lobby_ptr, const std::string& modal_id)
    {
        dpp::interaction_modal_response modal(gl::make_id(lobby_ptr, gl::lobby_commands::game_options), "");

        auto text_field = [&modal, lobby_ptr](auto&& command, auto&& label, const std::string& default_value = "", int max_input = 32)
        {
            return dpp::component().
                set_id(command).
                set_label(label).
                set_type(dpp::cot_text).
                set_default_value(default_value).
                set_min_length(1).
                set_max_length(max_input).
                set_text_style(dpp::text_short);
        };
        modal.add_component( text_field(gl::lobby_commands::game, "Game", lobby_ptr->settings.game).set_required(true) );
        modal.add_row();
        modal.add_component( text_field(gl::lobby_commands::game_logo, "Game logo", lobby_ptr->settings.game_logo, 900)
                            .set_placeholder("ex: https://www.website.com/image.png") );
        modal.add_row();
        modal.add_component( text_field(gl::lobby_commands::game_mod, "Mod", lobby_ptr->settings.game_mod, 900)
                            .set_placeholder("ex: [ModName](https://www.modsite/mod)") );
        modal.add_row();
        modal.add_component( text_field(gl::lobby_commands::game_map, "Map", lobby_ptr->settings.map) );

        return modal;
    }

    auto make_lobby_options(gl::lobby* lobby_ptr, const std::string& modal_id)
    {
        dpp::interaction_modal_response modal(modal_id, "");

        auto text_field = [&modal, lobby_ptr](auto&& command, auto&& label, const std::string& default_value = "", int max_input = 24)
        {
            return dpp::component().
                set_id(command).
                set_label(label).
                set_type(dpp::cot_text).
                set_default_value(default_value).
                set_min_length(0).
                set_max_length(max_input).
                set_text_style(dpp::text_short);
        };

        std::string default_access = lobby_ptr->settings.access == gl::lobby_access::public_ ? "y" : "n";
        modal.add_component( text_field(gl::lobby_commands::access, "Public (y = public | n = private)", default_access, 1)
                            .set_placeholder("ex: y")
                            .set_required(true) );
        modal.add_row();
        modal.add_component( text_field(gl::lobby_commands::slots, "Slots", std::to_string(lobby_ptr->settings.min_slots) + "-" + std::to_string(lobby_ptr->settings.max_slots), 7)
                            .set_placeholder("ex: 2-10")
                            .set_required(true) );
        modal.add_row();

        std::string default_time = lobby_ptr->settings.begin_time + (lobby_ptr->settings.end_time.empty() ? "" : "-" + lobby_ptr->settings.end_time);
        modal.add_component( text_field(gl::lobby_commands::time, "Time", default_time)
                            .set_placeholder("ex: 21:00 or 21:00-23:00")
                            .set_required(true) );
        modal.add_row();
        modal.add_component( text_field(gl::lobby_commands::date, "Date (default: today)", lobby_ptr->settings.date)
                            .set_placeholder("ex: 24/01/23") );
        modal.add_row();
        modal.add_component( text_field(gl::lobby_commands::host, "Host ID", lobby_ptr->settings.host)
                            .set_placeholder("Right click on a user -> copy user id") );

        return modal;
    }
} // ui