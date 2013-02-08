/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * Copyright (c) 2013 Red Hat, Inc.
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
 * Author: Alexander Larsson <alexl@redhat.com>
 *
 */

#include <gtk/gtk.h>
#include "gd-stack.h"
#include <math.h>

enum  {
  PROP_0,
  PROP_HOMOGENOUS,
  PROP_VISIBLE_CHILD,
  PROP_VISIBLE_CHILD_NAME
};

struct _GdStackPrivate {
  GList *children;

  GtkWidget *visible_child;

  gboolean homogenous;
};

#define GTK_PARAM_READWRITE G_PARAM_READWRITE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB
#define GD_STACK_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GD_TYPE_STACK, GdStackPrivate))

static void     gd_stack_add                            (GtkContainer  *widget,
							 GtkWidget     *child);
static void     gd_stack_remove                         (GtkContainer  *widget,
							 GtkWidget     *child);
static void     gd_stack_forall                         (GtkContainer *container,
							 gboolean      include_internals,
							 GtkCallback   callback,
							 gpointer      callback_data);
static void    gd_stack_compute_expand                  (GtkWidget      *widget,
							 gboolean       *hexpand,
							 gboolean       *vexpand);
static void     gd_stack_size_allocate                  (GtkWidget     *widget,
							 GtkAllocation *allocation);
static gboolean gd_stack_draw                           (GtkWidget     *widget,
							 cairo_t       *cr);
static void     gd_stack_get_preferred_height           (GtkWidget     *widget,
							 gint          *minimum_height,
							 gint          *natural_height);
static void     gd_stack_get_preferred_height_for_width (GtkWidget     *widget,
							 gint           width,
							 gint          *minimum_height,
							 gint          *natural_height);
static void     gd_stack_get_preferred_width            (GtkWidget     *widget,
							 gint          *minimum_width,
							 gint          *natural_width);
static void     gd_stack_get_preferred_width_for_height (GtkWidget     *widget,
							 gint           height,
							 gint          *minimum_width,
							 gint          *natural_width);
static void     gd_stack_finalize                       (GObject       *obj);
static void     gd_stack_get_property                   (GObject       *object,
							 guint          property_id,
							 GValue        *value,
							 GParamSpec    *pspec);
static void     gd_stack_set_property                   (GObject       *object,
							 guint          property_id,
							 const GValue  *value,
							 GParamSpec    *pspec);

G_DEFINE_TYPE(GdStack, gd_stack, GTK_TYPE_BIN);

static void
gd_stack_init (GdStack *stack)
{
  GdStackPrivate *priv;

  priv = GD_STACK_GET_PRIVATE (stack);
  stack->priv = priv;

  gtk_widget_set_has_window ((GtkWidget*) stack, FALSE);
  gtk_widget_set_redraw_on_allocate ((GtkWidget*) stack, TRUE);
}

static void
gd_stack_finalize (GObject* obj)
{
  GdStack *stack = GD_STACK (obj);
  GdStackPrivate *priv = stack->priv;

  G_OBJECT_CLASS (gd_stack_parent_class)->finalize (obj);
}

static void
gd_stack_get_property (GObject *object,
		       guint property_id,
		       GValue *value,
		       GParamSpec *pspec)
{
  GdStack *stack = GD_STACK (object);
  GdStackPrivate *priv = stack->priv;

  switch (property_id) {
    case PROP_HOMOGENOUS:
      g_value_set_boolean (value, priv->homogenous);
      break;
    case PROP_VISIBLE_CHILD:
      g_value_set_object (value, priv->visible_child);
      break;
    case PROP_VISIBLE_CHILD_NAME:
      g_value_set_string (value, gd_stack_get_visible_child_name (stack));
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
gd_stack_set_property (GObject *object,
		       guint property_id,
		       const GValue *value,
		       GParamSpec *pspec)
{
  GdStack *stack = GD_STACK (object);

  switch (property_id) {
    case PROP_HOMOGENOUS:
      gd_stack_set_homogenous (stack, g_value_get_boolean (value));
      break;
    case PROP_VISIBLE_CHILD:
      gd_stack_set_visible_child (stack, g_value_get_object (value));
      break;
    case PROP_VISIBLE_CHILD_NAME:
      gd_stack_set_visible_child_name (stack, g_value_get_string (value));
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
gd_stack_class_init (GdStackClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->get_property = gd_stack_get_property;
  object_class->set_property = gd_stack_set_property;
  object_class->finalize = gd_stack_finalize;

  widget_class->size_allocate = gd_stack_size_allocate;
  widget_class->draw = gd_stack_draw;
  widget_class->get_preferred_height = gd_stack_get_preferred_height;
  widget_class->get_preferred_height_for_width = gd_stack_get_preferred_height_for_width;
  widget_class->get_preferred_width = gd_stack_get_preferred_width;
  widget_class->get_preferred_width_for_height = gd_stack_get_preferred_width_for_height;
  widget_class->compute_expand = gd_stack_compute_expand;

  container_class->add = gd_stack_add;
  container_class->remove = gd_stack_remove;
  container_class->forall = gd_stack_forall;
  /*container_class->get_path_for_child = gd_stack_get_path_for_child; */
  gtk_container_class_handle_border_width (container_class);

  g_object_class_install_property (object_class,
				   PROP_HOMOGENOUS,
				   g_param_spec_boolean ("homogenous",
							 "Homogenous",
							 "Homogenous sizing",
							 FALSE,
							 GTK_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class,
				   PROP_VISIBLE_CHILD,
				   g_param_spec_object ("visible-child",
							"Visible child",
							"The widget currently visible in the stack",
							GTK_TYPE_WIDGET,
							GTK_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_VISIBLE_CHILD_NAME,
				   g_param_spec_string ("visible-child-name",
							"Name of visible child",
							"The name of the widget currently visible in the stack",
							NULL,
							GTK_PARAM_READWRITE));

  g_type_class_add_private (klass, sizeof (GdStackPrivate));
}


GtkWidget *
gd_stack_new (void)
{
  return g_object_new (GD_TYPE_STACK, NULL);
}

static void
set_visible_child (GdStack *stack,
		   GtkWidget *child)
{
  GdStackPrivate *priv = stack->priv;
  GtkWidget *w;
  GList *l;

  /* If none, pick first visible */
  if (child == NULL)
    {
      for (l = priv->children; l != NULL; l = l->next)
	{
	  w = l->data;
	  if (gtk_widget_get_visible (w))
	    {
	      child = w;
	      break;
	    }
	}
    }

  if (priv->visible_child)
    gtk_widget_set_child_visible (priv->visible_child, FALSE);

  priv->visible_child = child;

  if (child)
    gtk_widget_set_child_visible (child, TRUE);

  gtk_widget_queue_resize (GTK_WIDGET (stack));

  g_object_notify (G_OBJECT (stack), "visible-child");
  g_object_notify (G_OBJECT (stack), "visible-child-name");
}

static void
stack_child_visibility_notify_cb (GObject *obj,
				  GParamSpec *pspec,
				  gpointer user_data)
{
  GdStack *stack = GD_STACK (user_data);
  GdStackPrivate *priv = stack->priv;
  GtkWidget *child = GTK_WIDGET (obj);

  if (priv->visible_child == NULL &&
      gtk_widget_get_visible (child))
    set_visible_child (stack, child);
  else if (priv->visible_child == child &&
	   gtk_widget_get_visible (child))
    set_visible_child (stack, NULL);
}

static void
gd_stack_add (GtkContainer *container,
	      GtkWidget *child)
{
  GdStack *stack = GD_STACK (container);
  GdStackPrivate *priv = stack->priv;

  g_return_if_fail (child != NULL);

  priv->children = g_list_append (priv->children, child);

  gtk_widget_set_parent (child, GTK_WIDGET (stack));

  g_signal_connect (child, "notify::visible",
                    G_CALLBACK (stack_child_visibility_notify_cb), stack);

  if (priv->visible_child == NULL &&
      gtk_widget_get_visible (child))
    set_visible_child (stack, child);
  else
    gtk_widget_set_child_visible (child, FALSE);

  if (priv->homogenous)
    gtk_widget_queue_resize (GTK_WIDGET (stack));
}

static void
gd_stack_remove (GtkContainer *container,
		 GtkWidget    *child)
{
  GdStack *stack = GD_STACK (container);
  GdStackPrivate *priv = stack->priv;
  gboolean was_visible;
  GList *l;

  l = g_list_find (priv->children, child);
  if (l == NULL)
    return;

  priv->children = g_list_delete_link (priv->children, l);

  g_signal_handlers_disconnect_by_func (child,
					stack_child_visibility_notify_cb,
					stack);

  was_visible = gtk_widget_get_visible (child);
  gtk_widget_unparent (child);

  if (priv->visible_child == child)
    set_visible_child (stack, NULL);

  if (priv->homogenous)
    gtk_widget_queue_resize (GTK_WIDGET (stack));
}

void
gd_stack_set_homogenous (GdStack *stack,
			 gboolean homogenous)
{
  GdStackPrivate *priv;

  g_return_if_fail (stack != NULL);

  priv = stack->priv;

  homogenous = !!homogenous;

  if (priv->homogenous == homogenous)
    return;

  priv->homogenous = homogenous;

  gtk_widget_queue_resize (GTK_WIDGET (stack));

  g_object_notify (G_OBJECT (stack), "homogenous");
}

gboolean
gd_stack_get_homogenous (GdStack *stack)
{
  GdStackPrivate *priv;

  g_return_if_fail (stack != NULL);

  return stack->priv->homogenous;
}

/**
 * gd_stack_get_visible_child:
 * @stack: a #GdStack
 *
 * Gets the currently visible child of the #GdStack, or %NULL if the
 * there are no visible children. The returned widget does not have a reference
 * added, so you do not need to unref it.
 *
 * Return value: (transfer none): pointer to child of the #GdStack
 **/
GtkWidget *
gd_stack_get_visible_child (GdStack *stack)
{
  GdStackPrivate *priv;

  g_return_if_fail (stack != NULL);

  return stack->priv->visible_child;
}

const char *
gd_stack_get_visible_child_name (GdStack *stack)
{
  GdStackPrivate *priv;

  g_return_val_if_fail (stack != NULL, NULL);

  if (stack->priv->visible_child)
    return gtk_widget_get_name (stack->priv->visible_child);

  return NULL;
}

void
gd_stack_set_visible_child (GdStack    *stack,
			    GtkWidget  *child)
{
  GdStackPrivate *priv;

  g_return_if_fail (stack != NULL);
  g_return_if_fail (child != NULL);

  priv = stack->priv;

  if (g_list_find (priv->children, child) == NULL)
    return;

  if (gtk_widget_get_visible (child))
    set_visible_child (stack, child);
}

void
gd_stack_set_visible_child_name (GdStack    *stack,
				 const char *name)
{
  GdStackPrivate *priv;
  GtkWidget *child;
  GList *l;

  g_return_if_fail (stack != NULL);
  g_return_if_fail (name != NULL);

  priv = stack->priv;

  child = NULL;
  for (l = priv->children; l != NULL; l = l->next)
    {
      GtkWidget *w = l->data;
      const char *child_name = gtk_widget_get_name (w);
      if (child_name != NULL &&
	  strcmp (child_name, name) == 0)
	{
	  child = w;
	  break;
	}
    }

  if (child != NULL && gtk_widget_get_visible (child))
    set_visible_child (stack, child);
}

static void
gd_stack_forall (GtkContainer *container,
		 gboolean      include_internals,
		 GtkCallback   callback,
		 gpointer      callback_data)
{
  GdStack *stack = GD_STACK (container);
  GdStackPrivate *priv = stack->priv;
  GList *l;

  for (l = priv->children; l != NULL; l = l->next)
    (* callback) (l->data, callback_data);
}

static void
gd_stack_compute_expand (GtkWidget      *widget,
			 gboolean       *hexpand_p,
			 gboolean       *vexpand_p)
{
  GdStack *stack = GD_STACK (widget);
  GdStackPrivate *priv = stack->priv;
  gboolean hexpand, vexpand;
  GList *l;

  hexpand = FALSE;
  vexpand = FALSE;
  for (l = priv->children; l != NULL; l = l->next)
    {
      GtkWidget *child = l->data;

      if (!hexpand &&
	  gtk_widget_compute_expand (child, GTK_ORIENTATION_HORIZONTAL))
	hexpand = TRUE;

      if (!vexpand &&
	  gtk_widget_compute_expand (child, GTK_ORIENTATION_VERTICAL))
	vexpand = TRUE;

      if (hexpand && vexpand)
	break;
    }

  *hexpand_p = hexpand;
  *vexpand_p = vexpand;
}

static gboolean
gd_stack_draw (GtkWidget *widget,
	       cairo_t *cr)
{
  GdStack *stack = GD_STACK (widget);
  GdStackPrivate *priv = stack->priv;

  if (priv->visible_child)
    gtk_container_propagate_draw (GTK_CONTAINER (stack),
				  priv->visible_child,
				  cr);

  return TRUE;
}

static void
gd_stack_size_allocate (GtkWidget *widget,
			GtkAllocation *allocation)
{
  GdStack *stack = GD_STACK (widget);
  GdStackPrivate *priv = stack->priv;
  GtkAllocation child_allocation;
  GtkWidget *child;
  gboolean window_visible;
  int bin_x, bin_y;

  g_return_if_fail (allocation != NULL);

  gtk_widget_set_allocation (widget, allocation);

  if (priv->visible_child)
    {
      GtkAllocation child_allocation = *allocation;
      gtk_widget_size_allocate (priv->visible_child, &child_allocation);
    }
}

static void
gd_stack_get_preferred_height (GtkWidget *widget,
			       gint *minimum_height,
			       gint *natural_height)
{
  GdStack *stack = GD_STACK (widget);
  GdStackPrivate *priv = stack->priv;
  GtkWidget *child;
  gint child_min, child_nat;
  GList *l;

  *minimum_height = 0;
  *natural_height = 0;

  for (l = priv->children; l != NULL; l = l->next)
    {
      child = l->data;
      if (!priv->homogenous &&
	  priv->visible_child != child)
	continue;
      if (gtk_widget_get_visible (child))
	{
	  gtk_widget_get_preferred_height (child, &child_min, &child_nat);

	  *minimum_height = MAX (*minimum_height, child_min);
	  *natural_height = MAX (*natural_height, child_nat);
	}
    }
}

static void
gd_stack_get_preferred_height_for_width (GtkWidget* widget,
					 gint width,
					 gint *minimum_height,
					 gint *natural_height)
{
  GdStack *stack = GD_STACK (widget);
  GdStackPrivate *priv = stack->priv;
  GtkWidget *child;
  gint child_min, child_nat;
  GList *l;

  *minimum_height = 0;
  *natural_height = 0;

  for (l = priv->children; l != NULL; l = l->next)
    {
      child = l->data;
      if (!priv->homogenous &&
	  priv->visible_child != child)
	continue;
      if (gtk_widget_get_visible (child))
	{
	  gtk_widget_get_preferred_height_for_width (child, width, &child_min, &child_nat);

	  *minimum_height = MAX (*minimum_height, child_min);
	  *natural_height = MAX (*natural_height, child_nat);
	}
    }
}

static void
gd_stack_get_preferred_width (GtkWidget *widget,
			       gint *minimum_width,
			       gint *natural_width)
{
  GdStack *stack = GD_STACK (widget);
  GdStackPrivate *priv = stack->priv;
  GtkWidget *child;
  gint child_min, child_nat;
  GList *l;

  *minimum_width = 0;
  *natural_width = 0;

  for (l = priv->children; l != NULL; l = l->next)
    {
      child = l->data;
      if (!priv->homogenous &&
	  priv->visible_child != child)
	continue;
      if (gtk_widget_get_visible (child))
	{
	  gtk_widget_get_preferred_width (child, &child_min, &child_nat);

	  *minimum_width = MAX (*minimum_width, child_min);
	  *natural_width = MAX (*natural_width, child_nat);
	}
    }
}

static void
gd_stack_get_preferred_width_for_height (GtkWidget* widget,
					 gint height,
					 gint *minimum_width,
					 gint *natural_width)
{
  GdStack *stack = GD_STACK (widget);
  GdStackPrivate *priv = stack->priv;
  GtkWidget *child;
  gint child_min, child_nat;
  GList *l;

  *minimum_width = 0;
  *natural_width = 0;

  for (l = priv->children; l != NULL; l = l->next)
    {
      child = l->data;
      if (!priv->homogenous &&
	  priv->visible_child != child)
	continue;
      if (gtk_widget_get_visible (child))
	{
	  gtk_widget_get_preferred_width_for_height (child, height, &child_min, &child_nat);

	  *minimum_width = MAX (*minimum_width, child_min);
	  *natural_width = MAX (*natural_width, child_nat);
	}
    }
}
