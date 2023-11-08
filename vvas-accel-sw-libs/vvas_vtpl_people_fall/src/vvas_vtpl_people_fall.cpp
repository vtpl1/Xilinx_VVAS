/*
 * Copyright 2020-2022 Xilinx, Inc.
 * Copyright 2023 Videonetics Technology Pvt Ltd
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
#include "include/rapidjson/document.h"
#include "include/rapidjson/filereadstream.h"
#include "vms_live_event_sender.h"
#include <bits/stdc++.h>
#include <chrono>
#include <cmath>
#include <cstdlib>
// #include <fmt/format.h>
#include <fstream>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/vvas/gstinferencemeta.h>
#include <iostream>
#include <job.h>
#include <math.h>
#include <nng/protocol/pair0/pair.h>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <stdlib.h>
#include <string.h>
#include <vvas/vvas_kernel.h>

// Define the veriables
typedef enum { INFER_LEVEL_1 = 1, INFER_LEVEL_2 } eInferCasecaseLevel;

using namespace cv;
using namespace std;

#define MAX_CLASS_LEN 1024
#define MAX_LABEL_LEN 1024
#define MAX_ALLOWED_CLASS 20
#define MAX_ALLOWED_LABELS 20

typedef struct _VtplDataStructure {
  VmsLiveEventSender o_vms_live_event_sender;
  int ch_id;
  int person_collapse_event_generation_counter = 0,
      person_collapse_event_generation_counter_spnet = 0;
  uint64_t frame_id = 0;
  uint64_t event_id = 0;
  uint64_t first_level_counter = 0;
  int person_collapse_inter_event_generation_counter = 0;
  int prev_pub_event_frame_id = 0, prev_pub_event_frame_id_spnet = 0;

  int person_collapse_event_generation_frame_number_th = 12; // #frame
  int person_collapse_inter_event_generation_gap_frame_number_th = 120;
  bool is_first_person_collapse_event = true;
} VtplDataStructure;

static int vtpl_event_generator(char* label, int x, int y, int w, int h,
                                uint64_t frame_id)
{
  cout << " Fall event published: " << frame_id << endl;
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
int32_t xlnx_kernel_init(VVASKernel* handle)
{
  VtplDataStructure* p_vtpl_data_structure = new VtplDataStructure();
  handle->kernel_priv = p_vtpl_data_structure;
  return 0;
}

uint32_t xlnx_kernel_deinit(VVASKernel* handle)
{
  if (handle->kernel_priv == nullptr) {
    return 0;
  }
  delete ((VtplDataStructure*)handle->kernel_priv);
  handle->kernel_priv = nullptr;
  return 0;
}

uint32_t xlnx_kernel_start(VVASKernel* handle, int start,
                           VVASFrame* input[MAX_NUM_OBJECT],
                           VVASFrame* output[MAX_NUM_OBJECT])
{

  json_t* jconfig = handle->kernel_config;
  json_t* val_app_id = json_object_get(jconfig, "app-id");
  int app_id = 0;
  if (!val_app_id || !json_is_integer(val_app_id)) {
    std::cout << "::NO Value received PC " << std::endl;
  } else {
    app_id = json_integer_value(val_app_id);
  }

  json_t* val_channel_id = json_object_get(jconfig, "channel-id");
  int channel_id = 0;
  if (!val_channel_id || !json_is_integer(val_channel_id)) {
    std::cout << "::NO Value received PC " << std::endl;
  } else {
    channel_id = json_integer_value(val_channel_id);
  }
  json_t* val_filter_id = json_object_get(jconfig, "filter-id");
  int filter_id = 0;
  if (!val_filter_id || !json_is_integer(val_filter_id)) {
  } else {
    filter_id = json_integer_value(val_filter_id);
  }

  GstInferencePrediction *root, *child, *child_sp_net;
  GstInferenceClassification* classification;
  GSList *child_predictions, *child_predictions_sp_net, *pred_head_ptr;
  GList* classes;

  uint32_t img_height = input[0]->props.height;
  uint32_t img_width = input[0]->props.width;
  uint32_t stride = input[0]->props.stride;
  uint8_t* in_plane1 = (uint8_t*)input[0]->vaddr[0];
  VVASVideoFormat fmt = input[0]->props.fmt;

  cv::Mat mat_dst(cv::Size(img_width, img_height), CV_8UC3, in_plane1);
  cv::cvtColor(mat_dst, mat_dst, cv::COLOR_BGR2RGB);
  cv::Mat img(mat_dst);

  float aspcet_ratio = 1.0;
  float aspect_ratio_th = 0.7;

  eInferCasecaseLevel eInferlevel = INFER_LEVEL_1;
  guint x = 0, y = 0, w = 0, h = 0;
  float a[2];
  float* slope = (float*)&a;
  bool flag = false, PersonNoCollapseLevel_1 = false;
  bool PersonCollapseLevel_2 = false;
  char* pflabel;
  bool is_level_1_inferencer_Called = false;
  GstInferenceMeta* infer_meta = ((GstInferenceMeta*)gst_buffer_get_meta(
      (GstBuffer*)input[0]->app_priv, gst_inference_meta_api_get_type()));
  VtplDataStructure* p_vtpl_data_structure =
      (VtplDataStructure*)handle->kernel_priv;
  if (1 == filter_id) {
    p_vtpl_data_structure->frame_id++;
    is_level_1_inferencer_Called = true;
  } else if (2 == filter_id) {

    eInferlevel = INFER_LEVEL_2;
    // We are ready to publish now:

  } else {
    std::cout << "Unexpected filter id " << std::endl;
  }
  bool is_person_collapsed_event = false;
  if (is_level_1_inferencer_Called) {

    root = infer_meta->prediction;
    pred_head_ptr = gst_inference_prediction_get_children(root);

    // Checking multiple consicutive frames fall detection based on detector
    // resutls
    for (child_predictions = pred_head_ptr; child_predictions;
         child_predictions = g_slist_next(child_predictions)) {
      child = (GstInferencePrediction*)child_predictions->data;

      x = child->prediction.bbox.x;
      y = child->prediction.bbox.y;
      w = child->prediction.bbox.width;
      h = child->prediction.bbox.height;

      aspcet_ratio = w / h;

      for (classes = (GList*)child->prediction.classifications; classes;
           classes = g_list_next(classes)) {
        classification = (GstInferenceClassification*)classes->data;
        float confidence = classification->classification.class_prob;

        if (aspcet_ratio >= aspect_ratio_th) {
          is_person_collapsed_event = true;
          break;
        }
      }
      if (is_person_collapsed_event) {
        break;
      }
    }
    if (is_person_collapsed_event) {
      p_vtpl_data_structure->person_collapse_event_generation_counter++;
    }
    bool is_pc_event_1 = false;
    if (p_vtpl_data_structure->person_collapse_event_generation_counter >=
        p_vtpl_data_structure
            ->person_collapse_event_generation_frame_number_th) {
      is_pc_event_1 = true;
    }
    if (!is_person_collapsed_event) {
      if (p_vtpl_data_structure->person_collapse_event_generation_counter > 0) {
        p_vtpl_data_structure->person_collapse_event_generation_counter--;
      }
    }
    bool is_final_pc_event = false;
    if (is_pc_event_1 &&
        p_vtpl_data_structure->is_first_person_collapse_event) {
      std::cout << "**********************Publish first PC event" << std::endl;
      p_vtpl_data_structure->is_first_person_collapse_event = false;
      is_final_pc_event = true;
    }

    if (!p_vtpl_data_structure->is_first_person_collapse_event) {

      p_vtpl_data_structure->person_collapse_inter_event_generation_counter++;
      if (p_vtpl_data_structure
              ->person_collapse_inter_event_generation_counter ==
          p_vtpl_data_structure
              ->person_collapse_inter_event_generation_gap_frame_number_th) {
        if (is_person_collapsed_event) {
          std::cout << "+++++++++++++++++++++++++++++Next PC event"
                    << std::endl;
          is_final_pc_event = true;
        }
        p_vtpl_data_structure->person_collapse_inter_event_generation_counter =
            0;
      }
    }
    unsigned long milliseconds_since_epoch =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
    if (is_final_pc_event) {
      // std::cout << "*******publishing PC event with se_x: " << x << " se_y "
      //           << y << " Frame_id: " << p_vtpl_data_structure->frame_id
      //           << std::endl;
      cv::Point p1(x, y);
      cv::Point p2(x + w, y + h);
      int thickness = 2;
      // cv::rectangle(img, p1, p2, cv::Scalar(0, 240, 0), thickness, LINE_8,
      // 0);
      // p_vtpl_data_structure->o_vms_live_event_sender.sendEventFromEncodedMat(
      //     img, 0, 0, 0, 0, "", "", "",246, "", milliseconds_since_epoch, "",
      //     0, 0, 52);
      std::stringstream ss;
      ss << "PF_Detected" << p_vtpl_data_structure->frame_id;
      ss << ".jpeg";
      // cv::rectangle(img, p1, p2, cv::Scalar(0, 240, 0), thickness, LINE_8,
      // 0); cv::imwrite(ss.str().c_str(), img);
      cv::rectangle(img, p1, p2, cv::Scalar(0, 240, 0), thickness, LINE_8, 0);
      p_vtpl_data_structure->o_vms_live_event_sender.sendEventFromEncodedMat(
          img, 0, 0, 0, 0, "", "", "", app_id, "", milliseconds_since_epoch, "",
          0, 0, channel_id);
    }
  }
  return 0;
}
int32_t xlnx_kernel_done(VVASKernel* handle) { return 0; }
}
