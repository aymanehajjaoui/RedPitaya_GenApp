/*DetailsPanel.hpp*/

#pragma once

#include <gtkmm.h>
#include <string>
#include <vector>
#include <glibmm/main.h> 

class DetailsPanel : public Gtk::Box
{
public:
    DetailsPanel();
    bool is_visible() const;
    void append_log(const std::string &text);
    void clear_log();
    void set_status(const std::string &status);
    void set_progress(double fraction);
    void hide_panel();
    void show_panel();

private:
    Gtk::Separator separator;
    Gtk::ScrolledWindow scrollWindow;
    Gtk::TextView logView;
    Glib::RefPtr<Gtk::TextBuffer> logBuffer;
    Gtk::ProgressBar progressBar;
    Gtk::Button buttonClearLogs;
    std::vector<std::string> logHistory;
};

