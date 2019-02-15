#! /usr/bin/env python
# -*- coding: utf-8 -*-

"""
Implementation of nipype interfaces for QUIT MT modules.

To be implemented:
    - qi_dipolar_mt
    - qi_linshape
    - qi_lorentzian
    - qi_mtasym
    - qi_qmt

Requires that the QUIT tools are in your your system path

"""

from __future__ import (print_function, division, unicode_literals,
                        absolute_import)

from nipype.interfaces.base import CommandLineInputSpec, CommandLine, TraitedSpec, File, traits, isdefined
import json
import os
from .base import QUITCommand, QUITCommandInputSpec

############################ qi_lineshape ############################
# < To be implemented > #

############################ qi_lorentzian ############################


class LorentzianInputSpec(QUITCommandInputSpec):
    # Inputs
    in_file = File(exists=True,
                   argstr='%s',
                   mandatory=True,
                   desc='Path to input Z-spectrum',
                   position=-2)

    param_file = File(desc='Parameter .json file', position=-1, argstr='< %s',
                      xor=['param_dict'], mandatory=True, exists=True)

    param_dict = traits.Dict(desc='dictionary trait', position=-1,
                             argstr='', mandatory=True, xor=['param_file'])

    # Options
    mask_file = File(
        desc='Only process voxels within the mask', argstr='--mask=%s')
    threads = traits.Int(
        desc='Use N threads (default=4, 0=hardware limit)', argstr='--threads=%d')
    prefix = traits.String(
        desc='Output prefix', argstr='--out=%s')


class LorentzianOutputSpec(TraitedSpec):
    pd_map = File(desc="Path to PD map")
    f0_map = File(desc="Path to center-frequency map")
    fwhm_map = File(desc="Path to FWHM map")
    A_map = File(desc="Path to Lorentzian amplitude map")
    residual_map = File(desc="Path to residual map")


class Lorentzian(QUITCommand):
    """
    Fit a Lorentzian function to a Z-spectrum

    """

    _cmd = 'qi_lorentzian'
    input_spec = LorentzianInputSpec
    output_spec = LorentzianOutputSpec

    def _format_arg(self, name, spec, value):
        return self._process_params(name, spec, value)

    def _list_outputs(self):
        outputs = self.output_spec().get()
        outputs['pd_map'] = os.path.abspath(self._add_prefix('LTZ_PD.nii.gz'))
        outputs['f0_map'] = os.path.abspath(self._add_prefix('LTZ_f0.nii.gz'))
        outputs['fwhm_map'] = os.path.abspath(
            self._add_prefix('LTZ_fwhm.nii.gz'))
        outputs['A_map'] = os.path.abspath(self._add_prefix('LTZ_A.nii.gz'))
        outputs['residual_map'] = os.path.abspath(
            self._add_prefix('LTZ_residual.nii.gz'))
        return outputs

############################ qi_qmt ############################
# < To be implemented > #

############################ qi_zspec ############################


class ZSpecInputSpec(QUITCommandInputSpec):
    # Inputs
    in_file = File(exists=True,
                   argstr='%s',
                   mandatory=True,
                   desc='Path to input Z-spectrum',
                   position=-2)

    param_file = File(desc='Parameter .json file', position=-1, argstr='< %s',
                      xor=['param_dict'], mandatory=True, exists=True)

    param_dict = traits.Dict(desc='dictionary trait', position=-1,
                             argstr='', mandatory=True, xor=['param_file'])

    # Options
    fmap = File(
        desc='Fieldmap (in same units as frequencies)', argstr='--f0=%s')
    ref = File(desc='Reference image for %age output', argstr='--ref=%s')
    asym = traits.Bool(
        desc='Output MT-asymmetry spectrum instead of Z-spectrum', argstr='--asym')
    order = traits.Int(
        desc='Interpolation order (default 3)', argstr='--order=%d')
    threads = traits.Int(
        desc='Use N threads (default=4, 0=hardware limit)', argstr='--threads=%d')
    out_name = traits.String(
        desc='Output filename (default is input_interp)', argstr='--out=%s')


class ZSpecOutputSpec(TraitedSpec):
    out_file = File(desc="Path to interpolated Z-spectrum/MTA-spectrum")


class ZSpec(QUITCommand):
    """
    Interpolate a Z-spectrum (with correction for off-resonance)

    """

    _cmd = 'qi_zspec_interp'
    input_spec = ZSpecInputSpec
    output_spec = ZSpecOutputSpec

    def _format_arg(self, name, spec, value):
        return self._process_params(name, spec, value)

    def _parse_inputs(self, skip=None):
        if not isdefined(self.inputs.out_name):
            fname = os.path.abspath(
                self._gen_fname(self.inputs.in_file, suffix='_interp'))
            self.inputs.out_name = fname
        return super()._parse_inputs(skip)

    def _list_outputs(self):
        outputs = self.output_spec().get()
        outputs['out_file'] = self.inputs.out_name
        return outputs