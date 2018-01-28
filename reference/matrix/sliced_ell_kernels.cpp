/*******************************<GINKGO LICENSE>******************************
Copyright 2017-2018

Karlsruhe Institute of Technology
Universitat Jaume I
University of Tennessee

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
******************************<GINKGO LICENSE>*******************************/

#include "core/matrix/sliced_ell_kernels.hpp"


#include "core/base/exception_helpers.hpp"
#include "core/base/math.hpp"
#include "core/matrix/dense.hpp"
#include <iostream>


namespace gko {
namespace kernels {
namespace reference {
namespace sliced_ell {


template <typename ValueType, typename IndexType>
void spmv(const matrix::Sliced_ell<ValueType, IndexType> *a,
          const matrix::Dense<ValueType> *b, matrix::Dense<ValueType> *c)
{
	auto vals = a->get_const_values();
    auto col_idxs = a->get_const_col_idxs();
    auto slice_lens = a->get_const_slice_lens();
    auto slice_sets = a->get_const_slice_sets();
    int slice_num = (a->get_num_rows() + default_slice_size - 1) / default_slice_size;
    for (size_type slice = 0; slice < slice_num; slice++) {
        for (size_type row = 0; row < default_slice_size; row++) {
            size_type global_row = slice * default_slice_size + row;
            if (global_row >= a->get_num_rows()) {
                break;
            }
            for (size_type j = 0; j < c->get_num_cols(); j++) {
                c->at(global_row, j) = zero<ValueType>();
            }
            for (size_type i = 0; i < slice_lens[slice]; i++) {
                auto val = vals[row + (slice_sets[slice] + i) * default_slice_size];
                auto col = col_idxs[row + (slice_sets[slice] + i) * default_slice_size];
                for (size_type j = 0; j < c->get_num_cols(); j++) {
                    c->at(global_row, j) += val * b->at(col, j);
                }
            }
        }
    }
}

GKO_INSTANTIATE_FOR_EACH_VALUE_AND_INDEX_TYPE(GKO_DECLARE_SLICED_ELL_SPMV_KERNEL);


template <typename ValueType, typename IndexType>
void advanced_spmv(const matrix::Dense<ValueType> *alpha,
                   const matrix::Sliced_ell<ValueType, IndexType> *a,
                   const matrix::Dense<ValueType> *b,
                   const matrix::Dense<ValueType> *beta,
                   matrix::Dense<ValueType> *c)
{
	auto vals = a->get_const_values();
    auto col_idxs = a->get_const_col_idxs();
    auto slice_lens = a->get_const_slice_lens();
    auto slice_sets = a->get_const_slice_sets();
    int slice_num = (a->get_num_rows() + default_slice_size - 1) / default_slice_size;
    auto valpha = alpha->at(0, 0);
    auto vbeta = beta->at(0, 0);
    for (size_type slice = 0; slice < slice_num; slice++) {
        for (size_type row = 0; row < default_slice_size; row++) {
            size_type global_row = slice * default_slice_size + row;
            if (global_row >= a->get_num_rows()) {
                break;
            }
            for (size_type j = 0; j < c->get_num_cols(); j++) {
                c->at(global_row, j) *= vbeta;
            }
            for (size_type i = 0; i < slice_lens[slice]; i++) {
                auto val = vals[row + (slice_sets[slice] + i) * default_slice_size];
                auto col = col_idxs[row + (slice_sets[slice] + i) * default_slice_size];
                for (size_type j = 0; j < c->get_num_cols(); j++) {
                    c->at(global_row, j) += valpha * val * b->at(col, j);
                }
            }
        }
    }
}

GKO_INSTANTIATE_FOR_EACH_VALUE_AND_INDEX_TYPE(
    GKO_DECLARE_SLICED_ELL_ADVANCED_SPMV_KERNEL);


template <typename ValueType, typename IndexType>
void convert_to_dense(matrix::Dense<ValueType> *result,
                      const matrix::Sliced_ell<ValueType, IndexType> *source)
{
	auto exec = result->get_executor();
    if (exec != exec->get_master()) {
        NOT_SUPPORTED(exec);
    }
    auto num_rows = source->get_num_rows();
    auto num_cols = source->get_num_cols();
    auto vals = source->get_const_values();
    auto col_idxs = source->get_const_col_idxs();
    auto slice_lens = source->get_const_slice_lens();
    auto slice_sets = source->get_const_slice_sets();
    int slice_num = (source->get_num_rows() + default_slice_size - 1) / default_slice_size;
    for (size_type slice = 0; slice < slice_num; slice++) {
        for (size_type row = 0; row < default_slice_size; row++) {
            size_type global_row = slice * default_slice_size + row;
            if (global_row >= num_rows) {
                break;
            }
            for (size_type col = 0; col < num_cols; col++) {
                result->at(global_row, col) = zero<ValueType>();
            }
            for (size_type i = slice_sets[slice]; i < slice_sets[slice] + slice_lens[slice]; i++) {
                result->at(global_row, col_idxs[row + i * default_slice_size]) += vals[row + i * default_slice_size];
            }
        }
    }
}


GKO_INSTANTIATE_FOR_EACH_VALUE_AND_INDEX_TYPE(
    GKO_DECLARE_SLICED_ELL_CONVERT_TO_DENSE_KERNEL);


template <typename ValueType, typename IndexType>
void move_to_dense(matrix::Dense<ValueType> *result,
                   matrix::Sliced_ell<ValueType, IndexType> *source)
{
    reference::sliced_ell::convert_to_dense(result, source);
}


GKO_INSTANTIATE_FOR_EACH_VALUE_AND_INDEX_TYPE(
    GKO_DECLARE_SLICED_ELL_MOVE_TO_DENSE_KERNEL);

}  // namespace sliced_ell
}  // namespace reference
}  // namespace kernels
}  // namespace gko
