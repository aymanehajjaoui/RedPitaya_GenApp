/*SelectDialog.hpp*/

#pragma once

#include <gtkmm.h>
#include <vector>
#include <string>

class SelectDialog : public Gtk::Dialog
{
public:
    SelectDialog();
    std::vector<std::string> get_selected_versions() const;

private:
    Gtk::Box box;

    Gtk::CheckButton cb_threads_mutex, cb_threads_sem, cb_process_mutex, cb_process_sem;

    Gtk::Button buttonVersionHelp;

    void onVersionHelp();
};
