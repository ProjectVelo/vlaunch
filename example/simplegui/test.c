#include <gtk/gtk.h>

// Callback function for button click
static void on_button_clicked(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog;
    GtkWidget *parent_window = (GtkWidget*)data;
    
    dialog = gtk_message_dialog_new(GTK_WINDOW(parent_window),
                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_INFO,
                                   GTK_BUTTONS_OK,
                                   "Hello from your C GUI application!");
    gtk_window_set_title(GTK_WINDOW(dialog), "Message");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Callback function for window close
static void on_window_destroy(GtkWidget *widget, gpointer data) {
    gtk_main_quit();
}

int main(int argc, char *argv[]) {
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *label;
    GtkWidget *button;
    GtkWidget *exit_button;
    
    // Initialize GTK
    gtk_init(&argc, &argv);
    
    // Create main window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Hello World GUI - C");
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 200);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    
    // Connect window destroy signal
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);
    
    // Create vertical box container
    vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 20);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    
    // Create label
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), 
                        "<span foreground='blue' size='x-large' weight='bold'>Hello World!</span>");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 10);
    
    // Create "Click Me!" button
    button = gtk_button_new_with_label("Click Me!");
    gtk_widget_set_size_request(button, 100, 30);
    g_signal_connect(button, "clicked", G_CALLBACK(on_button_clicked), window);
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 5);
    
    // Create "Exit" button
    exit_button = gtk_button_new_with_label("Exit");
    gtk_widget_set_size_request(exit_button, 100, 30);
    g_signal_connect(exit_button, "clicked", G_CALLBACK(on_window_destroy), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), exit_button, FALSE, FALSE, 5);
    
    // Show all widgets
    gtk_widget_show_all(window);
    
    // Start GTK main loop
    gtk_main();
    
    return 0;
}