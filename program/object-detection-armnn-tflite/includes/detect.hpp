/*
 * Copyright (c) 2018 cTuning foundation.
 * See CK COPYRIGHT.txt for copyright details.
 *
 * SPDX-License-Identifier: BSD-3-Clause.
 * See CK LICENSE.txt for licensing details.
 */

#ifndef DETECT_H
#define DETECT_H

#include <iomanip>
#include <vector>
#include <iterator>

#include "armnn/ArmNN.hpp"
#include "armnn/Exceptions.hpp"
#include "armnn/Tensor.hpp"
#include "armnn/INetwork.hpp"
#include "armnnTfLiteParser/ITfLiteParser.hpp"

#include "settings.h"
#include "non_max_suppression.h"
#include "benchmark.h"

using namespace std;
using namespace CK;

template <typename TData, typename TInConverter, typename TOutConverter>
class ArmNNBenchmark : public Benchmark<TData, TInConverter, TOutConverter> {
public:
    ArmNNBenchmark(Settings* settings,
                   TData *in_ptr,
                   TData *boxes_ptr,
                   TData *scores_ptr
                   )
            : Benchmark<TData, TInConverter, TOutConverter>(settings, in_ptr, boxes_ptr, scores_ptr) {
    }
};

armnn::InputTensors MakeInputTensors(const std::pair<armnn::LayerBindingId,
        armnn::TensorInfo>& input, const void* inputTensorData)
{
    return { {input.first, armnn::ConstTensor(input.second, inputTensorData) } };
}

armnn::OutputTensors MakeOutputTensors(const std::pair<armnn::LayerBindingId,
        armnn::TensorInfo>& output, void* outputTensorData)
{
    return { {output.first, armnn::Tensor(output.second, outputTensorData) } };
}

void AddTensorToOutput(armnn::OutputTensors &v, const std::pair<armnn::LayerBindingId,
        armnn::TensorInfo>& output, void* outputTensorData ) {
    v.push_back({output.first, armnn::Tensor(output.second, outputTensorData) });
}

#endif //DETECT_H