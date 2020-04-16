/*
 *  qi_rfprofile.cpp
 *
 *  Copyright (c) 2017 Tobias Wood.
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 */

#include "Eigen/Core"

#include <unsupported/Eigen/Splines>

#include "Args.h"
#include "ImageIO.h"
#include "ImageTypes.h"
#include "JSON.h"
#include "Monitor.h"
#include "Spline.h"
#include "Util.h"
#include "itkImageMomentsCalculator.h"
#include "itkImageSliceIteratorWithIndex.h"
#include "itkImageSource.h"

namespace itk {

class ProfileImage : public ImageSource<QI::VolumeF> {
  public:
    using Self       = ProfileImage;
    using Superclass = ImageSource<QI::VectorVolumeF>;
    using TRegion    = QI::VectorVolumeF::RegionType;
    using Pointer    = SmartPointer<Self>;

    itkNewMacro(Self);
    itkTypeMacro(Self, ImageSource);

    void SetReference(const SmartPointer<QI::VolumeF> ref) { m_reference = ref; }
    void SetDebug(const bool d) { m_debug = d; }
    void SetDim(const int d) {
        if ((m_dim < 0) || (m_dim > 2)) {
            QI::Fail("Invalid dimension for RF profile, must be 0-2");
        }
        m_dim = d;
    }
    void SetRF(const Eigen::ArrayXd pos, const Eigen::ArrayXd vals) {
        m_spline = QI::SplineInterpolator(pos, vals);
        QI::Log(m_debug, "{}", m_spline);
    }

    void SetMask(const QI::VolumeF *mask) { this->SetNthInput(1, const_cast<QI::VolumeF *>(mask)); }
    void SetCenterMask(const bool cm) { this->m_centerMask = cm; }
    typename QI::VolumeF::ConstPointer GetMask() const {
        return static_cast<const QI::VolumeF *>(this->ProcessObject::GetInput(1));
    }

    void GenerateOutputInformation() ITK_OVERRIDE {
        auto output = this->GetOutput();
        output->SetRegions(m_reference->GetLargestPossibleRegion());
        output->SetSpacing(m_reference->GetSpacing());
        output->SetDirection(m_reference->GetDirection());
        output->SetOrigin(m_reference->GetOrigin());
        output->Allocate();
    }

  protected:
    SmartPointer<QI::VolumeF> m_reference;
    QI::SplineInterpolator    m_spline;
    bool                      m_debug = false, m_centerMask = false;
    int                       m_dim = 0;

    ProfileImage() {}
    ~ProfileImage() {}
    void DynamicThreadedGenerateData(const TRegion &region) ITK_OVERRIDE {
        auto const output = this->GetOutput();
        auto const mask   = this->GetMask();
        auto const dim1   = (m_dim + 1) % 3;
        auto const dim2   = (m_dim + 2) % 3;

        ImageSliceIteratorWithIndex<QI::VolumeF> imageIt(output, region);
        imageIt.SetFirstDirection(dim1);
        imageIt.SetSecondDirection(dim2);

        ImageSliceIteratorWithIndex<QI::VolumeF> it_B1(m_reference, region);
        it_B1.SetFirstDirection(dim1);
        it_B1.SetSecondDirection(dim2);

        ImageSliceConstIteratorWithIndex<QI::VolumeF> maskIter;
        if (mask) {
            maskIter = ImageSliceConstIteratorWithIndex<QI::VolumeF>(mask, region);
            maskIter.SetFirstDirection(dim1);
            maskIter.SetSecondDirection(dim2);
            maskIter.GoToBegin();
        }

        QI::VectorVolumeF::PointType pt_center;
        if (mask && m_centerMask) {
            auto moments = itk::ImageMomentsCalculator<QI::VolumeF>::New();
            moments->SetImage(mask);
            moments->Compute();
            QI::Log(m_debug, "Mask CoG is: {}", moments->GetCenterOfGravity());
            pt_center = moments->GetCenterOfGravity();
        } else {
            // Calculate geometric center
            QI::VectorVolumeF::IndexType idx_center;
            for (int i = 0; i < 3; i++) {
                idx_center[i] = m_reference->GetLargestPossibleRegion().GetSize()[i] / 2;
            }
            m_reference->TransformIndexToPhysicalPoint(idx_center, pt_center);
        }

        // Now get physical space index to deal with obliqued scans
        QI::VectorVolumeF::IndexType index1, index2;
        index1.Fill(0);
        index2.Fill(0);
        index2[m_dim] = 1;
        QI::VectorVolumeF::PointType pt1, pt2;
        m_reference->TransformIndexToPhysicalPoint(index1, pt1);
        m_reference->TransformIndexToPhysicalPoint(index2, pt2);
        auto const diff = pt2 - pt1;
        QI::Log(m_debug, "Physical difference: {}", diff);
        int phys_dim = 0;
        if ((fabs(diff[1]) > fabs(diff[0])) || (fabs(diff[2]) > fabs(diff[0]))) {
            if (fabs(diff[2]) > fabs(diff[1])) {
                phys_dim = 2;
            } else {
                phys_dim = 1;
            }
        }
        QI::Log(m_debug, "Physical dimension: {}", phys_dim);

        while (!imageIt.IsAtEnd()) {
            QI::VectorVolumeF::PointType pt, pt_rf;
            m_reference->TransformIndexToPhysicalPoint(imageIt.GetIndex(), pt);
            pt_rf     = pt - pt_center;
            float val = m_spline(pt_rf[phys_dim]);
            QI::Log(m_debug, "Absolute co-ord: {} Relative co-ord: {} Value: {}", pt, pt_rf, val);
            while (!imageIt.IsAtEndOfSlice()) {
                while (!imageIt.IsAtEndOfLine()) {
                    if (!mask || maskIter.Get()) {
                        imageIt.Set(val * it_B1.Get());
                    } else {
                        imageIt.Set(0.);
                    }
                    ++imageIt;
                    ++it_B1;
                    if (mask) {
                        ++maskIter;
                    }
                }
                imageIt.NextLine();
                it_B1.NextLine();
                if (mask)
                    maskIter.NextLine();
            }
            imageIt.NextSlice();
            it_B1.NextSlice();
            if (mask) {
                maskIter.NextSlice();
            }
        }
    }

  private:
    ProfileImage(const Self &);
    void operator=(const Self &);
};

} // End namespace itk

int rfprofile_main(int argc, char **argv) {
    Eigen::initParallel();
    args::ArgumentParser parser("Generates a relative B1 map from a B1+ and RF profile.\nInput is "
                                "the B1+ map.\nhttp://github.com/spinicist/QUIT");
    args::Positional<std::string> b1plus_path(parser, "B1+_FILE", "Input B1+ file");
    args::Positional<std::string> output_path(parser, "B1_FILE", "Output relative B1 file");

    args::HelpFlag       help(parser, "HELP", "Show this help menu", {'h', "help"});
    args::Flag           verbose(parser, "VERBOSE", "Print more information", {'v', "verbose"});
    args::ValueFlag<int> threads(parser,
                                 "THREADS",
                                 "Use N threads (default=hardware limit or $QUIT_THREADS)",
                                 {'T', "threads"},
                                 QI::GetDefaultThreads());
    args::ValueFlag<std::string> mask(parser, "MASK", "Only process within mask", {'m', "mask"});
    args::Flag centerMask(parser, "CENTER ON MASK", "Set slab center to mask CoG", {'c', "center"});
    args::ValueFlag<std::string> subregion(
        parser,
        "REGION",
        "Process subregion starting at voxel I,J,K with size SI,SJ,SK",
        {'s', "subregion"});
    args::ValueFlag<int> dimension(
        parser, "DIMENSION", "Which dimension to calculate the profile over", {"dim"}, 2);
    args::ValueFlag<std::string> infile(
        parser, "FILE", "Read JSON input from file instead of stdin", {"json"});
    QI::ParseArgs(parser, argc, argv, verbose, threads);

    auto reference = QI::ReadImage(QI::CheckPos(b1plus_path), verbose);

    QI::Log(verbose, "Reading slab profile");
    json       input   = infile ? QI::ReadJSON(infile.Get()) : QI::ReadJSON(std::cin);
    auto const rf_pos  = QI::ArrayFromJSON(input, "rf_pos", 1.);
    auto const rf_vals = QI::ArrayFromJSON(input, "rf_vals", 1.);

    QI::Log(verbose, "Profile points = {}", rf_pos.rows());
    QI::Log(verbose, "Generating image...");
    auto image = itk::ProfileImage::New();
    image->SetReference(reference);
    image->SetRF(rf_pos, rf_vals);
    image->SetDim(dimension.Get());
    if (mask) {
        image->SetMask(QI::ReadImage(mask.Get(), verbose));
        image->SetCenterMask(centerMask);
    }
    if (verbose) {
        auto monitor = QI::GenericMonitor::New();
        image->AddObserver(itk::ProgressEvent(), monitor);
    }
    image->Update();
    QI::WriteImage(image->GetOutput(), QI::CheckPos(output_path), verbose);
    return EXIT_SUCCESS;
}
