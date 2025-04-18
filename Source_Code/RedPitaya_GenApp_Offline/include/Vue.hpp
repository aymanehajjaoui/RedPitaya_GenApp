/*Vue.hpp*/

#pragma once

#include <gtkmm/window.h>
#include <gtkmm/button.h>
#include <gtkmm/box.h>
#include <gtkmm/entry.h>
#include <gtkmm/label.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/checkbutton.h>
#include <iostream>
#include <filesystem>
#include <vector>
#include <atomic>
#include <thread>
#include "DetailsPanel.hpp"

namespace fs = std::filesystem;

class Vue : public Gtk::Window
{
private:
    Gtk::Button buttonBrowseModel;
    Gtk::Button buttonExportLocally;
    Gtk::Button buttonConnectRedPitaya;
    Gtk::Button buttonExportToRedPitaya;
    Gtk::Button cancelExportButton;
    Gtk::Button buttonHelp;
    Gtk::Button buttonQuit;

    Gtk::Box mainBox;
    Gtk::Box buttonRowBox;
    Gtk::CheckButton checkShowDetails;

    std::string modelFolder;
    std::string genFilesDir = std::filesystem::absolute("gen_files").string();
    std::string redpitayaHost;
    std::string redpitayaPassword;

    DetailsPanel detailsPanel;

    bool modelLoaded = false;
    bool redpitayaConnected = false;
    std::atomic<bool> cancelExportFlag = false;

public:
    Vue();
    virtual ~Vue();

    void onButtonBrowseModel();
    void onExportLocallyClicked();
    void onConnectRedPitayaClicked();
    void handleConnectButton(Gtk::Dialog *dialog,
                             Gtk::RadioButton *radioMac,
                             Gtk::Entry *entryMac,
                             Gtk::Entry *entryIP,
                             Gtk::Entry *entryPassword);
    void showErrorDialog(Gtk::Widget *parent, const std::string &title, const std::string &text);
    void showInfoDialog(Gtk::Widget *parent, const std::string &message);
    void onExportToRedPitayaClicked();
    void onHelpClicked();
    void onQuitClicked();
    void onCheckShowDetailsClicked();
};
