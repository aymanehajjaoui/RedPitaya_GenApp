/*QuitHandler.cpp*/

#include "buttonsHandler/QuitHandler.hpp"

namespace QuitHandler
{
    void handle(Gtk::Window* parentWindow)
    {
        parentWindow->hide();
    }
}
