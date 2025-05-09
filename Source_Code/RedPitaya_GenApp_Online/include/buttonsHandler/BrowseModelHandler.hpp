/*BrowseModelHandler.hpp*/

#pragma once

#include <gtkmm.h>
#include <string>
#include <atomic>
#include <iostream>
#include "Utility/DetailsPanel.hpp"
#include "Utility/FileManager.hpp"

namespace BrowseModelHandler
{
    void handle(Gtk::Window* parentWindow,
                Gtk::Button& buttonBrowseModel,
                Gtk::Button& buttonExportLocally,
                Gtk::Button& buttonExportToRedPitaya,
                DetailsPanel& detailsPanel,
                std::string& modelFolder,
                bool& modelLoaded,
                bool redpitayaConnected);
}
