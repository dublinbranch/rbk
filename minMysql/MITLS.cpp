#include "MITLS.h"
#include "min_mysql.h"

// Explicit definition of the static thread_local member
template <typename T>
thread_local
    typename mi_tls_repository<T>::mapS mi_tls_repository<T>::repository =
        std::make_shared<typename mi_tls_repository<T>::mapT>();

// Explicit instantiation (with definition) for the bool specialization
template <>
thread_local mi_tls_repository<bool>::mapS mi_tls_repository<bool>::repository =
    std::make_shared<mi_tls_repository<bool>::mapT>();

// Explicit definitions of static thread_local members for required types
template <>
thread_local mi_tls_repository<int>::mapS mi_tls_repository<int>::repository =
    std::make_shared<mi_tls_repository<int>::mapT>();

template <>
thread_local mi_tls_repository<long>::mapS mi_tls_repository<long>::repository =
    std::make_shared<mi_tls_repository<long>::mapT>();

template <>
thread_local mi_tls_repository<std::shared_ptr<St_mysqlW>>::mapS mi_tls_repository<std::shared_ptr<St_mysqlW>>::repository =
    std::make_shared<mi_tls_repository<std::shared_ptr<St_mysqlW>>::mapT>();

template <>
thread_local mi_tls_repository<DB::InternalState>::mapS mi_tls_repository<DB::InternalState>::repository =
    std::make_shared<mi_tls_repository<DB::InternalState>::mapT>();
