#include "feature_pipeline.h"
#include "aidemo_wrap.h"
#include <iostream>

// feature_pipelien class
struct feature_pipeline
{
    wenet::FeaturePipeline *feature_pipe;
};

struct processed_feat
{
    const float *feats_ptr;
    size_t feats_length;
};

feature_pipeline *feature_pipeline_create()
{
    feature_pipeline *fp = new feature_pipeline;
    fp->feature_pipe = new wenet::FeaturePipeline();
    return fp;
}

void release_final_feats(float* feats)
{
    if (feats != nullptr)
    {
        delete[] feats;
    }
}

void release_preprocess_class(feature_pipeline *fp)
{
    delete fp;
}


void wav_preprocess(feature_pipeline *fp, float *wav, size_t wav_length, float* final_feats)
{
    // 将数组输入转为vector适配函数输入
    std::vector<float> wav_vector(wav, wav + wav_length);

    // 预处理函数
    fp->feature_pipe->AcceptWaveform(wav_vector);

    std::vector<std::vector<float>> feats;
    bool ok = fp->feature_pipe->Read(30, &feats);
    
    // 将vector转换为数组形式，因为C返回不了vector
    std::vector<float> flattened_feats;
    for (const auto& inner_vector : feats) 
    {
        flattened_feats.insert(flattened_feats.end(), inner_vector.begin(), inner_vector.end());
    }

    std::copy(flattened_feats.begin(), flattened_feats.end(), final_feats);
}