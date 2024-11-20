#include <not_implemented.h>

#include "../include/allocator_global_heap.h"

allocator_global_heap::allocator_global_heap(
    logger *logger) :
    _logger(logger)
{
    debug_with_guard("allocator_global_heap()");
}

allocator_global_heap::~allocator_global_heap()
{
    debug_with_guard("~allocator_global_heap()");
    _logger = nullptr;
    debug_with_guard("logger has become nullptr");
}

allocator_global_heap::allocator_global_heap(
    allocator_global_heap&& other) noexcept :
    _logger(other._logger)
{
    debug_with_guard("entrance in the constr movement");
    other._logger = nullptr;
    debug_with_guard("other logger has become nullptr");
}

allocator_global_heap &allocator_global_heap::operator=(
    allocator_global_heap &&other) noexcept
{
    debug_with_guard("entrance in the operator movement");
    if (this!= &other)
    {
        _logger = other._logger;
        other._logger = nullptr;
        debug_with_guard("other logger has become nullptr (in operator move)");
    }
    return *this;
}

[[nodiscard]] void *allocator_global_heap::allocate(
    size_t value_size,
    size_t values_count)
{
    debug_with_guard(std::string("Entrance in the allocate\n") + "Allocate : " + std::to_string(value_size) + " , " + std::to_string(values_count));
    void* ptr = nullptr;
    try
    {
        ptr = ::operator new(value_size * values_count);
    }
    catch (const std::bad_alloc& ex)
    {
        critical_with_guard(ex.what());
        error_with_guard(ex.what());
        throw ex;
    }
    debug_with_guard("the memory was successfully allocated");
    return ptr;
}

void allocator_global_heap::deallocate(
    void *at)
{
    debug_with_guard("Entrance in the deallocate");
    ::operator delete(at);
    debug_with_guard("Exit from deallocate");
}

inline logger *allocator_global_heap::get_logger() const
{
    return _logger;
}

inline std::string allocator_global_heap::get_typename() const noexcept
{
    return "allocator_global_heap";
}