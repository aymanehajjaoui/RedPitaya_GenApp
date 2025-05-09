/*ExportLocalHandler.hpp*/

#pragma once

#include <gtkmm.h>
#include <atomic>
#include <string>
#include "Utility/DetailsPanel.hpp"

namespace ExportLocalHandler
{
    void handle(Gtk::Window* parentWindow,
        Gtk::Button& buttonExportLocally,
        Gtk::Button& cancelExportButton,
        const std::string& modelFolder,
        std::atomic<bool>& cancelExportFlag,
        DetailsPanel& detailsPanel);
}
