/* Standard headers.  */
#include <stdlib.h>

/* LV2 headers.  */
#include <lv2/lv2plug.in/ns/extensions/ui/ui.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>

/* GTK+ headers.  */
#include <gtk/gtk.h>

/* IndiePocket headers.  */
#include "indiepocket_io.h"

typedef struct {
  LV2_Atom_Forge forge;
  LV2_URID_Map *map;
  IPIOURIs uris;
  LV2UI_Write_Function write;
  LV2UI_Controller controller;
  GtkWidget *button;
} IndiePocketUI;

/* Load kit button click callback.  */
static void
on_load_clicked (GtkWidget *widget, void *handle)
{
  IndiePocketUI *ui = (IndiePocketUI *) handle;
  (void) widget;

  GtkWidget *dialog;
  dialog = gtk_file_chooser_dialog_new("Load Kit", NULL,
                                       GTK_FILE_CHOOSER_ACTION_OPEN,
                                       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                       GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                       NULL);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) != GTK_RESPONSE_ACCEPT)
    {
      gtk_widget_destroy (dialog);
      return;
    }

  char *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
  gtk_widget_destroy (dialog);

  uint8_t forgebuf[IPIO_FORGE_BUFFER_SIZE];
  lv2_atom_forge_set_buffer (&ui->forge, forgebuf, IPIO_FORGE_BUFFER_SIZE);
  LV2_Atom *kitmsg = ipio_forge_kit_file_atom (&ui->forge, &ui->uris,
                                               filename, strlen (filename));

  ui->write (ui->controller, IPIO_ATOM_IN, lv2_atom_total_size (kitmsg),
             ui->uris.atom_eventTransfer, kitmsg);

  g_free (filename);
}

/* Set up UI.  */
static LV2UI_Handle
instantiate (const LV2UI_Descriptor *descriptor, const char *plugin_uri,
             const char *bundle_path, LV2UI_Write_Function write_function,
             LV2UI_Controller controller, LV2UI_Widget *widget,
             const LV2_Feature * const *features)
{
  (void) descriptor;
  (void) plugin_uri;
  (void) bundle_path;

  IndiePocketUI *ui = (IndiePocketUI *) malloc (sizeof (IndiePocketUI));
  if (!ui)
    return NULL;

  ui->map = NULL;
  ui->write = write_function;
  ui->controller = controller;
  ui->button = NULL;

  *widget = NULL;

  for (uint32_t i = 0; features[i]; ++i)
    {
      if (!strcmp (features[i]->URI, LV2_URID__map))
        ui->map = (LV2_URID_Map *) features[i]->data;
    }

  if (!ui->map)
    {
      fprintf (stderr, "indiepocket_ui.c: Host does not support urid:Map\n");
      free (ui);
      return NULL;
    }

  ipio_map_uris (&ui->uris, ui->map);
  lv2_atom_forge_init (&ui->forge, ui->map);

  ui->button = gtk_button_new_with_label ("Load Kit");
  g_signal_connect (ui->button, "clicked", G_CALLBACK (on_load_clicked), ui);

  *widget = ui->button;

  return ui;
}

/* Free any resources allocated in `instantiate'.  */
static void
cleanup (LV2UI_Handle handle)
{
  IndiePocketUI *ui = (IndiePocketUI *) handle;
  gtk_widget_destroy (ui->button);
  free (ui);
}

/* Port event listener.  */
static void
port_event (LV2UI_Handle handle, uint32_t port_index, uint32_t buffer_size,
            uint32_t format, const void *buffer)
{
  IndiePocketUI *ui = (IndiePocketUI *) handle;
  (void) ui;
  (void) port_index;
  (void) buffer_size;
  (void) format;
  (void) buffer;
}

/* Return any extension data supported by this UI.  */
static const void *
extension_data (const char *uri)
{
  (void) uri;
  return NULL;
}

/* IndiePocket UI descriptor.  */
static const LV2UI_Descriptor descriptor = {
  IPCKT_UI_URI,
  instantiate,
  cleanup,
  port_event,
  extension_data
};

/* Export UI to lv2 host.  */
LV2_SYMBOL_EXPORT
const LV2UI_Descriptor *
lv2ui_descriptor (uint32_t index)
{
  if (index == 0)
    return &descriptor;
  return NULL;
}
