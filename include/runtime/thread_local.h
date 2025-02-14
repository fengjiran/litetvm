//
// Created by 赵丹 on 25-2-8.
//

#ifndef THREAD_LOCAL_H
#define THREAD_LOCAL_H

#include <mutex>
#include <vector>

namespace litetvm::runtime {

/*!
 * \brief A threadlocal store to store threadlocal variables.
 *  Will return a thread local singleton of type T
 * \tparam T the type we like to store
 */
// template<typename T>
// class ThreadLocalStore {
// public:
//     /*! \return get a thread local singleton */
//     static T* Get() {
//         thread_local T inst;
//         return &inst;
//     }
//
// private:
//     /*! \brief constructor */
//     ThreadLocalStore() {}
//     /*! \brief destructor */
//     ~ThreadLocalStore() {
//         for (size_t i = 0; i < data_.size(); ++i) {
//             delete data_[i];
//         }
//     }
//     /*! \return singleton of the store */
//     static ThreadLocalStore<T>* Singleton() {
//         static ThreadLocalStore<T> inst;
//         return &inst;
//     }
//     /*!
//    * \brief register str for internal deletion
//    * \param str the string pointer
//    */
//     void RegisterDelete(T* str) {
//         std::unique_lock<std::mutex> lock(mutex_);
//         data_.push_back(str);
//         lock.unlock();
//     }
//     /*! \brief internal mutex */
//     std::mutex mutex_;
//     /*!\brief internal data */
//     std::vector<T*> data_;
// };

}// namespace litetvm::runtime

#endif//THREAD_LOCAL_H
