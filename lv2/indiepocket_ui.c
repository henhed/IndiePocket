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
on_file_selected (GtkWidget *widget, void *handle)
{
  IndiePocketUI *ui = (IndiePocketUI *) handle;
  char *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (widget));

  gtk_widget_set_sensitive (widget, FALSE);

  uint8_t forgebuf[IPIO_FORGE_BUFFER_SIZE];
  lv2_atom_forge_set_buffer (&ui->forge, forgebuf, IPIO_FORGE_BUFFER_SIZE);
  LV2_Atom *kitmsg = ipio_forge_kit_file_atom (&ui->forge, &ui->uris,
                                               filename);

  ui->write (ui->controller, IPIO_CONTROL, lv2_atom_total_size (kitmsg),
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

  ui->button = gtk_file_chooser_button_new (NULL,
                                            GTK_FILE_CHOOSER_ACTION_OPEN);
  g_signal_connect (ui->button, "file-set", G_CALLBACK (on_file_selected), ui);

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
  (void) port_index;
  (void) buffer_size;

  if (format != ui->uris.atom_eventTransfer)
    {
      fprintf (stderr, "Unknown format\n");
      return;
    }

  const LV2_Atom *atom = (const LV2_Atom *) buffer;
  if (!ipio_atom_type_is_object (&ui->forge, atom->type))
    {
      fprintf (stderr, "Unknown message type.\n");
      return;
    }

  const LV2_Atom_Object *obj = (const LV2_Atom_Object *) atom;
  const LV2_Atom *kit_path = ipio_atom_get_kit_file (&ui->uris, obj);
  if (!kit_path)
    {
      fprintf (stderr, "Unknown message sent to UI.\n");
      return;
    }

  const char *filename = (const char *) LV2_ATOM_BODY_CONST (kit_path);
  if (strlen (filename))
    {
      GFile *kit_file = g_file_new_for_path (filename);
      gtk_file_chooser_select_file (GTK_FILE_CHOOSER (ui->button), kit_file,
                                    NULL);
      g_object_unref (kit_file);
    }
  else
    gtk_file_chooser_unselect_all (GTK_FILE_CHOOSER (ui->button));

  gtk_widget_set_sensitive (ui->button, TRUE);
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
