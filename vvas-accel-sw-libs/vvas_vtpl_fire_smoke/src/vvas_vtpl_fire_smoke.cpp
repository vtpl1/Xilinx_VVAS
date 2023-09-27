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

#include "vms_live_event_sender.h"
#include <chrono>
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

// #define SHOW_MSG true
bool SHOW_MSG = false;

typedef struct _VtplDataStructure {
  VmsLiveEventSender o_vms_live_event_sender;
  int ch_id;
  uint64_t frame_id = 0;
  int fire_event_generation_counter = 0;
  int fire_inter_event_generation_gap_counter = 0;
  // TODO -- DSD -- change the following threshold values based on the
  // original logics of DeeperLook
  int fire_event_generation_frame_number_th = 10;           // #frame
  int fire_inter_event_generation_gap_frame_number_th = 30; // #frame

  int smoke_event_generation_counter = 0;
  int smoke_inter_event_generation_gap_counter = 0;
  // TODO -- DSD -- change the following threshold values based on the
  // original logics of DeeperLook
  int smoke_event_generation_frame_number_th = 10;
  int smoke_inter_event_generation_gap_frame_number_th = 30;

  bool is_first_fire_event = true;
  bool is_first_smoke_event = true;

} VtplDataStructure;

static int vtpl_event_generator(VVASFrame* input, char* label, float confidence,
                                int x, int y, int w, int h)
{
  // cout << " ---------------------------------------" << endl;
  bool is_smoke_event = false;
  bool is_fire_event = false;

  return 0;
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
  VtplDataStructure* p_vtpl_data_structure =
      (VtplDataStructure*)handle->kernel_priv;

  p_vtpl_data_structure->frame_id++;

  uint32_t img_height = input[0]->props.height;
  uint32_t img_width = input[0]->props.width;
  uint32_t stride = input[0]->props.stride;
  uint8_t* in_plane1 = (uint8_t*)input[0]->vaddr[0];
  VVASVideoFormat fmt = input[0]->props.fmt;

  // std::cout << "width: " << img_width << " height: " << img_height
  //           << " fmt: " << fmt << " n_planes: " << input[0]->n_planes
  //           << " stride: " << input[0]->props.stride << std::endl;

  cv::Mat mat_dst(cv::Size(img_width, img_height), CV_8UC3, in_plane1);
  cv::cvtColor(mat_dst, mat_dst, cv::COLOR_BGR2RGB);
  cv::Mat img(mat_dst);
  // std::cout<<"+++++++++++Frame_id :: " << p_vtpl_data_structure->frame_id <<
  // std::endl; std::stringstream ss; ss << "yuv_" <<
  // p_vtpl_data_structure->frame_id; ss << ".jpeg";
  // cv::imwrite(ss.str().c_str(), mat_dst);

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

  if (infer_meta) {
    root = infer_meta->prediction;
    pred_head_ptr = gst_inference_prediction_get_children(root);

    /* Iterate through the immediate child predictions */
    // for each detected object
    for (child_predictions = pred_head_ptr; child_predictions;
         child_predictions = g_slist_next(child_predictions)) {
      child = (GstInferencePrediction*)child_predictions->data;
      //  std::cout<< "Project" << std::endl;
      /* On each children, iterate through the different associated classes */
      // for each class of object
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

        // vtpl_event_generator(input[0], label, confidence, x, y, w, h);
        // std::stringstream ss;
        // ss << "FS_Detected" << p_vtpl_data_structure->frame_id;
        // ss << ".jpeg";
        // cv::Point p1(x, y);
        // cv::Point p2(x + w, y + h);
        // int thickness = 2;
        // // cv::rectangle(img, p1, p2, cv::Scalar(0, 240, 0), thickness,
        // LINE_8,
        // // 0); cv::imwrite(ss.str().c_str(), img); std::cout << "Hello from
        // fire" << std::endl; cv::imshow("frame", img); cv::waitKey(1);
        if (strcmp(label, "fire") == 0) {
          fe_x = x;
          fe_y = y;
          fe_w = w;
          fe_h = h;
          fe_conf = confidence;
          flabel = label;
          is_fire_event = true;
          // break;
        }
        if (strcmp(label, "smoke") == 0) {
          se_x = x;
          se_y = y;
          se_w = w;
          se_h = h;
          se_conf = confidence;
          slabel = label;
          is_smoke_event = true;
          // break;
        }
        // TODO -- DSD -- include multi-fire/smoke objects event generation
        // logic
        if (is_fire_event || is_smoke_event) {
          break;
        }
      }
      if (is_fire_event || is_smoke_event) {
        break;
      }
    }
  }

  // is_fire_event = true;

  if (is_fire_event) {
    p_vtpl_data_structure->fire_event_generation_counter++;
  }

  bool is_fire_event_1 = false;

  if (p_vtpl_data_structure->fire_event_generation_counter >=
      p_vtpl_data_structure->fire_event_generation_frame_number_th) {
    is_fire_event_1 = true;
  }

  if (!is_fire_event) {
    if (p_vtpl_data_structure->fire_event_generation_counter > 0) {
      p_vtpl_data_structure->fire_event_generation_counter--;
    }
  }
  bool is_final_fire_event = false;
  if (is_fire_event_1 && p_vtpl_data_structure->is_first_fire_event) {
    std::cout << "**********************Publish first fire event" << std::endl;
    // p_vtpl_data_structure->fire_event_generation_counter = 0;
    p_vtpl_data_structure->is_first_fire_event = false;
    is_final_fire_event = true;
  }
  if (!p_vtpl_data_structure->is_first_fire_event) {

    p_vtpl_data_structure->fire_inter_event_generation_gap_counter++;
    if (p_vtpl_data_structure->fire_inter_event_generation_gap_counter ==
        p_vtpl_data_structure
            ->fire_inter_event_generation_gap_frame_number_th) {
      if (is_fire_event) {
        std::cout << "+++++++++++++++++++++++++++++Next fire event"
                  << std::endl;
        is_final_fire_event = true;
      }
      p_vtpl_data_structure->fire_inter_event_generation_gap_counter = 0;
    }
  }

  // if (p_vtpl_data_structure->frame_id >= 100) {
  //   // return 0;
  //   exit(0);
  // }
  // is_smoke_event = true;

  if (is_smoke_event) {
    p_vtpl_data_structure->smoke_event_generation_counter++;
  }
  bool is_smoke_event_1 = false;

  if (p_vtpl_data_structure->smoke_event_generation_counter >=
      p_vtpl_data_structure->smoke_event_generation_frame_number_th) {
    is_smoke_event_1 = true;
  }
  if (!is_smoke_event) {
    if (p_vtpl_data_structure->smoke_event_generation_counter > 0) {
      p_vtpl_data_structure->smoke_event_generation_counter--;
    }
  }
  bool is_final_smoke_event = false;
  if (is_smoke_event_1 && p_vtpl_data_structure->is_first_smoke_event) {
    std::cout << "**********************Publish first smoke event" << std::endl;
    // p_vtpl_data_structure->smoke_event_generation_counter = 0;
    p_vtpl_data_structure->is_first_smoke_event = false;
    is_final_smoke_event = true;
  }
  if (!p_vtpl_data_structure->is_first_smoke_event) {

    p_vtpl_data_structure->smoke_inter_event_generation_gap_counter++;
    if (p_vtpl_data_structure->smoke_inter_event_generation_gap_counter ==
        p_vtpl_data_structure
            ->smoke_inter_event_generation_gap_frame_number_th) {
      if (is_smoke_event) {
        std::cout << "+++++++++++++++++++++++++++++Next smoke event"
                  << std::endl;
        is_final_smoke_event = true;
      }
      p_vtpl_data_structure->smoke_inter_event_generation_gap_counter = 0;
    }
  }

  if (SHOW_MSG) {
    std::cout << "-------[" << p_vtpl_data_structure->frame_id << "];F["
              << p_vtpl_data_structure->fire_event_generation_counter << "/10]["
              << p_vtpl_data_structure->fire_inter_event_generation_gap_counter
              << "/30];S["
              << p_vtpl_data_structure->smoke_event_generation_counter
              << "/10]["
              << p_vtpl_data_structure->smoke_inter_event_generation_gap_counter
              << "/30]" << std::endl;
  }

  unsigned long milliseconds_since_epoch =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count();

  if (is_final_fire_event) {
    std::cout << "*******publishing fire event with fe_x: " << fe_x << " fe_y "
              << fe_y << std::endl;
    cv::Point p1(fe_x, fe_y);
    cv::Point p2(fe_x + fe_w, fe_y + fe_h);
    int thickness = 2;
    cv::rectangle(img, p1, p2, cv::Scalar(0, 240, 0), thickness, LINE_8, 0);
    p_vtpl_data_structure->o_vms_live_event_sender.sendEventFromEncodedMat(
        img, 0, 0, 0, 0, "", "", "", 242, "", milliseconds_since_epoch, "", 0,
        0, 48);
  }

  if (is_final_smoke_event) {
    std::cout << "*******publishing smoke event with se_x: " << se_x << " se_y "
              << se_y << std::endl;
    cv::Point p1(se_x, se_y);
    cv::Point p2(se_x + se_w, se_y + se_h);
    int thickness = 2;
    cv::rectangle(img, p1, p2, cv::Scalar(0, 240, 0), thickness, LINE_8, 0);
    p_vtpl_data_structure->o_vms_live_event_sender.sendEventFromEncodedMat(
        img, 0, 0, 0, 0, "", "", "", 243, "", milliseconds_since_epoch, "", 0,
        0, 48);
  }
  // if (fe_w > 0)
  //   vtpl_event_generator(input[0], flabel, fe_conf, fe_x, fe_y, fe_w, fe_h);
  // if (se_w > 0)
  //   vtpl_event_generator(input[0], slabel, se_conf, se_x, se_y, se_w, se_h);

  return 0;
}

int32_t xlnx_kernel_done(VVASKernel* handle) { return 0; }
}
