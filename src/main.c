#include <config.h>
#include <glib.h>
#include "frogr-controller.h"

static FrogrController *fcontroller = NULL;

int
main (int argc, char **argv)
{
  /* Init threads system */
  g_thread_init (NULL);
  gdk_threads_init ();

  gdk_threads_enter ();
  gtk_init (&argc, &argv);
  g_set_application_name(PACKAGE);

  /* Run app */
  fcontroller = frogr_controller_get_instance ();
  frogr_controller_run_app (fcontroller);

  gdk_threads_leave ();

  return 0;
}
