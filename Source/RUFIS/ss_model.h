#pragma once

#include "Model.h"
#include "rufis_sequence.h"

struct SSModel : QI::Model<double, double, 3, 0> {
    RUFISSequence &    sequence;
    VaryingArray const start{30.0, 1.0, 1};
    VaryingArray const lo{0.1, 0.5, 0.5};
    VaryingArray const hi{60.0, 5.0, 1.5};

    std::array<std::string, NV> const varying_names{"M0", "T1", "B1"};

    int  input_size(const int /* Unused */) const { return sequence.size(); }
    auto signal(VaryingArray const &v, FixedArray const &) const -> QI_ARRAY(double);
    void derived(const VaryingArray &varying, const FixedArray &, DerivedArray &derived) const;
};

template <> struct QI::NoiseFromModelType<SSModel> : QI::RealNoise {};
