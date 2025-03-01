/*
 * GStreamer
 * Copyright (C) 2018-2020 RidgeRun <support@ridgerun.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

/*
 * This is the modified version of RidgeRun code
 * (https://github.com/RidgeRun/gst-inference) to support Xilinx VVAS product
 * specific use cases
 */

#include "gstinferenceprediction.h"

static GType gst_inference_prediction_get_type (void);
GST_DEFINE_MINI_OBJECT_TYPE (GstInferencePrediction, gst_inference_prediction);

typedef struct _PredictionScaleData PredictionScaleData;
struct _PredictionScaleData
{
  GstVideoInfo *from;
  GstVideoInfo *to;
};

typedef struct _PredictionFindData PredictionFindData;
struct _PredictionFindData
{
  GstInferencePrediction *prediction;
  guint64 prediction_id;
};

static void gst_inference_prediction_free (GstInferencePrediction * self);
static GstInferencePrediction *prediction_copy (const GstInferencePrediction *
    self);
static void prediction_free (GstInferencePrediction * obj);
static gboolean prediction_find (GstInferencePrediction * obj,
    PredictionFindData * found);
static GstInferencePrediction *prediction_find_unlocked (GstInferencePrediction
    * self, guint64 id);
static void prediction_reset (GstInferencePrediction * self);
static gchar *prediction_to_string (GstInferencePrediction * self, gint level);
static gchar *prediction_children_to_string (GstInferencePrediction * self,
    gint level);
static gchar *prediction_classes_to_string (GstInferencePrediction * self,
    gint level);
static GstInferencePrediction *prediction_scale (const GstInferencePrediction *
    self, GstVideoInfo * to, GstVideoInfo * from);
static void prediction_scale_ip (GstInferencePrediction * self,
    GstVideoInfo * to, GstVideoInfo * from);
static GSList *prediction_get_children_unlocked (GstInferencePrediction * self);
static gboolean prediction_merge (GstInferencePrediction * src,
    GstInferencePrediction * dst);
static void prediction_get_enabled (GstInferencePrediction * self,
    GList ** enabled);

static GstInferenceClassification
    * classification_copy (GstInferenceClassification * from, gpointer data);
static void classification_merge (GList * src, GList ** dst);
static gint classification_compare (gconstpointer a, gconstpointer b);
static void segmentation_copy (const GstInferencePrediction * self,
    GstInferencePrediction * other);
static void reid_copy (const GstInferencePrediction * self,
    GstInferencePrediction * other);

static void bounding_box_reset (VvasBoundingBox * bbox);
static gchar *bounding_box_to_string (VvasBoundingBox * bbox, gint level);

static void node_get_children (VvasTreeNode * node, gpointer data);
static gpointer node_copy (gconstpointer node, gpointer data);
static gboolean node_scale_ip (VvasTreeNode * node, gpointer data);
static gpointer node_scale (gconstpointer, gpointer data);
static gboolean node_assign (VvasTreeNode * node, gpointer data);
static gboolean node_find (VvasTreeNode * node, gpointer data);
static gboolean node_get_enabled (VvasTreeNode * node, gpointer data);

static void compute_factors (GstVideoInfo * from, GstVideoInfo * to,
    gdouble * hfactor, gdouble * vfactor);

GstInferencePrediction *
gst_inference_prediction_new (void)
{
  GstInferencePrediction *self = g_slice_new (GstInferencePrediction);

  gst_mini_object_init (GST_MINI_OBJECT_CAST (self), 0,
      gst_inference_prediction_get_type (),
      (GstMiniObjectCopyFunction) gst_inference_prediction_copy, NULL,
      (GstMiniObjectFreeFunction) gst_inference_prediction_free);

  g_mutex_init (&self->mutex);

  self->prediction.node = NULL;
  self->prediction.classifications = NULL;
  self->sub_buffer = NULL;
  self->prediction.segmentation.data = NULL;
  self->prediction.reid.data = NULL;
  self->prediction.obj_track_label = NULL;
  self->prediction.model_name = NULL;
  self->prediction.model_class = VVAS_XCLASS_NOTFOUND;
  self->prediction.tb = NULL;

  prediction_reset (self);

  return self;
}

GstInferencePrediction *
gst_inference_prediction_new_full (VvasBoundingBox * bbox)
{
  GstInferencePrediction *self = NULL;

  g_return_val_if_fail (bbox, NULL);

  self = gst_inference_prediction_new ();

  GST_INFERENCE_PREDICTION_LOCK (self);
  self->prediction.bbox = *bbox;
  GST_INFERENCE_PREDICTION_UNLOCK (self);

  return self;
}

GstInferencePrediction *
gst_inference_prediction_ref (GstInferencePrediction * self)
{
  g_return_val_if_fail (self, NULL);

  return (GstInferencePrediction *)
      gst_mini_object_ref (GST_MINI_OBJECT_CAST (self));
}

void
gst_inference_prediction_unref (GstInferencePrediction * self)
{
  g_return_if_fail (self);

  gst_mini_object_unref (GST_MINI_OBJECT_CAST (self));
}

void
gst_inference_prediction_append (GstInferencePrediction * self,
    GstInferencePrediction * child)
{
  g_return_if_fail (self);
  g_return_if_fail (child);

  GST_INFERENCE_PREDICTION_LOCK (self);
  GST_INFERENCE_PREDICTION_LOCK (child);
  vvas_treenode_append (self->prediction.node, child->prediction.node);
  GST_INFERENCE_PREDICTION_UNLOCK (child);
  GST_INFERENCE_PREDICTION_UNLOCK (self);
}

static GstInferenceClassification *
classification_copy (GstInferenceClassification * from, gpointer data)
{
  return gst_inference_classification_copy (from);
}

static void
reid_copy (const GstInferencePrediction * self, GstInferencePrediction * other)
{
  if (self->prediction.reid.copy)
    self->prediction.reid.copy (&self->prediction.reid,
        &other->prediction.reid);
}

static void
segmentation_copy (const GstInferencePrediction * self,
    GstInferencePrediction * other)
{
  if (self->prediction.segmentation.copy)
    self->prediction.segmentation.copy (&self->prediction.segmentation,
        &other->prediction.segmentation);
}

static void
tb_copy (const GstInferencePrediction * self, GstInferencePrediction * other)
{
  if (self->prediction.tb->copy)
    self->prediction.tb->copy ((void **) &self->prediction.tb,
        (void **) &other->prediction.tb);
}

static GstInferencePrediction *
prediction_copy (const GstInferencePrediction * self)
{
  GstInferencePrediction *other = NULL;

  g_return_val_if_fail (self, NULL);

  other = gst_inference_prediction_new ();

  other->prediction.prediction_id = self->prediction.prediction_id;
  other->prediction.enabled = self->prediction.enabled;
  other->prediction.bbox = self->prediction.bbox;
  other->prediction.model_class = self->prediction.model_class;
  other->prediction.count = self->prediction.count;
  other->prediction.pose14pt = self->prediction.pose14pt;
  other->prediction.feature = self->prediction.feature;

  if (self->sub_buffer != NULL)
    other->sub_buffer = gst_buffer_ref (self->sub_buffer);
  if (self->prediction.segmentation.data != NULL)
    segmentation_copy (self, other);
  if (self->prediction.reid.data != NULL)
    reid_copy (self, other);
  if (self->prediction.tb != NULL)
    tb_copy (self, other);

  if (self->prediction.obj_track_label)
    other->prediction.obj_track_label =
        g_strdup (self->prediction.obj_track_label);

  if (self->prediction.model_name)
    other->prediction.model_name = g_strdup (self->prediction.model_name);

  other->reserved_1 = self->reserved_1;
  other->reserved_2 = self->reserved_2;
  other->reserved_3 = self->reserved_3;
  other->reserved_4 = self->reserved_4;
  other->reserved_5 = self->reserved_5;

  other->prediction.classifications =
      vvas_list_copy_deep (self->prediction.classifications,
      (vvas_list_copy_func) classification_copy, NULL);

  return other;
}

static void
prediction_scale_pose14pt_ip (const GstInferencePrediction * self,
    gdouble hfactor, gdouble vfactor)
{

  gint num;
  Pose14Pt *self_pose14pt = (Pose14Pt *) & self->prediction.pose14pt;
  Pointf *self_point_ptr = (Pointf *) self_pose14pt;

  for (num = 0; num < NUM_POSE_POINT; num++) {
    self_point_ptr->x = nearbyintf (self_point_ptr->x * hfactor);
    self_point_ptr->y = nearbyintf (self_point_ptr->y * vfactor);

    self_point_ptr++;
  }
}

static void
prediction_scale_pose14pt (const GstInferencePrediction * self,
    GstInferencePrediction * dest, gdouble hfactor, gdouble vfactor)
{

  gint num;
  Pose14Pt *self_pose14pt = (Pose14Pt *) & self->prediction.pose14pt;
  Pointf *self_point_ptr = (Pointf *) self_pose14pt;
  Pose14Pt *dest_pose14pt = (Pose14Pt *) & dest->prediction.pose14pt;
  Pointf *dest_point_ptr = (Pointf *) dest_pose14pt;

  for (num = 0; num < NUM_POSE_POINT; num++) {
    dest_point_ptr->x = nearbyintf (self_point_ptr->x * hfactor);
    dest_point_ptr->y = nearbyintf (self_point_ptr->y * vfactor);

    self_point_ptr++;
    dest_point_ptr++;
  }
}


static gpointer
node_copy (gconstpointer node, gpointer data)
{
  GstInferencePrediction *self = (GstInferencePrediction *) node;

  g_return_val_if_fail (node, NULL);

  return prediction_copy (self);
}

static gboolean
node_assign (VvasTreeNode * node, gpointer data)
{
  GstInferencePrediction *pred = (GstInferencePrediction *) node->data;

  g_return_val_if_fail (node, FALSE);

  if (pred->prediction.node) {
    vvas_treenode_destroy (pred->prediction.node);
    pred->prediction.node = NULL;
  }

  pred->prediction.node = node;

  return FALSE;
}

GstInferencePrediction *
gst_inference_prediction_copy (const GstInferencePrediction * self)
{
  VvasTreeNode *other = NULL;

  g_return_val_if_fail (self, NULL);

  GST_INFERENCE_PREDICTION_LOCK ((GstInferencePrediction *) self);

  /* Copy the binary tree */
  other = vvas_treenode_copy_deep (self->prediction.node, node_copy, NULL);

  /* Now finish assigning the nodes to the predictions */
  vvas_treenode_traverse (other, IN_ORDER, TRAVERSE_ALL, -1,
      (vvas_treenode_traverse_func) node_assign, NULL);

  GST_INFERENCE_PREDICTION_UNLOCK ((GstInferencePrediction *) self);

  return (GstInferencePrediction *) other->data;
}

static gchar *
bounding_box_to_string (VvasBoundingBox * bbox, gint level)
{
  gint indent = level * 2;

  g_return_val_if_fail (bbox, NULL);

  return g_strdup_printf ("{\n"
      "%*s  x : %d\n"
      "%*s  y : %d\n"
      "%*s  width : %u\n"
      "%*s  height : %u\n"
      "%*s}",
      indent, "", bbox->x,
      indent, "", bbox->y,
      indent, "", bbox->width, indent, "", bbox->height, indent, "");
}

static gchar *
prediction_children_to_string (GstInferencePrediction * self, gint level)
{
  GSList *subpreds = NULL;
  GSList *iter = NULL;
  GString *string = NULL;

  g_return_val_if_fail (self, NULL);

  /* Build the child predictions using a GString */
  string = g_string_new (NULL);

  subpreds = prediction_get_children_unlocked (self);

  for (iter = subpreds; iter != NULL; iter = g_slist_next (iter)) {
    GstInferencePrediction *pred = (GstInferencePrediction *) iter->data;
    gchar *child = prediction_to_string (pred, level + 1);

    g_string_append_printf (string, "%s, ", child);
    g_free (child);
  }

  g_slist_free (subpreds);

  return g_string_free (string, FALSE);
}

static gchar *
prediction_classes_to_string (GstInferencePrediction * self, gint level)
{
  GList *iter = NULL;
  GString *string = NULL;

  g_return_val_if_fail (self, NULL);

  /* Build the classes for this predictions using a GString */
  string = g_string_new (NULL);

  for (iter = (GList *) self->prediction.classifications; iter != NULL;
      iter = (GList *) g_list_next (iter)) {
    GstInferenceClassification *c = (GstInferenceClassification *) iter->data;
    gchar *sclass = gst_inference_classification_to_string (c, level + 1);

    g_string_append_printf (string, "%s, ", sclass);
    g_free (sclass);
  }

  return g_string_free (string, FALSE);
}

static gchar *
prediction_to_string (GstInferencePrediction * self, gint level)
{
  gint indent = level * 2;
  gchar *bbox = NULL;
  gchar *children = NULL;
  gchar *classes = NULL;
  gchar *prediction = NULL;

  g_return_val_if_fail (self, NULL);

  bbox = bounding_box_to_string (&self->prediction.bbox, level + 1);
  classes = prediction_classes_to_string (self, level + 1);
  children = prediction_children_to_string (self, level + 1);

  prediction = g_strdup_printf ("{\n"
      "%*s  id : %" G_GUINT64_FORMAT ",\n"
      "%*s  enabled : %s,\n"
      "%*s  bbox : %s,\n"
      "%*s  track label : %s,\n"
      "%*s  classes : [\n"
      "%*s    %s\n"
      "%*s  ],\n"
      "%*s  predictions : [\n"
      "%*s    %s\n"
      "%*s  ]\n"
      "%*s}",
      indent, "", self->prediction.prediction_id,
      indent, "", self->prediction.enabled ? "True" : "False",
      indent, "", bbox,
      indent, "", self->prediction.obj_track_label,
      indent, "", indent, "", classes, indent, "",
      indent, "", indent, "", children, indent, "", indent, "");

  g_free (bbox);
  g_free (children);
  g_free (classes);

  return prediction;
}

gchar *
gst_inference_prediction_to_string (GstInferencePrediction * self)
{
  gchar *serial = NULL;

  g_return_val_if_fail (self, NULL);

  GST_INFERENCE_PREDICTION_LOCK (self);
  serial = prediction_to_string (self, 0);
  GST_INFERENCE_PREDICTION_UNLOCK (self);

  return serial;
}

static void
node_get_children (VvasTreeNode * node, gpointer data)
{
  GSList **children = (GSList **) data;
  GstInferencePrediction *prediction;

  g_return_if_fail (node);
  g_return_if_fail (children);

  prediction = (GstInferencePrediction *) node->data;

  *children = g_slist_append (*children, prediction);
}

static GSList *
prediction_get_children_unlocked (GstInferencePrediction * self)
{
  GSList *children = NULL;

  g_return_val_if_fail (self, NULL);

  if (self->prediction.node) {
    vvas_treenode_traverse_child (self->prediction.node, TRAVERSE_ALL,
        node_get_children, &children);
  }

  return children;
}

GSList *
gst_inference_prediction_get_children (GstInferencePrediction * self)
{
  GSList *children = NULL;

  GST_INFERENCE_PREDICTION_LOCK (self);
  children = prediction_get_children_unlocked (self);
  GST_INFERENCE_PREDICTION_UNLOCK (self);

  return children;
}

static void
mem_reset (char *ptr, int size)
{
  g_return_if_fail (ptr);

  memset (ptr, 0, size);
}

static void
bounding_box_reset (VvasBoundingBox * bbox)
{
  g_return_if_fail (bbox);

  bbox->x = 0;
  bbox->y = 0;
  bbox->width = 0;
  bbox->height = 0;
  bbox->box_color.red = 0;
  bbox->box_color.green = 0;
  bbox->box_color.blue = 0;
  bbox->box_color.alpha = 0;
}

static void
prediction_reset (GstInferencePrediction * self)
{
  g_return_if_fail (self);

  self->prediction.prediction_id = vvas_inferprediction_get_prediction_id ();
  self->prediction.enabled = TRUE;
  self->prediction.bbox_scaled = FALSE;
  bounding_box_reset (&self->prediction.bbox);
  self->prediction.count = 0;
  self->prediction.model_class = VVAS_XCLASS_NOTFOUND;
  mem_reset ((char *) &self->prediction.pose14pt, sizeof (Pose14Pt));
  mem_reset ((char *) &self->prediction.feature, sizeof (Feature));
  self->prediction.feature.type = UNKNOWN_FEATURE;

  /* Free all children */
  prediction_free (self);

  self->prediction.node = vvas_treenode_new (self);
}

static void
prediction_free (GstInferencePrediction * self)
{
  GSList *children = prediction_get_children_unlocked (self);

  /* Free all children recursively */
  g_slist_free_full (children, (GDestroyNotify) gst_inference_prediction_unref);

  /* Now  free our classifications */
  g_list_free_full ((GList *) self->prediction.classifications,
      (GDestroyNotify) gst_inference_classification_unref);
  self->prediction.classifications = NULL;

  if (self->prediction.node) {
    vvas_treenode_destroy (self->prediction.node);
    self->prediction.node = NULL;
  }

  if (self->sub_buffer != NULL)
    gst_buffer_unref (self->sub_buffer);

  if (self->prediction.segmentation.data != NULL) {
    self->prediction.segmentation.free (&self->prediction.segmentation);
  }
  if (self->prediction.reid.data && self->prediction.reid.free) {
    self->prediction.reid.free (&self->prediction.reid);
  }

  if (self->prediction.tb) {
    self->prediction.tb->free ((void **) &self->prediction.tb);
  }

  if (self->prediction.obj_track_label != NULL)
    g_free (self->prediction.obj_track_label);

  if (self->prediction.model_name)
    g_free (self->prediction.model_name);

  self->sub_buffer = NULL;
  self->prediction.obj_track_label = NULL;
  self->prediction.model_name = NULL;
  self->reserved_1 = NULL;
  self->reserved_2 = NULL;
  self->reserved_3 = NULL;
  self->reserved_4 = NULL;
  self->reserved_5 = NULL;
}

static void
gst_inference_prediction_free (GstInferencePrediction * self)
{
  g_return_if_fail (self);

  prediction_free (self);

  g_mutex_clear (&self->mutex);
  g_slice_free (GstInferencePrediction, self);
}

void
gst_inference_prediction_append_classification (GstInferencePrediction * self,
    GstInferenceClassification * c)
{
  g_return_if_fail (self);
  g_return_if_fail (c);

  GST_INFERENCE_PREDICTION_LOCK (self);
  self->prediction.classifications = (VvasList *)
      g_list_append ((GList *) self->prediction.classifications, c);
  GST_INFERENCE_PREDICTION_UNLOCK (self);
}

static void
compute_factors (GstVideoInfo * from, GstVideoInfo * to, gdouble * hfactor,
    gdouble * vfactor)
{
  gint fw, fh, tw, th;

  g_return_if_fail (from);
  g_return_if_fail (to);
  g_return_if_fail (hfactor);
  g_return_if_fail (vfactor);

  fw = GST_VIDEO_INFO_WIDTH (from);
  tw = GST_VIDEO_INFO_WIDTH (to);
  fh = GST_VIDEO_INFO_HEIGHT (from);
  th = GST_VIDEO_INFO_HEIGHT (to);

  g_return_if_fail (fw);
  g_return_if_fail (fh);

  *hfactor = tw * 1.0 / fw;
  *vfactor = th * 1.0 / fh;
}

static GstInferencePrediction *
prediction_scale (const GstInferencePrediction * self, GstVideoInfo * to,
    GstVideoInfo * from)
{
  GstInferencePrediction *dest = NULL;

  gdouble hfactor, vfactor;

  g_return_val_if_fail (self, NULL);
  g_return_val_if_fail (to, NULL);
  g_return_val_if_fail (from, NULL);

  dest = prediction_copy (self);

  compute_factors (from, to, &hfactor, &vfactor);

  dest->prediction.bbox.x = self->prediction.bbox.x * hfactor;
  dest->prediction.bbox.y = self->prediction.bbox.y * vfactor;

  dest->prediction.bbox.width =
      nearbyintf (self->prediction.bbox.width * hfactor);
  dest->prediction.bbox.height =
      nearbyintf (self->prediction.bbox.height * vfactor);


    if (self->prediction.model_class == VVAS_XCLASS_POSEDETECT) {
    prediction_scale_pose14pt (self, dest, hfactor, vfactor);
  }

  GST_LOG ("scaled bbox: %dx%d@%dx%d -> %dx%d@%dx%d",
      self->prediction.bbox.x, self->prediction.bbox.y,
      self->prediction.bbox.width, self->prediction.bbox.height,
      dest->prediction.bbox.x, dest->prediction.bbox.y,
      dest->prediction.bbox.width, dest->prediction.bbox.height);

  return dest;
}

static void
prediction_scale_ip (GstInferencePrediction * self, GstVideoInfo * to,
    GstVideoInfo * from)
{
  gdouble hfactor, vfactor;

  g_return_if_fail (self);
  g_return_if_fail (to);
  g_return_if_fail (from);

  compute_factors (from, to, &hfactor, &vfactor);

  self->prediction.bbox.x = self->prediction.bbox.x * hfactor;
  self->prediction.bbox.y = self->prediction.bbox.y * vfactor;

  self->prediction.bbox.width = self->prediction.bbox.width * hfactor;
  self->prediction.bbox.height = self->prediction.bbox.height * vfactor;
    if (self->prediction.model_class == VVAS_XCLASS_POSEDETECT) {
    prediction_scale_pose14pt_ip (self, hfactor, vfactor);
  }
}

static gboolean
node_scale_ip (VvasTreeNode * node, gpointer data)
{
  GstInferencePrediction *self = (GstInferencePrediction *) node->data;
  PredictionScaleData *sdata = (PredictionScaleData *) data;

  prediction_scale_ip (self, sdata->to, sdata->from);

  return FALSE;
}

static gpointer
node_scale (gconstpointer node, gpointer data)
{
  const GstInferencePrediction *self = (GstInferencePrediction *) node;
  PredictionScaleData *sdata = (PredictionScaleData *) data;

  return prediction_scale (self, sdata->to, sdata->from);
}

void
gst_inference_prediction_scale_ip (GstInferencePrediction * self,
    GstVideoInfo * to, GstVideoInfo * from)
{
  PredictionScaleData data = {.from = from,.to = to };

  g_return_if_fail (self);
  g_return_if_fail (to);
  g_return_if_fail (from);

  GST_INFERENCE_PREDICTION_LOCK (self);

  vvas_treenode_traverse (self->prediction.node, IN_ORDER, TRAVERSE_ALL, -1,
      (vvas_treenode_traverse_func) node_scale_ip, &data);

  GST_INFERENCE_PREDICTION_UNLOCK (self);
}

GstInferencePrediction *
gst_inference_prediction_scale (GstInferencePrediction * self,
    GstVideoInfo * to, GstVideoInfo * from)
{
  VvasTreeNode *other = NULL;
  PredictionScaleData data = {.from = from,.to = to };

  g_return_val_if_fail (self, NULL);
  g_return_val_if_fail (to, NULL);
  g_return_val_if_fail (from, NULL);

  GST_INFERENCE_PREDICTION_LOCK (self);

  other =
      vvas_treenode_copy_deep (self->prediction.node,
      (vvas_treenode_copy_func) node_scale, &data);

  /* Now finish assigning the nodes to the predictions */
  vvas_treenode_traverse (other, IN_ORDER, TRAVERSE_ALL, -1,
      (vvas_treenode_traverse_func) node_assign, NULL);

  GST_INFERENCE_PREDICTION_UNLOCK (self);

  return (GstInferencePrediction *) other->data;
}

static gboolean
prediction_find (GstInferencePrediction * obj, PredictionFindData * found)
{
  gboolean ret = FALSE;

  g_return_val_if_fail (obj, TRUE);
  g_return_val_if_fail (found, TRUE);

  if (obj->prediction.prediction_id == found->prediction_id) {
    found->prediction = gst_inference_prediction_ref (obj);
    ret = TRUE;
  }

  return ret;
}

static gboolean
node_find (VvasTreeNode * node, gpointer data)
{
  PredictionFindData *found = (PredictionFindData *) data;
  GstInferencePrediction *current = (GstInferencePrediction *) node->data;

  g_return_val_if_fail (found, TRUE);

  return prediction_find (current, found);
}

static GstInferencePrediction *
prediction_find_unlocked (GstInferencePrediction * self, guint64 id)
{
  PredictionFindData found = { 0, id };

  g_return_val_if_fail (self, NULL);

  vvas_treenode_traverse (self->prediction.node, IN_ORDER, TRAVERSE_ALL, -1,
      (vvas_treenode_traverse_func) node_find, &found);

  return found.prediction;
}

GstInferencePrediction *
gst_inference_prediction_find (GstInferencePrediction * self, guint64 id)
{
  GstInferencePrediction *found = NULL;

  g_return_val_if_fail (self, NULL);

  GST_INFERENCE_PREDICTION_LOCK (self);

  found = prediction_find_unlocked (self, id);

  GST_INFERENCE_PREDICTION_UNLOCK (self);

  return found;
}

static gint
classification_compare (gconstpointer a, gconstpointer b)
{
  GstInferenceClassification *ca = (GstInferenceClassification *) a;
  GstInferenceClassification *cb = (GstInferenceClassification *) b;

  return ca->classification.classification_id ==
      cb->classification.classification_id ? 0 : 1;
}

static void
classification_merge (GList * src, GList ** dst)
{
  GList *iter = NULL;

  g_return_if_fail (dst);

  /* For each classification in the src, see if it exists in the dst */
  for (iter = src; iter; iter = g_list_next (iter)) {
    GList *exists =
        g_list_find_custom (*dst, iter->data, classification_compare);

    /* Copy and append it to the dst if it doesn't exist */
    if (!exists) {
      GstInferenceClassification *child =
          gst_inference_classification_copy (iter->data);
      *dst = g_list_append (*dst, child);
    }
  }
}

static gboolean
prediction_merge (GstInferencePrediction * src, GstInferencePrediction * dst)
{
  GSList *src_children = prediction_get_children_unlocked (src);
  GSList *iter = NULL;
  GSList *new_children = NULL;
  gboolean new_added = FALSE;

  g_return_val_if_fail (src, FALSE);
  g_return_val_if_fail (dst, FALSE);
  g_return_val_if_fail (src->prediction.prediction_id ==
      dst->prediction.prediction_id, FALSE);

  /* Two things might've happened:
   * 1) A new class was added
   * 2) A new subprediction was added
   */

  /* Handle 1) here */
  classification_merge ((GList *) src->prediction.classifications,
      (GList **) & dst->prediction.classifications);

  /* Handle 2) here */
  for (iter = src_children; iter; iter = g_slist_next (iter)) {
    GstInferencePrediction *current = (GstInferencePrediction *) iter->data;

    /* TODO: Optimize this by only searching the immediate children */
    GstInferencePrediction *found =
        prediction_find_unlocked (dst, current->prediction.prediction_id);

    /* No matching prediction, save it to append it later */
    if (!found) {
      new_children = g_slist_append (new_children, current);
      continue;
    }

    /* Recurse into the children */
    new_added = prediction_merge (current, found);

    gst_inference_prediction_unref (found);
  }

  /* Finally append all the new children to dst. Do it after all
     children have been processed */
  for (iter = new_children; iter; iter = g_slist_next (iter)) {
    GstInferencePrediction *prediction =
        gst_inference_prediction_copy ((GstInferencePrediction *) iter->data);
    gst_inference_prediction_append (dst, prediction);
  }

  new_added |= new_children ? TRUE : FALSE;

  g_slist_free (src_children);
  g_slist_free_full (new_children,
      (GDestroyNotify) gst_inference_prediction_unref);

  return new_added;
}

gboolean
gst_inference_prediction_merge (GstInferencePrediction * src,
    GstInferencePrediction * dst)
{
  gboolean needs_scale = FALSE;

  g_return_val_if_fail (src, FALSE);
  g_return_val_if_fail (dst, FALSE);

  if (src == dst) {
    return needs_scale;
  }

  GST_INFERENCE_PREDICTION_LOCK (src);
  GST_INFERENCE_PREDICTION_LOCK (dst);

  needs_scale = prediction_merge (src, dst);

  GST_INFERENCE_PREDICTION_UNLOCK (dst);
  GST_INFERENCE_PREDICTION_UNLOCK (src);

  return needs_scale;
}

static void
prediction_get_enabled (GstInferencePrediction * self, GList ** found)
{
  g_return_if_fail (self);
  g_return_if_fail (found);

  if (self->prediction.enabled) {
    *found = g_list_append (*found, self);
  }
}

static gboolean
node_get_enabled (VvasTreeNode * node, gpointer data)
{
  GstInferencePrediction *self = NULL;
  GList **found = (GList **) data;

  g_return_val_if_fail (node, TRUE);
  g_return_val_if_fail (found, TRUE);

  self = (GstInferencePrediction *) node->data;

  prediction_get_enabled (self, found);

  return FALSE;
}

GList *
gst_inference_prediction_get_enabled (GstInferencePrediction * self)
{
  GList *found = NULL;

  g_return_val_if_fail (self, NULL);

  vvas_treenode_traverse (self->prediction.node, IN_ORDER, TRAVERSE_ALL, -1,
      (vvas_treenode_traverse_func) node_get_enabled, &found);

  return found;
}
