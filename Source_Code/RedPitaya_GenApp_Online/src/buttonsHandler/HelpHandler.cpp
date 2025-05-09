/*HelpHandler.cpp*/

#include "buttonsHandler/HelpHandler.hpp"

namespace HelpHandler
{
    void handle(Gtk::Window* parentWindow)
    {
        auto dialog = new Gtk::Dialog("Help & Dependencies", false);
        dialog->set_resizable(false);
        dialog->set_position(Gtk::WIN_POS_CENTER);

        dialog->set_transient_for(*parentWindow);
        dialog->set_modal(false);
        dialog->add_button("OK", Gtk::RESPONSE_OK);

        auto label = Gtk::make_managed<Gtk::Label>(
            "To export to Red Pitaya, ensure you have the required dependencies installed:\n\n"
            "For Ubuntu/Debian:\n"
            "    sudo apt update\n"
            "    sudo apt install sshpass\n\n"
            "For Arch Linux:\n"
            "    sudo pacman -S sshpass\n\n"
            "For macOS (Homebrew):\n"
            "    brew install hudochenkov/sshpass/sshpass\n\n"
            "Once installed, restart the program and retry the export."
        );
        label->set_line_wrap(true);
        label->set_margin_top(10);
        label->set_margin_bottom(10);
        label->set_margin_start(10);
        label->set_margin_end(10);

        dialog->get_content_area()->pack_start(*label, Gtk::PACK_EXPAND_WIDGET);
        dialog->show_all();

        dialog->signal_response().connect([dialog](int response)
        {
            if (response == Gtk::RESPONSE_OK) {
                dialog->hide();
                delete dialog;
            }
        });
    }
}
