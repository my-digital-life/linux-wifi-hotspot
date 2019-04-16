//
// Created by lakinduakash on 13/04/19.
//



#include <gtk/gtk.h>
#include <stdlib.h>
#include <X11/Xlib.h>

#include "h_prop.h"
#include "ui.h"
#include "read_config.h"
#include "util.h"

#define BUFSIZE 512
#define AP_ENABLED "AP-ENABLED"

GtkBuilder *builder;
GObject *window;
GtkButton *button_create_hp;
GtkButton *button_stop_hp;

GtkEntry *entry_ssd;
GtkEntry *entry_pass;
GtkEntry *entry_mac;
GtkEntry *entry_channel;

GtkComboBox *combo_wifi;
GtkComboBox *combo_internet;

GtkRadioButton *rb_freq_auto;
GtkRadioButton *rb_freq_2;
GtkRadioButton *rb_freq_5;

GtkCheckButton *cb_hidden;
GtkCheckButton *cb_psk;
GtkCheckButton *cb_mac;
GtkCheckButton *cb_novirt;

GtkProgressBar *progress_bar;

GtkLabel *label_status;

GError *error = NULL;


const char** iface_list;
const char** wifi_iface_list;
int iface_list_length;
int wifi_iface_list_length;
char* running_info[3];
guint id;



static void *stopHp() {
    if(running_info[0]!=NULL){
        gtk_label_set_label(label_status,"Stopping ...");
        start_pb_pulse();
        lock_all_views(TRUE);
        startShell(build_kill_create_ap_command(running_info[0]));
        g_thread_new("init_running",init_running_info,NULL);
    }
    return 0;
}

static void on_create_hp_clicked(GtkWidget *widget, gpointer data) {

    WIData *d = (WIData *) data;

    if (gtk_combo_box_get_active (combo_wifi) >= 0 && gtk_combo_box_get_active (combo_internet) >= 0) {


        gchar *wifi = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT(combo_wifi));
        gchar *internet = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT(combo_internet));

        //Remove trailing new lines
        wifi[strcspn(wifi, "\n")] = 0;
        internet[strcspn(internet, "\n")] = 0;

        startShell(build_wh_mkconfig_command(wifi, internet, (char *) gtk_entry_get_text(d->ssid),
                                             (char *) gtk_entry_get_text(d->pass)));

        g_thread_new("shell_create_hp", run_create_hp_shell, (void*)build_wh_from_config());

        g_free (wifi);
        g_free (internet);
    } else{

        g_print("Please select Wifi and Internet interfaces\n");
    }


}

static void on_stop_hp_clicked(GtkWidget *widget, gpointer data) {
    g_thread_new("shell2", stopHp, NULL);

}

int initUi(int argc, char *argv[]){

    XInitThreads();

    gtk_init(&argc, &argv);

    /* Construct a GtkBuilder instance and load our UI description */
    const char* debug_glade_file="glade/wifih.ui";
    const char* prod_glade_file="/usr/share/wihotspot/glade/wifih.ui";

    FILE *file;
    builder = gtk_builder_new();

    if ((file = fopen(debug_glade_file, "r"))){
        fclose(file);
        if (gtk_builder_add_from_file(builder, debug_glade_file, &error) == 0) {
            g_printerr("Error loading file: %s\n", error->message);
            g_clear_error(&error);
            return 1;
        }
    }
    else if ((file = fopen(prod_glade_file, "r"))){
        fclose(file);
        if (gtk_builder_add_from_file(builder, prod_glade_file, &error) == 0) {
            g_printerr("Error loading file: %s\n", error->message);
            g_clear_error(&error);
            return 1;
        }
    } else{
        return 1;
    }


    /* Connect signal handlers to the constructed widgets. */
    window = gtk_builder_get_object(builder, "window");
    g_signal_connect (window, "destroy", G_CALLBACK(gtk_main_quit), NULL);


    button_create_hp = (GtkButton *) gtk_builder_get_object(builder, "button_create_hp");
    button_stop_hp = (GtkButton *) gtk_builder_get_object(builder, "button_stop_hp");

    entry_ssd = (GtkEntry *) gtk_builder_get_object(builder, "entry_ssid");
    entry_pass = (GtkEntry *) gtk_builder_get_object(builder, "entry_pass");

    entry_mac = (GtkEntry *) gtk_builder_get_object(builder, "entry_mac");
    entry_channel = (GtkEntry *) gtk_builder_get_object(builder, "entry_channel");

    combo_wifi = (GtkComboBox *) gtk_builder_get_object(builder, "combo_wifi");
    combo_internet = (GtkComboBox *) gtk_builder_get_object(builder, "combo_internet");

    cb_hidden = (GtkCheckButton *) gtk_builder_get_object(builder, "cb_hidden");
    cb_psk = (GtkCheckButton *) gtk_builder_get_object(builder, "cb_psk");
    cb_mac = (GtkCheckButton *) gtk_builder_get_object(builder, "cb_mac");
    cb_novirt = (GtkCheckButton *) gtk_builder_get_object(builder, "cb_novirt");

    label_status = (GtkLabel *) gtk_builder_get_object(builder, "label_status");

    progress_bar = (GtkProgressBar *) gtk_builder_get_object(builder, "progress_bar");


    //gtk_entry_set_visibility(entry_pass,FALSE);

    WIData wiData = {
            .pass= entry_pass,
            .ssid= entry_ssd
    };

    start_pb_pulse();
    g_thread_new("init_running",init_running_info,NULL);
    //init_running_info();

    init_interface_list();

    init_ui_from_config(&wiData);

    g_signal_connect (button_create_hp, "clicked", G_CALLBACK(on_create_hp_clicked), &wiData);
    g_signal_connect (button_stop_hp, "clicked", G_CALLBACK(on_stop_hp_clicked), NULL);


    gtk_main();

    return 0;
}


void init_ui_from_config(WIData* data){

    if(read_config_file()==READ_CONFIG_FILE_SUCCESS){

        ConfigValues *values=getConfigValues();


        if(values->ssid!=NULL)
            gtk_entry_set_text(data->ssid,values->ssid);
        if(values->pass!=NULL)
            gtk_entry_set_text(data->pass,values->pass);

        if(values->iface_wifi!=NULL){
            int idw=find_str(values->iface_wifi,wifi_iface_list,wifi_iface_list_length);

            if(idw !=-1){
                gtk_combo_box_set_active(combo_wifi,idw);
            }
        }

        if(values->iface_inet!=NULL){
            int idw=find_str(values->iface_inet,iface_list,iface_list_length);

            if(idw !=-1){
                gtk_combo_box_set_active(combo_internet,idw);
            }
        }

    }
}

void init_interface_list(){

    iface_list_length=0;
    wifi_iface_list_length=0;
    iface_list=(const char**)get_interface_list(&iface_list_length);
    wifi_iface_list=(const char**)get_wifi_interface_list(&wifi_iface_list_length);

    for (int i = 0; i < iface_list_length; i++){

        gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_internet), iface_list[i]);
    }

    for (int i = 0; i < wifi_iface_list_length; i++){
        gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_wifi), wifi_iface_list[i]);
    }

}


void lock_all_views(gboolean set_lock){
    if(set_lock){
        gtk_editable_set_editable( (GtkEditable*)entry_ssd,FALSE);
        gtk_editable_set_editable( (GtkEditable*)entry_pass,FALSE);
        gtk_widget_set_sensitive ((GtkWidget*)button_create_hp, FALSE);
        gtk_widget_set_sensitive ((GtkWidget*)button_stop_hp, FALSE);
        gtk_widget_set_sensitive ((GtkWidget*)combo_internet, FALSE);
        gtk_widget_set_sensitive ((GtkWidget*)combo_wifi, FALSE);
    } else{
        gtk_editable_set_editable( (GtkEditable*)entry_ssd,TRUE);
        gtk_editable_set_editable( (GtkEditable*)entry_pass,TRUE);
        gtk_widget_set_sensitive ((GtkWidget*)button_create_hp, TRUE);
        gtk_widget_set_sensitive ((GtkWidget*)button_stop_hp, TRUE);
        gtk_widget_set_sensitive ((GtkWidget*)button_stop_hp, TRUE);
        gtk_widget_set_sensitive ((GtkWidget*)combo_internet, TRUE);
        gtk_widget_set_sensitive ((GtkWidget*)combo_wifi, TRUE);
    }
}


void lock_running_views(gboolean set_lock){
    if(set_lock){
        gtk_editable_set_editable( (GtkEditable*)entry_ssd,FALSE);
        gtk_editable_set_editable( (GtkEditable*)entry_pass,FALSE);
        gtk_widget_set_sensitive ((GtkWidget*)button_create_hp, FALSE);

        gtk_widget_set_sensitive ((GtkWidget*)button_stop_hp, TRUE);

        gtk_widget_set_sensitive ((GtkWidget*)combo_internet, FALSE);
        gtk_widget_set_sensitive ((GtkWidget*)combo_wifi, FALSE);
    } else{
        gtk_editable_set_editable( (GtkEditable*)entry_ssd,TRUE);
        gtk_editable_set_editable( (GtkEditable*)entry_pass,TRUE);
        gtk_widget_set_sensitive ((GtkWidget*)button_create_hp, TRUE);

        gtk_widget_set_sensitive ((GtkWidget*)button_stop_hp, FALSE);

        gtk_widget_set_sensitive ((GtkWidget*)combo_internet, TRUE);
        gtk_widget_set_sensitive ((GtkWidget*)combo_wifi, TRUE);
    }
}

static guint start_pb_pulse(){
    gtk_widget_set_visible((GtkWidget*)progress_bar,TRUE);
    return id= g_timeout_add (100, update_progress_in_timeout, progress_bar);
}

static void stop_pb_pulse(){
    g_source_remove(id);
    gtk_widget_set_visible((GtkWidget*)progress_bar,FALSE);
}

static gboolean update_progress_in_timeout (gpointer pbar)
{
    gtk_progress_bar_pulse (pbar);
    return TRUE; /* keep running */
}


void clear_running_info(){

        g_free(running_info[0]);
        running_info[0]=NULL;
}

void* init_running_info(){

    clear_running_info();
    lock_all_views(TRUE);

    gtk_label_set_label(label_status,"Getting running info...");

    get_running_info(running_info);

    if(running_info[0]!=NULL){

        char a[BUFSIZE];
        snprintf(a,BUFSIZE,"Running as PID: %s",running_info[0]);
        gtk_label_set_label(label_status,a);

        lock_all_views(FALSE);
        lock_running_views(TRUE);


    } else{

        gtk_label_set_label(label_status,"Not running");
        lock_all_views(FALSE);
        lock_running_views(FALSE);
    }

    stop_pb_pulse();
    return 0;
}


static void *run_create_hp_shell(void *cmd) {

    char buf[BUFSIZE];
    FILE *fp;

    if ((fp = popen(cmd, "r")) == NULL) {
        printf("Error opening pipe!\n");
        return NULL;
    }

    start_pb_pulse();

    while (fgets(buf, BUFSIZE, fp) != NULL) {
        buf[strcspn(buf, "\n")] = 0;
        gtk_label_set_label(label_status,buf);

        if (strstr(buf, AP_ENABLED) != NULL) {
            init_running_info();
            return 0;
        }
    }

    if (pclose(fp)) {
        printf("Command not found or exited with error status\n");
        init_running_info();
        return NULL;
    }

    init_running_info();
    return 0;
}
