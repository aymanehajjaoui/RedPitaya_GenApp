/*DetailsPanel.cpp*/

#include "Utility/DetailsPanel.hpp"

DetailsPanel::DetailsPanel() : Gtk::Box(Gtk::ORIENTATION_VERTICAL)
{
    set_spacing(5);

    
    pack_start(separator, Gtk::PACK_SHRINK);

    
    logBuffer = Gtk::TextBuffer::create();
    logView.set_buffer(logBuffer);
    logView.set_editable(false);
    logView.set_wrap_mode(Gtk::WRAP_WORD);

    scrollWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    scrollWindow.set_size_request(-1, 100);
    scrollWindow.set_hexpand(true);
    scrollWindow.set_vexpand(true);
    scrollWindow.add(logView);
    pack_start(scrollWindow, Gtk::PACK_EXPAND_WIDGET);

    
    buttonClearLogs.set_label("Clear logs");
    buttonClearLogs.signal_clicked().connect(sigc::mem_fun(*this, &DetailsPanel::clear_log));
    pack_start(buttonClearLogs, Gtk::PACK_SHRINK);

    
    progressBar.set_show_text(true);
    progressBar.set_fraction(0.0);
    pack_start(progressBar, Gtk::PACK_SHRINK);

    hide_panel(); 
}

bool DetailsPanel::is_visible() const
{
    return get_visible();
}

void DetailsPanel::append_log(const std::string &text)
{
    logHistory.push_back(text);

    
    logBuffer->insert(logBuffer->end(), text + "\n");

    Glib::signal_idle().connect_once([this]()
                                     {
            auto iter = logBuffer->end();
            logView.scroll_to(iter); });
}

void DetailsPanel::clear_log()
{
    logHistory.clear();
    logBuffer->set_text("");
}

void DetailsPanel::set_status(const std::string &status)
{
    progressBar.set_text(status);
}

void DetailsPanel::set_progress(double fraction)
{
    progressBar.set_fraction(fraction);
}

void DetailsPanel::hide_panel()
{
    hide();
}

void DetailsPanel::show_panel()
{
    logBuffer->set_text(""); 

    for (const auto &line : logHistory)
        logBuffer->insert(logBuffer->end(), line + "\n");

    show_all();

    Glib::signal_idle().connect_once([this]()
                                     {
        auto iter = logBuffer->end();
        logView.scroll_to(iter); });
}
