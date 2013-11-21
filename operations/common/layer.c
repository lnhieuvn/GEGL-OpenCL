/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */

/* FIXME: need to make this OpenRaster inspired layer integrate better
 * with the newer caching system
 */


#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_string(composite_op, _("Operation"), "gegl:over",
                  _("Composite operation to use"))
gegl_chant_double(opacity, _("Opacity"), 0.0, 1.0, 1.0,
                  _("Opacity"))
gegl_chant_double(x, _("X"), -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                  _("Horizontal position"))
gegl_chant_double(y, _("Y"), -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                  _("Vertical position"))
gegl_chant_double(scale, _("Scale"), -G_MAXDOUBLE, G_MAXDOUBLE, 1.0,
                  _("Scale 1:1 size"))
gegl_chant_file_path(src, _("Source"), "",
                _("Source datafile (png, jpg, raw, svg, bmp, tif, ...)"))

#else

#include <gegl-plugin.h>
struct _GeglChant
{
  GeglOperationMeta parent_instance;
  gpointer          properties;

  GeglNode *self;
  GeglNode *input;
  GeglNode *aux;
  GeglNode *output;

  GeglNode *composite_op;
  GeglNode *translate;
  GeglNode *opacity;
  GeglNode *scale;
  GeglNode *load;

  gchar *cached_path;

  gdouble p_opacity;
  gdouble p_scale;
  gdouble p_x;
  gdouble p_y;
  gchar  *p_composite_op;
};

typedef struct
{
  GeglOperationMetaClass parent_class;
} GeglChantClass;

#define GEGL_CHANT_C_FILE "layer.c"
#include "gegl-chant.h"
GEGL_DEFINE_DYNAMIC_OPERATION(GEGL_TYPE_OPERATION_META)

#include <glib/gprintf.h>

static void
do_setup (GeglOperation *operation)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  GeglChant *self = GEGL_CHANT (operation);

  /* If the src is NULL, and we previously used a source, clear what we have
   * cached and directly link the input and output. We don't need a composite
   * operation if we don't have a source, so don't continue preparing.
   */
  if (o->src[0] == 0)
    {
      if (self->cached_path != NULL)
        {
          gegl_node_link (self->input, self->output);
          g_free (self->cached_path);
          self->cached_path = NULL;
        }

      return;
    }

  /* Check if the composite operation we're using has changed from that which
   * is already in use.
   */
  if (!self->p_composite_op || strcmp (self->p_composite_op, o->composite_op))
    {
      gegl_node_set (self->composite_op,
                     "operation", o->composite_op,
                     NULL);
      if (self->p_composite_op)
        g_free (self->p_composite_op);
      self->p_composite_op = g_strdup (o->composite_op);
    }

  /* Load a src image, and relink the input/composite/output chain, as it
   * will currently be set to an input/output chain without a composite
   * source.
   */

  if (self->cached_path == NULL || strcmp (o->src, self->cached_path))
    {
      gegl_node_set (self->load,
          "operation", "gegl:load",
          NULL);
      gegl_node_set (self->load,
          "path",  o->src,
          NULL);

      /* Currently not using the composite op, reinsert it */
      if (!self->cached_path)
        gegl_node_link_many (self->input, self->composite_op, self->output, NULL);

      if (self->cached_path)
        g_free (self->cached_path);
      self->cached_path = g_strdup (o->src);
    }

  if (o->scale != self->p_scale)
    {
      gegl_node_set (self->scale,
                     "x",  o->scale,
                     "y",  o->scale,
                     NULL);
      self->p_scale= o->scale;
    }

  if (o->opacity != self->p_opacity)
    {
      gegl_node_set (self->opacity,
                     "value",  o->opacity,
                     NULL);
      self->p_opacity = o->opacity;
    }

  if (o->x != self->p_x ||
      o->y != self->p_y)
    {
      gegl_node_set (self->translate,
                     "x",  o->x,
                     "y",  o->y,
                     NULL);
      self->p_x = o->x;
      self->p_y = o->y;
    }


}

static void
my_set_property (GObject      *gobject,
                 guint         property_id,
                 const GValue *value,
                 GParamSpec   *pspec)
{
  GeglOperation *operation = GEGL_OPERATION (gobject);
  GeglChant     *self      = GEGL_CHANT (operation);

  /* The set_property provided by the chant system does the
   * storing and reffing/unreffing of the input properties */
  set_property(gobject, property_id, value, pspec);

  if (self->load)
    do_setup (operation);
}

static void attach (GeglOperation *operation)
{
  GeglChant  *self = GEGL_CHANT (operation);
  GeglChantO *o    = GEGL_CHANT_PROPERTIES (operation);
  GeglNode *gegl;

  self->self = GEGL_OPERATION (self)->node;
  gegl = self->self;

  self->input = gegl_node_get_input_proxy (gegl, "input");
  self->aux = gegl_node_get_input_proxy (gegl, "aux");
  self->output = gegl_node_get_output_proxy (gegl, "output");

  self->composite_op = gegl_node_new_child (gegl,
                                         "operation", o->composite_op,
                                         NULL);

  self->translate = gegl_node_new_child (gegl, "operation", "gegl:translate", NULL);
  self->scale = gegl_node_new_child (gegl, "operation", "gegl:scale-ratio", NULL);
  self->opacity = gegl_node_new_child (gegl, "operation", "gegl:opacity", NULL);

  self->load = gegl_node_new_child (gegl,
                                    "operation", "gegl:text",
                                    "string", "Load operation placeholder",
                                    NULL);

  gegl_node_link_many (self->load, self->scale, self->opacity, self->translate,
                       NULL);
  gegl_node_link_many (self->input, self->composite_op, self->output, NULL);
  gegl_node_connect_from (self->composite_op, "aux", self->translate, "output");

  do_setup (operation);
}


static void
finalize (GObject *object)
{
  GeglChant *self = GEGL_CHANT (object);

  if (self->cached_path)
    g_free (self->cached_path);
  if (self->p_composite_op)
    g_free (self->p_composite_op);

  G_OBJECT_CLASS (gegl_chant_parent_class)->finalize (object);
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GObjectClass       *object_class;
  GeglOperationClass *operation_class;

  object_class    = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->finalize     = finalize;
  object_class->set_property = my_set_property;

  operation_class->attach = attach;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:layer",
    "categories" , "meta",
    "description", _("A layer in the traditional sense"),
    NULL);
}


#endif
