/*
 * Copyright 2020-2022 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstdlib>
#include <fstream>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/vvas/gstinferencemeta.h>
#include <iostream>
#include <math.h>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <stdlib.h>
#include <string.h>
#include <vvas/vvas_kernel.h>

// int log_level = LOG_LEVEL_WARNING;

using namespace cv;
using namespace std;

#define MAX_CLASS_LEN 1024
#define MAX_LABEL_LEN 1024
#define MAX_ALLOWED_CLASS 20
#define MAX_ALLOWED_LABELS 20

uint64_t frame_id = 0;
int event_generation_counter = 0;

static int vtpl_event_generator(VVASFrame* input, char* label, float confidence,
                                int x, int y, int w, int h)
{
  cout << "---------------------------------------" << endl;
  // bool is_smoke_event = false;
  // bool is_fire_event = false;

  cout << "Classification = " << label << endl;
  cout << "x = " << x << "; "
       << "y = " << y << "; "
       << "w = " << w << "; "
       << "h = " << h << ";\n";

  return 0;
}

extern "C" {
int32_t xlnx_kernel_init(VVASKernel* handle) { return 0; }

uint32_t xlnx_kernel_deinit(VVASKernel* handle) { return 0; }

uint32_t xlnx_kernel_start(VVASKernel* handle, int start,
                           VVASFrame* input[MAX_NUM_OBJECT],
                           VVASFrame* output[MAX_NUM_OBJECT])
{
  GstInferenceMeta* infer_meta = NULL;
  GstInferencePrediction *root, *child;
  GstInferenceClassification* classification;
  GSList *child_predictions, *pred_head_ptr;
  GList* classes;

  bool is_fire_event = false;
  bool is_smoke_event = false;
  guint fe_x = 0, fe_y = 0, fe_w = 0, fe_h = 0;
  float fe_conf;
  char* flabel;

  guint se_x = 0, se_y = 0, se_w = 0, se_h = 0;
  float se_conf;
  char* slabel;

  infer_meta = ((GstInferenceMeta*)gst_buffer_get_meta(
      (GstBuffer*)input[0]->app_priv, gst_inference_meta_api_get_type()));

  if ((handle) && (!strcmp((char*)handle->name, "libkrnl_PF_LEVEL_1"))) {
    ++frame_id;
    // cout<<"******FrameID:"<<frame_id<<endl;
  }

  if (infer_meta) {
    root = infer_meta->prediction;
    pred_head_ptr = gst_inference_prediction_get_children(root);

    /* Iterate through the immediate child predictions */
    for (child_predictions = pred_head_ptr; child_predictions;
         child_predictions = g_slist_next(child_predictions)) {
      child = (GstInferencePrediction*)child_predictions->data;

      /* On each children, iterate through the different associated classes */
      for (classes = (GList*)child->prediction.classifications; classes;
           classes = g_list_next(classes)) {
        guint x, y, w, h;
        classification = (GstInferenceClassification*)classes->data;

        char* label = classification->classification.class_label;
        float confidence = classification->classification.class_prob;

        x = child->prediction.bbox.x;
        y = child->prediction.bbox.y;
        w = child->prediction.bbox.width;
        h = child->prediction.bbox.height;

        vtpl_event_generator(input[0], label, confidence, x, y, w, h);

        if (strcmp(label, "fire") == 0) {
          fe_x = x;
          fe_y = y;
          fe_w = w;
          fe_h = h;
          fe_conf = confidence;
          flabel = label;
          is_fire_event = true;
          break;
        }
        if (strcmp(label, "smoke") == 0) {
          se_x = x;
          se_y = y;
          se_w = w;
          se_h = h;
          se_conf = confidence;
          slabel = label;
          is_smoke_event = true;
          break;
        }
      }
    }
  }

  if (!is_fire_event)
    if (event_generation_counter > 0)
      event_generation_counter--;

  if (!is_smoke_event)
    if (event_generation_counter > 0)
      event_generation_counter--;

  if (fe_w > 0)
    vtpl_event_generator(input[0], flabel, fe_conf, fe_x, fe_y, fe_w, fe_h);
  if (se_w > 0)
    vtpl_event_generator(input[0], slabel, se_conf, se_x, se_y, se_w, se_h);

  return 0;
}

int32_t xlnx_kernel_done(VVASKernel* handle) { return 0; }
}
