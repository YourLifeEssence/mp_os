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
    //std::cout << space_size << " " << memory_size << " " << *reinterpret_cast<size_t*>(placement) << std::endl;
    placement += sizeof(size_t);
    *reinterpret_cast<void**>(placement) = placement + sizeof(bool) + sizeof(void*);

    *reinterpret_cast<bool*>(reinterpret_cast<unsigned char*>(placement) + sizeof(void*)) = false; //блок свободен

    *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(placement) + sizeof(bool) + sizeof(void*)) = space_size;

    *reinterpret_cast<void**>(*reinterpret_cast<void**>(placement)) = nullptr;

    //second side meta
    unsigned char* reverse_placement = reinterpret_cast<unsigned char*>(placement) + space_size - get_available_block_meta_size();

    *reinterpret_cast<size_t*>(reverse_placement) = space_size;

    reverse_placement += sizeof(size_t);
    *reinterpret_cast<void**>(reverse_placement) = nullptr;

    reverse_placement += sizeof(void*);
    *reinterpret_cast<bool*>(reverse_placement) = false;

    debug_with_guard("exit constr alloc");
}

[[nodiscard]] void *allocator_boundary_tags::allocate(
    size_t value_size,
    size_t values_count)
{
    debug_with_guard("entrance in allocate");
    throw_if_allocator_instance_state_was_moved();

    std::lock_guard<std::mutex> lock(obtain_synchronizer());

    auto requested_size = value_size * values_count + get_available_block_meta_size();

    unsigned char* current_block = reinterpret_cast<unsigned char*>(_trusted_memory) + get_void_ptr_shift();
    unsigned char* last_block = reinterpret_cast<unsigned char*>(_trusted_memory) + *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(_trusted_memory) + get_fit_mode_shift());

    allocator_with_fit_mode::fit_mode fit_mode = *reinterpret_cast<allocator_with_fit_mode::fit_mode*>(reinterpret_cast<unsigned char*>(_trusted_memory) + get_mutex_shift());

    void* selected_block = nullptr;

    switch (fit_mode) {
    case allocator_with_fit_mode::fit_mode::first_fit :
        debug_with_guard("entrance switch-case about fit mode - first fit");
        selected_block = allocate_with_first_fit(requested_size);
        break;
    case allocator_with_fit_mode::fit_mode::the_best_fit :
        debug_with_guard("entrance switch-case about fit mode - the best fit");
        selected_block = allocate_with_best_fit(requested_size);
        break;
    case allocator_with_fit_mode::fit_mode::the_worst_fit :
        debug_with_guard("entrance switch-case about fit mode - the worst fit");
        selected_block = allocate_with_worst_fit(requested_size);
        break;
    default:
        throw std::invalid_argument("Unknown fit mode");
    }

    if (selected_block) {
        debug_with_guard("exited allocate");
        return selected_block;
    }
    debug_with_guard("selected_block equal nullptr");
    error_with_guard("error allocate memory in allocate - bad alloc\n");
    throw std::bad_alloc();
}

void allocator_boundary_tags::deallocate(
    void *at)
{
    debug_with_guard("entrance dealloc");
    throw not_implemented("void allocator_boundary_tags::deallocate(void *)", "your code should be here...");
}

inline void allocator_boundary_tags::set_fit_mode(
    allocator_with_fit_mode::fit_mode mode)
{
    debug_with_guard("entrance set_fit_mode");
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

    return *reinterpret_cast<class logger**>(reinterpret_cast<unsigned char*>(_trusted_memory) + get_allocator_shift());
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
    std::cout << *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(_trusted_memory) + get_fit_mode_shift()) << std::endl;

    unsigned char* current_block = reinterpret_cast<unsigned char*>(_trusted_memory) + get_void_ptr_shift();
    unsigned char* last_block = reinterpret_cast<unsigned char*>(_trusted_memory) + *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(_trusted_memory) + get_fit_mode_shift());
    while (current_block < last_block) {
        auto size_Block = *reinterpret_cast<size_t*>(current_block + sizeof(void*));
        bool if_true = *reinterpret_cast<bool*>(current_block + get_status_block_shift());
        if (if_true) {

            if (size_Block == sizeNewBlock) 
                return allocateFullBlock(current_block);

            if (size_Block > sizeNewBlock) 
                return allocateBlock(current_block, sizeNewBlock);
        }
        current_block += size_Block;
    }
    debug_with_guard("error in allocate_with_first_fit");
    throw std::bad_alloc();
}

void*& allocator_boundary_tags::get_first_block() const {
    return *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(_trusted_memory) + get_size_shift());
}

constexpr size_t allocator_boundary_tags::get_status_block_shift() {
    return sizeof(void*);
}

constexpr size_t allocator_boundary_tags::get_size_block_shift() {
    return get_status_block_shift() + sizeof(bool);
}

void* allocator_boundary_tags::allocateBlock(unsigned char* block, size_t sizeNewBlock) {
    auto sizeOfBlock = *reinterpret_cast<size_t*>(block + get_size_block_shift());
    auto remainingSize = sizeOfBlock - sizeNewBlock - get_available_block_meta_size();

    *reinterpret_cast<size_t*>(block + get_size_block_shift()) = sizeNewBlock;
    *reinterpret_cast<bool*>(block + get_status_block_shift()) = true;

    unsigned char* new_block = block + sizeNewBlock + get_available_block_meta_size();

    *reinterpret_cast<size_t*>(new_block + get_size_block_shift()) = remainingSize;
    *reinterpret_cast<bool*>(new_block + get_status_block_shift()) = false;

    return block + get_size_block_shift();
}

void* allocator_boundary_tags::allocateFullBlock(unsigned char* block) {
    *reinterpret_cast<bool*>(block + get_status_block_shift()) = true;

    return block + get_size_block_shift();
}

void* allocator_boundary_tags::allocate_with_best_fit(size_t sizeNewBlock) {
    unsigned char* current_block = reinterpret_cast<unsigned char*>(_trusted_memory) + get_void_ptr_shift();
    unsigned char* last_block = reinterpret_cast<unsigned char*>(_trusted_memory) + *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(_trusted_memory) + get_fit_mode_shift());

    unsigned char* best_fit_block = nullptr;
    size_t best_fit_size = SIZE_MAX;

    while (current_block < last_block) {
        auto sizeOfBlock = *reinterpret_cast<size_t*>(current_block + get_size_block_shift());
        if (!*reinterpret_cast<bool*>(current_block + get_status_block_shift())) {
            if (sizeOfBlock >= sizeNewBlock && sizeOfBlock < best_fit_size) {
                best_fit_size = sizeOfBlock;
                best_fit_block = current_block;
            }
        }
        current_block += sizeOfBlock;
    }

    if (best_fit_block) {
        return allocateBlock(best_fit_block, sizeNewBlock);
    }
    debug_with_guard("error in allocate_with_best_fit");
    throw std::bad_alloc();
}

void* allocator_boundary_tags::allocate_with_worst_fit(size_t sizeNewBlock) {
    unsigned char* current_block = reinterpret_cast<unsigned char*>(_trusted_memory) + get_void_ptr_shift();
    unsigned char* last_block = reinterpret_cast<unsigned char*>(_trusted_memory) + *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(_trusted_memory) + get_fit_mode_shift());

    unsigned char* worst_fit_block = nullptr;
    size_t worst_fit_size = 0;

    while (current_block < last_block) {
        auto sizeOfBlock = *reinterpret_cast<size_t*>(current_block + get_size_block_shift());
        if (!*reinterpret_cast<bool*>(current_block + get_status_block_shift())) {
            if (sizeOfBlock >= sizeNewBlock && sizeOfBlock > worst_fit_size) {
                worst_fit_size = sizeOfBlock;
                worst_fit_block = current_block;
            }
        }
        current_block += sizeOfBlock;
    }

    if (worst_fit_block) {
        return allocateBlock(worst_fit_block, sizeNewBlock);
    }
    debug_with_guard("error in allocate_with_worst_fit");
    throw std::bad_alloc();
}