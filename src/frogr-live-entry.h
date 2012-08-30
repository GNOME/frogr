/*
 * frogr-live-entry.h -- Frogr 'live' entry (autocompletion enabled)
 *
 * Copyright (C) 2011 Mario Sanchez Prada
 * Authors: Mario Sanchez Prada <msanchez@gnome.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 3 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>
 *
 */

#ifndef FROGR_LIVE_ENTRY_H
#define FROGR_LIVE_ENTRY_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define FROGR_TYPE_LIVE_ENTRY           (frogr_live_entry_get_type())
#define FROGR_LIVE_ENTRY(obj)           (G_TYPE_CHECK_INSTANCE_CAST(obj, FROGR_TYPE_LIVE_ENTRY, FrogrLiveEntry))
#define FROGR_LIVE_ENTRY_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST(klass, FROGR_TYPE_LIVE_ENTRY, FrogrLiveEntryClass))
#define FROGR_IS_LIVE_ENTRY(obj)           (G_TYPE_CHECK_INSTANCE_TYPE(obj, FROGR_TYPE_LIVE_ENTRY))
#define FROGR_IS_LIVE_ENTRY_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), FROGR_TYPE_LIVE_ENTRY))
#define FROGR_LIVE_ENTRY_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), FROGR_TYPE_LIVE_ENTRY, FrogrLiveEntryClass))

typedef struct _FrogrLiveEntry        FrogrLiveEntry;
typedef struct _FrogrLiveEntryClass   FrogrLiveEntryClass;

struct _FrogrLiveEntryClass
{
  GtkEntryClass parent_class;
};

struct _FrogrLiveEntry
{
  GtkEntry parent;
};

GType frogr_live_entry_get_type (void) G_GNUC_CONST;

GtkWidget *frogr_live_entry_new (void);

void frogr_live_entry_set_auto_completion (FrogrLiveEntry *self, GSList *data);

G_END_DECLS  /* FROGR_LIVE_ENTRY_H */

#endif
