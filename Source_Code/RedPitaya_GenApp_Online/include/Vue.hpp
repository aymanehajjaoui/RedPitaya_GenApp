/*Vue.hpp*/

#pragma once

#include <gtkmm/window.h>
#include <gtkmm/button.h>
#include <gtkmm/box.h>
#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>
#include <gtkmm/label.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/radiobutton.h>
#include <iostream>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

class Vue : public Gtk::Window
{
private:
    Gtk::Button buttonBrowseModel;
    Gtk::Button buttonExportLocally;
    Gtk::Button buttonConnectRedPitaya;
    Gtk::Button buttonExportToRedPitaya;
    Gtk::Button buttonHelp;
    Gtk::Button buttonQuit;

    Gtk::Box box;
    Gtk::Label createdLabel;

    std::string modelFolder;
    std::string redpitayaHost;
    std::string redpitayaPassword;

    bool modelLoaded = false;
    bool redpitayaConnected = false;

public:
    Vue();
    virtual ~Vue();

    void onButtonBrowseModel();
    void onExportLocallyClicked();
    void onConnectRedPitayaClicked();
    void onExportToRedPitayaClicked();
    void onHelpClicked();
    void onQuitClicked();
};
