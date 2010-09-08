static gboolean _picture_uploaded (upload_picture_st *up_st);
static void _upload_picture_thread (upload_picture_st *up_st);

/* Private API */

static gboolean
_picture_uploaded (upload_picture_st *up_st)
{
  g_return_val_if_fail (up_st != NULL, FALSE);
  GFunc callback = up_st->callback;
  gpointer object = up_st->object;
  gpointer data = up_st->data;

  /* Free memory */
  g_slice_free (upload_picture_st, up_st);

  /* Execute callback */
  if (callback)
    callback (object, data);

  /* Remove the GSource */
  return FALSE;
}

static void
_upload_picture_thread (upload_picture_st *up_st)
{
  g_return_if_fail (up_st != NULL);
  /* FrogrFacade *facade = up_st->facade; */
  /* FrogrPicture *picture = up_st->picture; */

  /* FrogrFacadePrivate *priv = FROGR_FACADE_GET_PRIVATE (facade); */
  /* flickcurl_upload_params *uparams; */
  /* flickcurl_upload_status *ustatus; */

  /* /\* Upload picture and gather photo ID *\/ */

  /* /\* Prepare upload params *\/ */
  /* uparams = g_slice_new (flickcurl_upload_params); */
  /* uparams->title = frogr_picture_get_title (picture); */
  /* uparams->photo_file = frogr_picture_get_filepath (picture); */
  /* uparams->description = frogr_picture_get_description (picture); */
  /* uparams->tags = frogr_picture_get_tags (picture); */
  /* uparams->is_public = frogr_picture_is_public (picture); */
  /* uparams->is_friend = frogr_picture_is_friend (picture); */
  /* uparams->is_family = frogr_picture_is_family (picture); */
  /* uparams->safety_level = 1; /\* Harcoded: 'safe' *\/ */
  /* uparams->content_type = 1; /\* Harcoded: 'photo' *\/ */

  /* /\* Upload the test photo (private) *\/ */
  /* g_debug ("\n\nNow uploading picture %s...\n", */
  /*          frogr_picture_get_title (picture)); */

  /* ustatus = flickcurl_photos_upload_params (priv->fcurl, uparams); */
  /* if (ustatus) { */
  /*   g_debug ("[OK] Success uploading a new picture\n"); */

  /*   /\* Save photo ID along with the picture *\/ */
  /*   frogr_picture_set_id (picture, ustatus->photoid); */

  /*   /\* Print and free upload status *\/ */
  /*   g_debug ("\tPhoto upload status:"); */
  /*   g_debug ("\t\tPhotoID: %s", ustatus->photoid); */
  /*   g_debug ("\t\tSecret: %s", ustatus->secret); */
  /*   g_debug ("\t\tOriginalSecret: %s", ustatus->originalsecret); */
  /*   g_debug ("\t\tTicketID: %s", ustatus->ticketid); */

  /*   /\* Free *\/ */
  /*   flickcurl_free_upload_status (ustatus); */

  /* } else { */
  /*   g_debug ("[ERRROR] Failure uploading a new picture\n"); */
  /* } */

  /* /\* Free memory *\/ */
  /* g_slice_free (flickcurl_upload_params, uparams); */

  /* /\* At last, just call the return callback *\/ */
  /* g_idle_add ((GSourceFunc)_picture_uploaded, up_st); */
}

void
frogr_facade_upload_picture (FrogrFacade *self,
                             FrogrPicture *picture,
                             GFunc callback,
                             gpointer object,
                             gpointer data)
{
  g_return_if_fail(FROGR_IS_FACADE (self));

  upload_picture_st *up_st;

  /* Create structure to pass to the thread */
  up_st = g_slice_new (upload_picture_st);
  up_st->facade = self;
  up_st->picture = picture;
  up_st->callback = callback;
  up_st->object = object;
  up_st->data = data;

  /* Initiate the process in another thread */
  g_thread_create ((GThreadFunc)_upload_picture_thread,
                   up_st, FALSE, NULL);
}
