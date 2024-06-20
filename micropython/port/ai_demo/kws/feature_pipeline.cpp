// Copyright (c) 2017 Personal (Binbin Zhang)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "feature_pipeline.h"

#include <algorithm>
#include <utility>

namespace wenet {

FeaturePipeline::FeaturePipeline()
    : feature_dim_(40),
      fbank_(40, 16000, 400,
             160),
      num_frames_(0),
      input_finished_(false) {
        std::vector<float> remain_wav_init(320, 0.0f);
        remained_wav_ = remain_wav_init;
      }

void FeaturePipeline::AcceptWaveform(const std::vector<float>& wav) {
  std::vector<std::vector<float>> feats;
  std::vector<float> waves;
  waves.insert(waves.end(), remained_wav_.begin(), remained_wav_.end());
  waves.insert(waves.end(), wav.begin(), wav.end());
  int num_frames = fbank_.Compute(waves, &feats);

  // std::cout << "==========num_frame_per_chunk:" << num_frames << "===========" << std::endl;


  for (size_t i = 0; i < feats.size(); ++i) {
    feature_queue_.Push(std::move(feats[i]));
  }
  num_frames_ += num_frames;

  // int left_samples = waves.size() - config_.frame_shift * num_frames;
  int left_samples = waves.size() - 160 * num_frames;

  // std::cout << "==========left_samples_per_chunk:" << left_samples << "===========" << std::endl;


  remained_wav_.resize(left_samples);
  // std::copy(waves.begin() + config_.frame_shift * num_frames, waves.end(),
  //           remained_wav_.begin());
  std::copy(waves.begin() + 160 * num_frames, waves.end(),
            remained_wav_.begin());

  // We are still adding wave, notify input is not finished
  finish_condition_.notify_one();
}

void FeaturePipeline::AcceptWaveform(const std::vector<int16_t>& wav) {
  std::vector<float> float_wav(wav.size());
  for (size_t i = 0; i < wav.size(); i++) {
    float_wav[i] = static_cast<float>(wav[i]);
  }
  this->AcceptWaveform(float_wav);
}

void FeaturePipeline::set_input_finished() {
  CHECK(!input_finished_);
  {
    std::lock_guard<std::mutex> lock(mutex_);
    input_finished_ = true;
  }
  finish_condition_.notify_one();
}

bool FeaturePipeline::ReadOne(std::vector<float>* feat) {
  if (!feature_queue_.Empty()) {
    *feat = std::move(feature_queue_.Pop());
    return true;
  } else {
    std::unique_lock<std::mutex> lock(mutex_);
    while (!input_finished_) {
      // This will release the lock and wait for notify_one()
      // from AcceptWaveform() or set_input_finished()
      finish_condition_.wait(lock);
      if (!feature_queue_.Empty()) {
        *feat = std::move(feature_queue_.Pop());
        return true;
      }
    }
    CHECK(input_finished_);
    // Double check queue.empty, see issue#893 for detailed discussions.
    if (!feature_queue_.Empty()) {
      *feat = std::move(feature_queue_.Pop());
      return true;
    } else {
      return false;
    }
  }
}

bool FeaturePipeline::Read(int num_frames,
                           std::vector<std::vector<float>>* feats) {
  feats->clear();
  std::vector<float> feat;
  while (feats->size() < num_frames) {
    if (ReadOne(&feat)) {
      feats->push_back(std::move(feat));
    } else {
      return false;
    }
  }
  return true;
}

void FeaturePipeline::Reset() {
  input_finished_ = false;
  num_frames_ = 0;
  remained_wav_.clear();
  feature_queue_.Clear();
}

}  // namespace wenet
