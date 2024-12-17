#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char mot[50];
    int frequence;
} Mot;

typedef struct {
    GtkApplicationWindow parent;
    GtkWidget *stack;
    GtkWidget *search_bar;
    GtkWidget *search_entry;
    GtkWidget *results_view;
    GtkWidget *open_button;
    GtkWidget *analyze_button;
    GtkWidget *save_button;
    GtkWidget *stats_label;
    Mot tableauMots[1000];
    int nombreMots;
    int totalMots;
    int totalLignes;
    int totalCaracteres;
} TextAnalyzerWindow;

G_DEFINE_TYPE(TextAnalyzerWindow, text_analyzer_window, GTK_TYPE_APPLICATION_WINDOW)

static void
text_analyzer_window_init(TextAnalyzerWindow *win) {
    gtk_widget_init_template(GTK_WIDGET(win));
    win->nombreMots = 0;
    win->totalMots = 0;
    win->totalLignes = 0;
    win->totalCaracteres = 0;
}

static void
open_file_dialog(GtkWidget *button, TextAnalyzerWindow *win) {
    GtkWidget *dialog;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    
    dialog = gtk_file_chooser_dialog_new("Open File",
                                        GTK_WINDOW(win),
                                        action,
                                        "_Cancel",
                                        GTK_RESPONSE_CANCEL,
                                        "_Open",
                                        GTK_RESPONSE_ACCEPT,
                                        NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        filename = gtk_file_chooser_get_filename(chooser);
        
        // Here we'll implement file reading and analysis
        FILE *file = fopen(filename, "r");
        if (file != NULL) {
            char buffer[1024];
            GString *content = g_string_new("");
            
            while (fgets(buffer, sizeof(buffer), file)) {
                g_string_append(content, buffer);
                win->totalLignes++;
            }
            
            // Update stats
            win->totalCaracteres = content->len;
            win->totalMots = 0;  // We'll implement word counting
            
            // Update stats label
            char stats[256];
            snprintf(stats, sizeof(stats), 
                    "Lines: %d\nCharacters: %d\nWords: %d",
                    win->totalLignes,
                    win->totalCaracteres,
                    win->totalMots);
            gtk_label_set_text(GTK_LABEL(win->stats_label), stats);
            
            // Show text in view
            GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(win->results_view));
            gtk_text_buffer_set_text(buffer, content->str, content->len);
            
            g_string_free(content, TRUE);
            fclose(file);
        }
        
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

static void
save_results(GtkWidget *button, TextAnalyzerWindow *win) {
    GtkWidget *dialog;
    GtkFileChooser *chooser;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;

    dialog = gtk_file_chooser_dialog_new("Save Results",
                                        GTK_WINDOW(win),
                                        action,
                                        "_Cancel",
                                        GTK_RESPONSE_CANCEL,
                                        "_Save",
                                        GTK_RESPONSE_ACCEPT,
                                        NULL);
    chooser = GTK_FILE_CHOOSER(dialog);

    gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(chooser);
        FILE *file = fopen(filename, "w");
        
        if (file != NULL) {
            fprintf(file, "Text Analysis Results\n");
            fprintf(file, "====================\n\n");
            fprintf(file, "Statistics:\n");
            fprintf(file, "- Lines: %d\n", win->totalLignes);
            fprintf(file, "- Characters: %d\n", win->totalCaracteres);
            fprintf(file, "- Words: %d\n", win->totalMots);
            
            // Add word frequency when implemented
            
            fclose(file);
        }
        
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

static void
text_analyzer_window_class_init(TextAnalyzerWindowClass *class) {
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class),
                                              "/org/gtk/textanalyzer/window.ui");
    
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), TextAnalyzerWindow, stack);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), TextAnalyzerWindow, search_bar);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), TextAnalyzerWindow, search_entry);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), TextAnalyzerWindow, results_view);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), TextAnalyzerWindow, open_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), TextAnalyzerWindow, analyze_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), TextAnalyzerWindow, save_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), TextAnalyzerWindow, stats_label);
}

static void
activate(GtkApplication *app, gpointer user_data) {
    TextAnalyzerWindow *win;
    win = g_object_new(text_analyzer_window_get_type(), "application", app, NULL);
    gtk_window_present(GTK_WINDOW(win));
}

int
main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("org.gtk.textanalyzer", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}