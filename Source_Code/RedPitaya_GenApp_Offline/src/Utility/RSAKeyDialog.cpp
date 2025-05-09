/*RSAKeyDialog.cpp*/

#include "Utility/RSAKeyDialog.hpp"

RSAKeyDialog::RSAKeyDialog(Gtk::Window &parent)
    : Gtk::Dialog("Generate RSA Key", parent, true)
{
    set_default_size(400, 200);
    auto *box = get_content_area();

    auto labelPath = Gtk::make_managed<Gtk::Label>("Private Key Path:");
    box->pack_start(*labelPath, Gtk::PACK_SHRINK);
    box->pack_start(entryPath, Gtk::PACK_SHRINK);
    entryPath.set_placeholder_text("Path to private key (e.g. ~/.ssh/id_rsa_redpitaya)");

    auto labelSize = Gtk::make_managed<Gtk::Label>("Key Size:");
    comboSize.append("2048");
    comboSize.append("4096");
    comboSize.set_active_text("2048");
    box->pack_start(*labelSize, Gtk::PACK_SHRINK);
    box->pack_start(comboSize, Gtk::PACK_SHRINK);

    auto labelPass = Gtk::make_managed<Gtk::Label>("Passphrase (optional):");
    entryPassphrase.set_visibility(false);
    box->pack_start(*labelPass, Gtk::PACK_SHRINK);
    box->pack_start(entryPassphrase, Gtk::PACK_SHRINK);

    add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    add_button("_Generate", Gtk::RESPONSE_OK);
    show_all_children();
    if (auto okButton = get_widget_for_response(Gtk::RESPONSE_OK)) {
        okButton->grab_focus();}
}

std::string RSAKeyDialog::get_key_path() const { return entryPath.get_text(); }
std::string RSAKeyDialog::get_key_size() const { return comboSize.get_active_text(); }
std::string RSAKeyDialog::get_passphrase() const { return entryPassphrase.get_text(); }
