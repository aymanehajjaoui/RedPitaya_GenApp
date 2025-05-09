/*ExportToRedPitayaHandler.hpp*/

#pragma once

#include <gtkmm.h>
#include <atomic>
#include <string>
#include <vector>
#include "Utility/DetailsPanel.hpp"

namespace ExportToRedPitayaHandler
{
    void handle(Gtk::Window* parentWindow,
                Gtk::Button& buttonExportToRedPitaya,
                Gtk::Button& buttonConnectRedPitaya,
                Gtk::Button& cancelExportButton,
                const std::string& modelFolder,
                const std::string& redpitayaHost,
                const std::string& redpitayaPassword,
                const std::string& redpitayaPrivateKeyPath,
                std::atomic<bool>& cancelExportFlag,
                DetailsPanel& detailsPanel);
}
