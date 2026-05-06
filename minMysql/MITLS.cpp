#include "MITLS.h"
#include "min_mysql.h"

template <typename T>
thread_local typename mi_tls_repository<T>::Repo mi_tls_repository<T>::repo{};

template <>
thread_local mi_tls_repository<bool>::Repo mi_tls_repository<bool>::repo{};

template <>
thread_local mi_tls_repository<int>::Repo mi_tls_repository<int>::repo{};

template <>
thread_local mi_tls_repository<long>::Repo mi_tls_repository<long>::repo{};

template <>
thread_local mi_tls_repository<u64>::Repo mi_tls_repository<u64>::repo{};

template <>
thread_local mi_tls_repository<std::shared_ptr<St_mysqlW>>::Repo mi_tls_repository<std::shared_ptr<St_mysqlW>>::repo{};

template <>
thread_local mi_tls_repository<DB::InternalState>::Repo mi_tls_repository<DB::InternalState>::repo{};
