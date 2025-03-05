//
// Created by 赵丹 on 25-3-5.
//

#ifndef LITETVM_NODE_NDARRAY_HASH_EQUAL_H
#define LITETVM_NODE_NDARRAY_HASH_EQUAL_H

#include "node/structural_hash.h"
#include "runtime/ndarray.h"

namespace litetvm {

class SEqualReducer;
class SHashReducer;

/*! \brief A custom hash handler that ignores NDArray raw data. */
class SHashHandlerIgnoreNDArray : public SHashHandlerDefault {
protected:
  void DispatchSHash(const ObjectRef& object, bool map_free_vars) override;
};

/*!
 * \brief Test two NDArrays for equality.
 * \param lhs The left operand.
 * \param rhs The right operand.
 * \param equal A Reducer class to reduce the structural equality result of two objects.
 * See tvm/node/structural_equal.h.
 * \param compare_data Whether or not to consider ndarray raw data in the equality testing.
 * \return The equality testing result.
 */
bool NDArrayEqual(const NDArray::Container* lhs, const NDArray::Container* rhs,
                  SEqualReducer equal, bool compare_data);

/*!
 * \brief Hash NDArray.
 * \param arr The NDArray to compute the hash for.
 * \param hash_reduce A Reducer class to reduce the structural hash value.
 * See tvm/node/structural_hash.h.
 * \param hash_data Whether or not to hash ndarray raw data.
 */
void NDArrayHash(const NDArray::Container* arr, SHashReducer* hash_reduce, bool hash_data);


}

#endif //LITETVM_NODE_NDARRAY_HASH_EQUAL_H
