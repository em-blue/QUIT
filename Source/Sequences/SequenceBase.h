/*
 *  SequenceBase.h
 *
 *  Copyright (c) 2016 Tobias Wood.
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 */

#ifndef SEQUENCES_BASE_H
#define SEQUENCES_BASE_H

#include "JSON.h"
#include <Eigen/Core>
#include <string>

namespace QI {

struct SequenceBase {
    virtual std::string &  name() const = 0;
    virtual Eigen::Index   size() const = 0;
    virtual size_t         count() const;
    virtual Eigen::ArrayXd weights(double f0 = 0.0) const;
};

#define QI_SEQUENCE_DECLARE(NAME)        \
    std::string &name() const override { \
        static std::string name = #NAME; \
        return name;                     \
    }                                    \
    NAME##Sequence() = default;

#define QI_SEQUENCE_DECLARE_NOSIG(NAME)  \
    std::string &name() const override { \
        static std::string name = #NAME; \
        return name;                     \
    }                                    \
    NAME##Sequence() = default;

} // End namespace QI

#endif // SEQUENCES_BASE_H
