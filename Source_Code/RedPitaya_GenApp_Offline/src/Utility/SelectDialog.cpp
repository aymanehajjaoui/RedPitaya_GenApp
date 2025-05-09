/*VersionSelectDialog.cpp*/

#include "Utility/SelectDialog.hpp"

SelectDialog::SelectDialog()
    : Gtk::Dialog("Select Versions to Export"),
      box(Gtk::ORIENTATION_VERTICAL),
      cb_threads_mutex("threads_mutex"),
      cb_threads_sem("threads_sem"),
      cb_process_mutex("process_mutex"),
      cb_process_sem("process_sem"),
      buttonVersionHelp("Need help about which version to choose?")
{
    set_resizable(true);
    set_default_size(400, 250);
    set_position(Gtk::WIN_POS_CENTER);
    set_modal(false);        
    set_deletable(true);     

    Gtk::Box *contentArea = get_content_area();
    box.set_spacing(5);
    box.set_border_width(10);

    box.pack_start(cb_threads_mutex, Gtk::PACK_SHRINK);
    box.pack_start(cb_threads_sem, Gtk::PACK_SHRINK);
    box.pack_start(cb_process_mutex, Gtk::PACK_SHRINK);
    box.pack_start(cb_process_sem, Gtk::PACK_SHRINK);

    buttonVersionHelp.signal_clicked().connect(sigc::mem_fun(*this, &SelectDialog::onVersionHelp));
    box.pack_start(buttonVersionHelp, Gtk::PACK_SHRINK);

    contentArea->pack_start(box);
    add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    add_button("_OK", Gtk::RESPONSE_OK);

    show_all_children();
}

std::vector<std::string> SelectDialog::get_selected_versions() const
{
    std::vector<std::string> selected;

    if (cb_threads_mutex.get_active())
        selected.push_back("threads_mutex");
    if (cb_threads_sem.get_active())
        selected.push_back("threads_sem");
    if (cb_process_mutex.get_active())
        selected.push_back("process_mutex");
    if (cb_process_sem.get_active())
        selected.push_back("process_sem");

    return selected;
}

void SelectDialog::onVersionHelp()
{
    Gtk::MessageDialog helpDialog(*this, "Explanation of Available Versions", false, Gtk::MESSAGE_INFO);
    helpDialog.set_secondary_text(
        "Each version corresponds to a different execution strategy:\n\n"
        "• threads_mutex — Uses multithreading with mutex-based synchronization.\n"
        "• threads_sem   — Uses multithreading with POSIX semaphores.\n"
        "• process_mutex — Uses multiple processes with shared memory and mutexes.\n"
        "• process_sem   — Uses multiple processes with shared memory and semaphores.\n\n"
        "Choose the version(s) that match your system setup or requirements.");
    helpDialog.run();
}
