/*
 * Copyright (c) 2011 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by 
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public 
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License 
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Cosimo Cecchi <cosimoc@redhat.com>
 *
 */

#include "gd-styled-text-renderer.h"

typedef struct _GdStyledTextRendererPrivate GdStyledTextRendererPrivate;

struct _GdStyledTextRendererPrivate {
  GList *style_classes;
};

G_DEFINE_TYPE_WITH_PRIVATE (GdStyledTextRenderer, gd_styled_text_renderer, GTK_TYPE_CELL_RENDERER_TEXT)

static void
gd_styled_text_renderer_render (GtkCellRenderer      *cell,
                                cairo_t              *cr,
                                GtkWidget            *widget,
                                const GdkRectangle   *background_area,
                                const GdkRectangle   *cell_area,
                                GtkCellRendererState  flags)
{
  GdStyledTextRenderer *self = GD_STYLED_TEXT_RENDERER (cell);
  GdStyledTextRendererPrivate *priv;
  GtkStyleContext *context;
  const gchar *style_class;
  GList *l;

  priv = gd_styled_text_renderer_get_instance_private (self);

  context = gtk_widget_get_style_context (widget);
  gtk_style_context_save (context);

  for (l = priv->style_classes; l != NULL; l = l->next)
    {
      style_class = l->data;
      gtk_style_context_add_class (context, style_class);
    }

  GTK_CELL_RENDERER_CLASS (gd_styled_text_renderer_parent_class)->render 
    (cell, cr, widget,
     background_area, cell_area, flags);

  gtk_style_context_restore (context);
}

static void
gd_styled_text_renderer_finalize (GObject *obj)
{
  GdStyledTextRenderer *self = GD_STYLED_TEXT_RENDERER (obj);
  GdStyledTextRendererPrivate *priv;

  priv = gd_styled_text_renderer_get_instance_private (self);

  if (priv->style_classes != NULL)
    {
      g_list_free_full (priv->style_classes, g_free);
      priv->style_classes = NULL;
    }

  G_OBJECT_CLASS (gd_styled_text_renderer_parent_class)->finalize (obj);
}

static void
gd_styled_text_renderer_class_init (GdStyledTextRendererClass *klass)
{
  GtkCellRendererClass *crclass = GTK_CELL_RENDERER_CLASS (klass);
  GObjectClass *oclass = G_OBJECT_CLASS (klass);

  oclass->finalize = gd_styled_text_renderer_finalize;
  crclass->render = gd_styled_text_renderer_render;
}

static void
gd_styled_text_renderer_init (GdStyledTextRenderer *self)
{
}

GtkCellRenderer *
gd_styled_text_renderer_new (void)
{
  return g_object_new (GD_TYPE_STYLED_TEXT_RENDERER,
                       NULL);
}

void
gd_styled_text_renderer_add_class (GdStyledTextRenderer *self,
                                   const gchar *class)
{
  GdStyledTextRendererPrivate *priv;

  priv = gd_styled_text_renderer_get_instance_private (self);

  if (g_list_find_custom (priv->style_classes, class, (GCompareFunc) g_strcmp0))
    return;

  priv->style_classes = g_list_append (priv->style_classes, g_strdup (class));
}

void
gd_styled_text_renderer_remove_class (GdStyledTextRenderer *self,
                                      const gchar *class)
{
  GdStyledTextRendererPrivate *priv;
  GList *class_element;

  priv = gd_styled_text_renderer_get_instance_private (self);

  class_element = g_list_find_custom (priv->style_classes, class, (GCompareFunc) g_strcmp0);

  if (class_element == NULL)
    return;

  priv->style_classes = g_list_remove_link (priv->style_classes, class_element);
  g_free (class_element->data);
  g_list_free_1 (class_element);
}
