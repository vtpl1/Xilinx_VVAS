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
#include <bits/stdc++.h>
#include <cmath>
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

int event_generation_counter = 0, event_generation_counter_spnet = 0;
uint64_t frame_id = 0;
int prev_pub_event_frame_id = 0, prev_pub_event_frame_id_spnet = 0;

// Define the veriables.
typedef enum { INFER_LEVEL_1 = 1, INFER_LEVEL_2 } eInferCasecaseLevel;

using namespace cv;
using namespace std;

#define MAX_CLASS_LEN 1024
#define MAX_LABEL_LEN 1024
#define MAX_ALLOWED_CLASS 20
#define MAX_ALLOWED_LABELS 20

static int vtpl_event_generator(char* label, int x, int y, int w, int h, uint64_t frame_id)
{
  cout << "Fall event published: " << frame_id << endl;
  cout << "Classification = " << label << endl;
  cout << "x = " << x << "; "
       << "y = " << y << "; "
       << "w = " << w << "; "
       << "h = " << h << ";\n";

  return 0;
}

void retfind_rect_from_pose(Pose14Pt pose, float* p)
{
  int right_hip_x = pose.right_hip.x;
  int right_hip_y = pose.right_hip.y;

  int right_ankle_x = pose.right_ankle.x;
  int right_ankle_y = pose.right_ankle.y;
  int left_hip_x = pose.left_hip.x;
  int left_hip_y = pose.left_hip.y;

  int left_ankle_x = pose.left_ankle.x;
  int left_ankle_y = pose.left_ankle.y;

  int neck_x = pose.neck.x;
  int neck_y = pose.neck.y;

  float mid_x = (right_hip_x + left_hip_x) / 2;
  float mid_y = (right_hip_y + left_hip_y) / 2;

  float leg_x = (right_ankle_x + left_ankle_x) / 2;
  float leg_y = (right_ankle_y + left_ankle_y) / 2;

  float slope_upper_body, slope_lower_body;

  if (round(abs(abs(neck_x) - abs(mid_x))) < 1) {
    slope_upper_body = 6;
  } else {
    slope_upper_body = abs(neck_y - mid_y) / abs(neck_x - mid_x);
  }

  if (round(abs(abs(leg_x) - abs(mid_x))) < 1) {
    slope_lower_body = -6;
  } else {
    slope_lower_body = (leg_y - mid_y) / (leg_x - mid_x);
  }

  p[0] = slope_upper_body;
  p[1] = slope_lower_body;
}

extern "C" {
int32_t xlnx_kernel_init(VVASKernel* handle) { return 0; }

uint32_t xlnx_kernel_deinit(VVASKernel* handle) { return 0; }

uint32_t xlnx_kernel_start(VVASKernel* handle, int start, VVASFrame* input[MAX_NUM_OBJECT],
                           VVASFrame* output[MAX_NUM_OBJECT])
{
  GstInferenceMeta* infer_meta = NULL;
  GstInferencePrediction *root, *child, *child_sp_net;
  GstInferenceClassification* classification;
  GSList *child_predictions, *child_predictions_sp_net, *pred_head_ptr;
  GList* classes;
  float aspcet_ratio = 0.0;

  eInferCasecaseLevel eInferlevel = INFER_LEVEL_1;

  guint x = 0, y = 0, w = 0, h = 0;
  int event_generation_th = 10;
  int inter_event_generation_th = 20;
  bool is_event = false;
  float a[2];
  float* slope = (float*)&a;
  bool flag = false, PersonNoCollapseLevel_1 = false;
  bool PersonCollapseLevel_2 = false;

  infer_meta =
      ((GstInferenceMeta*)gst_buffer_get_meta((GstBuffer*)input[0]->app_priv, gst_inference_meta_api_get_type()));

  /*
   * Note: IN JSON FILE NAME SHOULD BE GIVEN AS "PF_LEVEL_1"
   *       For Level1 xfilter
   */
  if ((handle) && (!strcmp((char*)handle->name, "libkrnl_PF_LEVEL_1"))) {
    ++frame_id;
  } else if (!strcmp((char*)handle->name, "libkrnl_PF_LEVEL_2")) {
    eInferlevel = INFER_LEVEL_2;
  } else {
    GST_WARNING("Invalid Infer Level");
  }

  if (infer_meta) {
    root = infer_meta->prediction;
    pred_head_ptr = gst_inference_prediction_get_children(root);

    // Checking multiple consicutive frames fall detection based on detector resutls
    for (child_predictions = pred_head_ptr; child_predictions; child_predictions = g_slist_next(child_predictions)) {
      child = (GstInferencePrediction*)child_predictions->data;

      x = child->prediction.bbox.x;
      y = child->prediction.bbox.y;
      w = child->prediction.bbox.width;
      h = child->prediction.bbox.height;

      aspcet_ratio = w / h;

      for (classes = (GList*)child->prediction.classifications; classes; classes = g_list_next(classes)) {
        classification = (GstInferenceClassification*)classes->data;
        if (aspcet_ratio <= 0.7) {
          free(classification->classification.class_label);
          classification->classification.class_label = strdup("Person/NoCollapse");
        }

        /* check for Level 1 if the frame is "NO COLLAPSE FRAME"*/
        if ((eInferlevel == INFER_LEVEL_1) && (aspcet_ratio <= 0.7)) {
          PersonNoCollapseLevel_1 = true;
        }
        // label = classification->classification.class_label;
      }

      if ((eInferlevel == INFER_LEVEL_1) && (true == PersonNoCollapseLevel_1)) {
        /* At Level 1 , there is no COLLAPSE DETECTED in entire frame
         * Decrementing Event Generation Counter
         */
        event_generation_counter--;
        // cout << "No COLLAPSE FRAME level 1 " << event_generation_counter << endl;

        if (event_generation_counter < 0) {
          event_generation_counter = 0;
        }
      }

      /* On each children, iterate through the different associated classes */
      if (aspcet_ratio > 0.7) {
        /* ENABLE SECOND LEVEL INFERENCE */
        // Detector detecting person collapsed -- Calling Pose estimation.
        child_sp_net = NULL;

        for (child_predictions_sp_net = gst_inference_prediction_get_children(child); child_predictions_sp_net;
             child_predictions_sp_net = g_slist_next(child_predictions_sp_net)) {
          child_sp_net = (GstInferencePrediction*)child_predictions_sp_net->data;
          Pose14Pt pose = child_sp_net->prediction.pose14pt;

          retfind_rect_from_pose(pose, slope);
          if ((slope[0] > 0) && (slope[0] < 0.5)) {
            flag = true;
            PersonCollapseLevel_2 = true;
          } else {
            flag = false;
          }

          for (classes = (GList*)child->prediction.classifications; classes; classes = g_list_next(classes)) {
            classification = (GstInferenceClassification*)classes->data;

            if (flag) {
              free(classification->classification.class_label);
              classification->classification.class_label = strdup("Person/Collapse");
              // label = classification->classification.class_label;
            } else {
              free(classification->classification.class_label);
              classification->classification.class_label = strdup("Person/NoCollapse");
              // label = classification->classification.class_label;
              // flag = false;
            }
          }
        } // end of for loop

        if ((eInferlevel == INFER_LEVEL_2) && (true == PersonCollapseLevel_2)) {

          /* In Frame we detected atleast one collapse so frame is
           * considered as "COLLAPSE FRAME" Incrementing Event Generation counter
           */
          event_generation_counter++;
          // cout << "COLLAPSE FRAME level 2 " << event_generation_counter << endl;
        } else if ((eInferlevel == INFER_LEVEL_2) && (false == PersonCollapseLevel_2)) {

          /* In Frame we detected atleast one collapse so frame is
           * considered as "NO COLLAPSE FRAME" Decrementing Event Generation counter
           */
          event_generation_counter--;
          // cout << "No COLLAPSE FRAME " << event_generation_counter << endl;

          if (event_generation_counter < 0) {
            event_generation_counter = 0;
          }
        }
      } else {
        child->prediction.enabled = false;
      }
    } /* Infer level 1 meta data loop */
  }

  if (INFER_LEVEL_2 == eInferlevel) {

    // if (event_generation_counter < event_generation_th)
    // {
    //   event_generation_counter++;
    // }
    // cout << "Event publish " << event_generation_counter << endl;

    if (event_generation_counter >= event_generation_th) {
      is_event = true;
      event_generation_counter = 0;
    }

    if (is_event) {
      /* Process inference for next stage */
      int event_gap = frame_id - prev_pub_event_frame_id;

      // cout << "Event gap " << event_gap << endl;

      if (event_gap >= inter_event_generation_th) {
        prev_pub_event_frame_id = frame_id;
        char* label = strdup("Person/Collapse");
        cout << "---------------------------------------" << endl;
        cout << "Event Gap: [" << event_gap << "/" << inter_event_generation_th << "]" << endl;
        vtpl_event_generator(label, x, y, w, h, frame_id);
        cout << "---------------------------------------" << endl;
      }
    }
  }

  return 0;
}

int32_t xlnx_kernel_done(VVASKernel* handle) { return 0; }
}
