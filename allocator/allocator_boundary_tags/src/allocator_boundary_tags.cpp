#include <not_implemented.h>

#include <mutex>

#include "../include/allocator_boundary_tags.h"

allocator_boundary_tags::~allocator_boundary_tags()
{
    debug_with_guard("Entrance in the destructor");

    clear_memory();

    debug_with_guard("Exited the destructor");
}

allocator_boundary_tags::allocator_boundary_tags(
    allocator_boundary_tags &&other) noexcept :
    _trusted_memory(nullptr)
{
    //is it necessary?
    if (other._trusted_memory == nullptr) {
        debug_with_guard("other object is empty");
        throw std::logic_error("Other obj can't be empty");
    }

    std::lock_guard<std::mutex> lock(obtain_synchronizer());
    _trusted_memory = other._trusted_memory;
    other._trusted_memory = nullptr;
}

allocator_boundary_tags &allocator_boundary_tags::operator=(
    allocator_boundary_tags &&other) noexcept
{
    //is it necessary?
    if (other._trusted_memory == nullptr){
        debug_with_guard("other object is empty");
        throw std::logic_error("Other obj can't be empty");
    }

    if (this != &other) {
        std::lock_guard<std::mutex> lock(obtain_synchronizer());

        clear_memory();

        _trusted_memory = other._trusted_memory;
        other._trusted_memory = nullptr;
    }
    return *this;
}

allocator_boundary_tags::allocator_boundary_tags(
    size_t space_size,
    allocator *parent_allocator,
    logger *logger,
    allocator_with_fit_mode::fit_mode allocate_fit_mode)
{


    if (space_size < get_available_block_meta_size()) {
        logger->log("The allocated memory is not enough to accommodate metadata", logger::severity::error);
        throw std::logic_error("Can't initialize allocator instance");
    }

    size_t memory_size = space_size + common_medata_size() + get_available_block_meta_size();

    try
    {
        _trusted_memory = parent_allocator == nullptr ? ::operator new(memory_size) : parent_allocator->allocate(1, memory_size);
    }
    catch (const std::bad_alloc& e)
    {
        logger->log("Error allocate of the memory", logger::severity::critical);
        throw;
    }

    allocator** parent_allocator_placement = reinterpret_cast<allocator**>(_trusted_memory);
    *parent_allocator_placement = parent_allocator;

    class logger** logger_placement = reinterpret_cast<class logger**>(parent_allocator_placement + 1);
    *logger_placement = logger;

    std::mutex* synchronizer_placement = reinterpret_cast<std::mutex*>(logger_placement + 1);
    new (reinterpret_cast<void*>(synchronizer_placement)) std::mutex();

    unsigned char* placement = reinterpret_cast<unsigned char*>(synchronizer_placement);

    placement += sizeof(std::mutex);
    *reinterpret_cast<allocator_with_fit_mode::fit_mode*>(placement) = allocate_fit_mode;

    placement += sizeof(allocator_with_fit_mode::fit_mode);
    *reinterpret_cast<size_t*>(placement) = memory_size;

    placement += sizeof(size_t);
    *reinterpret_cast<void**>(placement) = placement + sizeof(void*);

    *reinterpret_cast<void**>(*reinterpret_cast<void**>(placement)) = nullptr;

    *reinterpret_cast<size_t*>(reinterpret_cast<void**>(*reinterpret_cast<void**>(placement)) + 1) = memory_size - get_available_block_meta_size();
    //TODO : finish it :::: 1)allocate   2)func allocate_with_fit_mode....

}

[[nodiscard]] void *allocator_boundary_tags::allocate(
    size_t value_size,
    size_t values_count)
{
    throw_if_allocator_instance_state_was_moved();

    std::lock_guard<std::mutex> lock(obtain_synchronizer());

    auto requsted_size = value_size * values_count + get_available_block_meta_size();

    ...
}

void allocator_boundary_tags::deallocate(
    void *at)
{
    throw not_implemented("void allocator_boundary_tags::deallocate(void *)", "your code should be here...");
}

inline void allocator_boundary_tags::set_fit_mode(
    allocator_with_fit_mode::fit_mode mode)
{
    if (_trusted_memory == nullptr)
        return;

    std::lock_guard<std::mutex> lock(obtain_synchronizer());

    *reinterpret_cast<allocator_with_fit_mode::fit_mode*>(reinterpret_cast<unsigned char*>(_trusted_memory) + get_mutex_shift()) = mode;
}

inline allocator *allocator_boundary_tags::get_allocator() const
{
    if (_trusted_memory == nullptr)
        return nullptr;

    return *reinterpret_cast<allocator**>(_trusted_memory);
}

std::vector<allocator_test_utils::block_info> allocator_boundary_tags::get_blocks_info() const noexcept
{
    throw not_implemented("std::vector<allocator_test_utils::block_info> allocator_boundary_tags::get_blocks_info() const noexcept", "your code should be here...");
}

inline logger *allocator_boundary_tags::get_logger() const
{
    if (_trusted_memory == nullptr)
        return nullptr;

    return *reinterpret_cast<logger**>(reinterpret_cast<unsigned char*>(_trusted_memory) + get_allocator_shift());
}

inline std::string allocator_boundary_tags::get_typename() const noexcept
{
    return "allocator_boundary_tags";
}

// beginning 
constexpr size_t allocator_boundary_tags::get_available_block_meta_size() {
    return 2 * sizeof(bool) + 2 * sizeof(void*) + 2 * sizeof(size_t);
}

constexpr size_t allocator_boundary_tags::get_ancillary_block_meta_size() {
    return 2 * sizeof(bool) + 2 * sizeof(void*) + 2 * sizeof(size_t);
}

constexpr size_t allocator_boundary_tags::common_medata_size() {
    return sizeof(allocator*) + sizeof(logger*) + sizeof(std::mutex) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t) + sizeof(void*);
}

inline size_t allocator_boundary_tags::get_allocator_shift() const{
    return sizeof(allocator*);
}

inline size_t allocator_boundary_tags::get_logger_shift() const{
    return get_allocator_shift() + sizeof(logger*);
}

inline size_t allocator_boundary_tags::get_mutex_shift() const {
    return get_logger_shift() + sizeof(std::mutex);
}

inline size_t allocator_boundary_tags::get_fit_mode_shift() const {
    return get_mutex_shift() + sizeof(allocator_with_fit_mode::fit_mode);
}

inline size_t allocator_boundary_tags::get_size_shift() const {
    return get_fit_mode_shift() + sizeof(size_t);
}

inline size_t allocator_boundary_tags::get_void_ptr_shift() const {
    return get_size_shift() + sizeof(void*);
}

std::mutex& allocator_boundary_tags::obtain_synchronizer() const {
    return *reinterpret_cast<std::mutex*>(const_cast<unsigned char*>(reinterpret_cast<unsigned char const*>(_trusted_memory) + get_logger_shift()));
}

void allocator_boundary_tags::clear_memory() {
    debug_with_guard("Entrance in the func: clear_memory");
    if (_trusted_memory == nullptr)
        return;

    destruct(&obtain_synchronizer()); //Delete mutex

    if (get_allocator() == nullptr) { //Clearing memory
        ::operator delete(_trusted_memory);
        debug_with_guard("in if ( get_allocator() == nullptr )");
    }
    else {
        get_allocator()->deallocate(_trusted_memory);
        debug_with_guard("in else ( get_allocator() != nullptr )");
    }

    _trusted_memory = nullptr;
    debug_with_guard("Exited the func: clear_memory");
}

void allocator_boundary_tags::throw_if_allocator_instance_state_was_moved() const {
    if (_trusted_memory == nullptr)
    {
        throw std::logic_error("Allocator instance state was moved :/");
    }
}

void* allocator_boundary_tags::allocate_with_first_fit(size_t sizeNewBlock) {
    unsigned char* current_block = reinterpret_cast<unsigned char*>(_trusted_memory) + get_void_ptr_shift();
    unsigned char* last_block = reinterpret_cast<unsigned char*>(_trusted_memory) + *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(_trusted_memory) + get_fit_mode_shift());
    while (current_block < last_block) {
        auto sizeOfBlock = *reinterpret_cast<size_t*>(current_block + get_size_block_shift());
        if (!*reinterpret_cast<bool*>(current_block + get_status_block_shift())) {
            ...
        }
        current_block += sizeOfBlock;
    }
    throw std::bad_alloc();
    }
}

void*& allocator_boundary_tags::get_first_block() const {
    return *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(_trusted_memory) + get_size_shift());
}

constexpr size_t allocator_boundary_tags::get_status_block_shift() {
    return sizeof(void*);
}

constexpr size_t allocator_boundary_tags::get_size_block_shift() {
    return get_status_Block_Shift() + sizeof(bool);
}