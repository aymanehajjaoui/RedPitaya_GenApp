/*RSAKeyDialog.hpp*/

#pragma once

#include <gtkmm.h>

class RSAKeyDialog : public Gtk::Dialog
{
public:
    RSAKeyDialog(Gtk::Window &parent);
    std::string get_key_path() const;
    std::string get_key_size() const;
    std::string get_passphrase() const;

private:
    Gtk::Entry entryPath;
    Gtk::ComboBoxText comboSize;
    Gtk::Entry entryPassphrase;
};
