/*Vue.hpp*/

#pragma once

#include <gtkmm.h>
#include <iostream>
#include <filesystem>
#include <vector>
#include <atomic>
#include <thread>
#include <stdexcept>
#include "buttonsHandler/BrowseModelHandler.hpp"
#include "buttonsHandler/ExportLocalHandler.hpp"
#include "buttonsHandler/ConnectRedPitayaHandler.hpp"
#include "buttonsHandler/ExportToRedPitayaHandler.hpp"
#include "buttonsHandler/HelpHandler.hpp"
#include "buttonsHandler/QuitHandler.hpp"
#include "Utility/DetailsPanel.hpp"

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
    std::string redpitayaHost;
    std::string redpitayaPassword;
    std::string redpitayaPrivateKeyPath;

    DetailsPanel detailsPanel;

    bool modelLoaded = false;
    bool redpitayaConnected = false;
    std::atomic<bool> cancelExportFlag = false;

public:
    Vue();
    virtual ~Vue();

    void onCheckShowDetailsClicked();
};
