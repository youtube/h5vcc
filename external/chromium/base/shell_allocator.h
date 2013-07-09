#ifndef BASE_SHELL_ALLOCATOR_H_
#define BASE_SHELL_ALLOCATOR_H_

namespace base {

#if defined(__LB_LINUX__)
#define _THROW0(...)
#endif

namespace shell_allocator_internal {
// This structure should only be used internally.
struct BufferListType_ {
  BufferListType_() : preserved_buffer_start(NULL),
                      preserved_buffer_end(NULL),
                      preserved_item_count(0),
                      item_size(0) {}

  void* preserved_buffer_start;   // Start of preserved memory block
  void* preserved_buffer_end;     // End of preserved memory block
  unsigned int preserved_item_count;    // Number of items in preserved block
  size_t item_size;               // Size of items in this list
  std::vector<void*> items;       // List of items in the list
};

}  // namespace shell_allocator_internal

template<int max_reusable_array_size>
class ShellAllocatorBufferList {
public:
    virtual ~ShellAllocatorBufferList() {
      clear();
    }

    // Get the buffer list for the specific array size
    shell_allocator_internal::BufferListType_* get_list(int index) {
        if (index >= 0 && index < max_reusable_array_size) {
            return &buffer_list_[index];
        }
        return NULL;
    }

    void clear() {
      // Free all memory blocks
      for (int i = 0; i < max_reusable_array_size; ++i) {
        shell_allocator_internal::BufferListType_* list = get_list(i);
        DCHECK(list);
        for (int j = 0; j < list->items.size(); j++) {
          void* item = list->items[j];
          // free if not this is not a preserved block
          if (item > list->preserved_buffer_end ||
              item < list->preserved_buffer_start)
            free(item);
        }
        list->items.clear();
        // If there is preserved buffer, reinitialize them in case
        // we still need to continue to use this object.
        if (list->preserved_item_count) {
          init_list(i, list->preserved_item_count, list->item_size,
                    list->preserved_buffer_start);
        }
      }
    }

    // This is functions can be overwritten in special cases.
    static int get_list_index(int array_size) {
      DCHECK_GT(array_size, 0);
      return array_size - 1;
    }

protected:
    // Init a buffer list for a specific request count. Also set up
    // item size for the list. This function can be called from
    // derived class to use a preserved buffer for the list.
    void init_list(int array_index, size_t size, unsigned int count,
                   void* buffer) {
      shell_allocator_internal::BufferListType_* list = get_list(array_index);
      DCHECK(list);
      if (list) {
        char* buffer_char = static_cast<char*>(buffer);
        list->preserved_buffer_start = buffer_char;
        list->preserved_buffer_end = buffer_char + size * count - 1;
        list->preserved_item_count = count;
        list->item_size = size;

        list->items.reserve(count);
        for (int i = 0; i < count; ++i) {
          list->items.push_back(buffer_char + size * i);
        }
      }
    }

private:
    // The index represents the number of elements in the requested array.
    shell_allocator_internal::BufferListType_
        buffer_list_[max_reusable_array_size];
};

// Reusable buffer class
template<typename Type, typename Buffer_List_Type>
class ShellAllocatorBuffer {
public:
    inline void* allocate(size_t array_size) {
      size_t size = sizeof(Type);
      DCHECK(Buffer_List_Type::get_instance());
      int list_index = Buffer_List_Type::get_list_index(array_size);
      DCHECK_GE(list_index, 0);
      // Reuse block only when the "count" is managed
      shell_allocator_internal::BufferListType_* list =
          Buffer_List_Type::get_instance()->get_list(list_index);
      // If this list is not initialized, set the size at the first time
      if (list && list->items.size() > 0) {
        if (list->item_size == 0)
          list->item_size = size * array_size;
        // Make sure the required item size matches the list item size
        DCHECK_EQ(size * array_size, list->item_size);
        // block is available
        void* mem = list->items.back();
        list->items.pop_back();
        return mem;
      }
      return malloc(array_size * size);  // Real malloc
    }

    inline void deallocate(void* p, size_t array_size) {
      DCHECK(Buffer_List_Type::get_instance());
      int list_index = Buffer_List_Type::get_list_index(array_size);
      DCHECK_GE(list_index, 0);
      shell_allocator_internal::BufferListType_* list =
          Buffer_List_Type::get_instance()->get_list(list_index);
      // Reuse block only when the "count" is managed
      if (list) {
        list->items.push_back(p);
        return;
      }
      free(p);  // Real free
    }
};

// Allocator class. This class calls allocate() and deallocate() in
// ShellAllocatorBuffer, which are global to all the instances
// that have the same Buffer_List_Type. In another word,
// the reusable buffer is global. The class should be used carefully
// Good:
// - No malloc calls even if a different object is created.
// Bad:
// - The class is not thread safe.
// - The memory blocks in the buffer list will not be freed until
//   the block list object is destroyed or "cleared".
template<typename Type, typename Buffer_List_Type>
class ShellAllocator {
public:
    // typedefs
    typedef Type value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef size_t size_type;
    typedef std::ptrdiff_t difference_type;

    // convert an allocator<T> to allocator<U>
    template<typename U>
    struct rebind {
      typedef ShellAllocator<U, Buffer_List_Type> other;
    };

    ShellAllocator() _THROW0() {}
    ~ShellAllocator() {}
    ShellAllocator(const ShellAllocator<Type, Buffer_List_Type>&) _THROW0() {}
    template<class _Other, class _other_T>
        ShellAllocator(const ShellAllocator<_Other, _other_T>&) _THROW0() {}

    // address
    inline pointer address(reference r) { return &r; }
    inline const_pointer address(const_reference r) { return &r; }

    // memory allocation
    inline pointer allocate(size_type cnt,
        typename std::allocator<void>::const_pointer = 0) {
      void* new_memory = reuse_buffer_.allocate(cnt);
      return static_cast<pointer>(new_memory);
    }
    inline void deallocate(pointer p, size_type n) {
      reuse_buffer_.deallocate(p, n);
    }
    // size
    inline size_type max_size() const {
      size_type count = (size_type)(-1) / sizeof (Type);
      return (0 < count ? count : 1);
    }

    // construction/destruction
    inline void construct(pointer p, const Type& t) {
      new(p) Type(t);
    }
    inline void destroy(pointer p) {
      p->~Type();
    }

    inline bool operator==(ShellAllocator const&) { return true; }
    inline bool operator!=(ShellAllocator const& a) { return !operator==(a); }

private:
    ShellAllocatorBuffer<Type, Buffer_List_Type>  reuse_buffer_;
};


// The node class is not accessible. This is a placeholder type
// to avoid accessing the class in hash_map's internal code.
// We need only the size of the class.
template<typename NodeDataType>
class shell_hash_map_node_sim {
  NodeDataType value_;
  void* dummy1_;
  // gcc's _Hash_node is a Value type and a HashNode*.
#if !defined (__LB_LINUX__)
  void* dummy2_;
#endif
};

template <typename K, typename V, typename Allocator>
class shell_hash_map :
#if defined(__LB_PS3__) || defined(__LB_WIIU__)
    public base::hash_map<K, V, std::hash_compare<K, std::less<K> >, Allocator> { };
#elif defined(__LB_LINUX__)
    public base::hash_map<K, V, base::hash<K>, std::equal_to<K>, Allocator> { };
#else
#error This file should not be included in non steel builds!
#endif

template <typename V, typename Allocator>
class shell_hash_set :
#if defined(__LB_PS3__) || defined(__LB_WIIU__)
    public base::hash_set<V, std::hash_compare<V, std::less<V> >, Allocator> { };
#elif defined(__LB_LINUX__)
    public base::hash_set<V, base::hash<V>, std::equal_to<V>, Allocator> { };
#else
#error This file should not be included in non steel builds!
#endif

} // namespace base

#endif  // BASE_SHELL_ALLOCATOR_H_
