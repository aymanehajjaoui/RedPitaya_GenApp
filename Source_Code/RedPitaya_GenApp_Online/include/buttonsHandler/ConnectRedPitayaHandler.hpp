/*ConnectRedPitayaHandler.hpp*/

#pragma once

#include <gtkmm.h>
#include <string>
#include <atomic>
#include <filesystem>
#include <cstdlib>
#include "Utility/DetailsPanel.hpp"

namespace ConnectRedPitayaHandler
{
    void handle(Gtk::Window* parentWindow,
                Gtk::Button& buttonConnectRedPitaya,
                Gtk::Button& buttonExportToRedPitaya,
                DetailsPanel& detailsPanel,
                std::string& redpitayaHost,
                std::string& redpitayaPassword,
                std::string& redpitayaPrivateKeyPath,
                bool& redpitayaConnected,
                bool modelLoaded);
}
