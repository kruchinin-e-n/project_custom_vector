#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>

template <typename T>
class RawMemory {
	public:

		RawMemory() = default;

		explicit RawMemory(size_t capacity) 
			: buffer_(Allocate(capacity))	// Allocates raw memory for n elements and returns a pointer to it
			, capacity_(capacity)
		{}

		~RawMemory() { Deallocate(buffer_); }

		T* operator+(size_t offset) noexcept {
			assert(offset <= capacity_);		// Check if the offset exceeds the capacity
			return buffer_ + offset;			// Allowed to obtain the address of the memory cell next to the last element of the array
		}

		const T* operator+(size_t offset) const noexcept {
			return const_cast <RawMemory&> (*this) + offset;	// Obtain additional memory space (next to the last element of the array)
		}

		T& operator[](size_t index) noexcept {
			assert(index < capacity_);		// Checking that the index does not exceed capacity limits
			return buffer_[index];			// Accessing an element in allocated memory by index
		}

		const T& operator[](size_t index) const noexcept {		// Accessing an element in allocated memory by index
			return const_cast <RawMemory&> (*this) [index];		// Constancy trick to avoid duplication of assert
		}

		void Swap(RawMemory& other) noexcept {
			std::swap(buffer_, other.buffer_);
			std::swap(capacity_, other.capacity_);
		}

		const T* GetAddress() const noexcept { return buffer_; }	// Get const pointer to allocated memory
			  T* GetAddress()       noexcept { return buffer_; }	// Get pointer to allocated memory

		size_t Capacity() const { return capacity_; }

	private:

		static T* Allocate(size_t n) {		// Allocates raw memory for n elements and returns a pointer to it
			return n != 0
				? static_cast<T*>(operator new(n * sizeof(T))) 
				: nullptr;
		}
		static void Deallocate(T* buffer) noexcept {		// Frees raw memory previously allocated at buf using Allocate
			operator delete(buffer); 
		}

		T* buffer_ = nullptr;		// Pointer to allocated raw memory for n elements
		size_t capacity_ = 0;
};

template <typename T>
class Vector {

	public:

		// --- Constructors ---

		Vector() = default;

		explicit Vector(size_t size) 
			: data_(size)
			, size_(size) 
		{ 
			std::uninitialized_value_construct_n(	// Constructs n objects in the uninitialized storage starting at first by value-initialization
				data_.GetAddress(),					// uninitialized storage
				size);								// n objects
		}

		Vector(const Vector& other) 
			: data_(other.size_), size_(other.size_) 
		{ 
			std::uninitialized_copy_n(		// Copies count elements from a range beginning at first to an uninitialized memory area beginning
				other.data_.GetAddress(),	// range beginning at first
				size_,						// count elements
				data_.GetAddress()			// an uninitialized memory area
			); 
		}

		Vector(Vector&& other) noexcept { Swap(other); }

		// --- Destructor -- 

		~Vector() { 
			std::destroy_n(				// Destroys the n objects in the range starting at first
				data_.GetAddress(),		// range starting
				size_					// n objects
			); 
		}

		// --- Operators overloads

		const T& operator[](size_t index) const noexcept { return const_cast<Vector&>(*this)[index]  ; } // Constancy trick to avoid duplication of assert
			  T& operator[](size_t index)       noexcept { assert(index < size_); return data_[index]; } // Accessing an element in allocated memory by index

		Vector& operator=(const Vector& rhs) {			// assignment operator
			if (this != &rhs) {							// checking for self-assignment
				if (rhs.size_ > data_.Capacity()) {		// copy-and-swap
					Vector rhs_copy(rhs);				// copy-and-swap
					Swap(rhs_copy);						// copy-and-swap
				}
				else {	// Copy elements from rhs, creating new ones or deleting existing ones if necessary
					if (rhs.size_ < size_) {
						std::copy(								// Copies the elements in the range, defined by [first, last), to another range beginning at d_first
							rhs.data_.GetAddress(),				// first
							rhs.data_.GetAddress() + rhs.size_, // last
							data_.GetAddress()					// beginning at d_first
						);
						std::destroy_n(						// Destroy "tail" - Destroys the n objects in the range starting at first
							data_.GetAddress() + rhs.size_,	// range starting
							size_ - rhs.size_				// n objects
						);
					}
					else {
						std::copy(							// Copies the elements in the range, defined by [first, last), to another range beginning at d_first
							rhs.data_.GetAddress(),			// first
							rhs.data_.GetAddress() + size_,	// last
							data_.GetAddress()				// beginning at d_first
						);
						std::uninitialized_copy_n(			// Copies count elements from a range beginning at first to an uninitialized memory area beginning
							rhs.data_.GetAddress() + size_,	// range beginning at first
							rhs.size_ - size_,				// count elements
							data_.GetAddress() + size_		// an uninitialized memory area
						);
					}
					size_ = rhs.size_;
				}
			}
			return *this;
		}

		Vector& operator=(Vector&& rhs) noexcept {
			if (this != &rhs) { Swap(rhs); }	// checking for self-assignment, swap
			return *this;
		}

		// --- "std::vector"-like functions ---

		size_t Size()     const noexcept { return size_; }				// Get vector size
		size_t Capacity() const noexcept { return data_.Capacity(); }	// Get vector capacity

		void Reserve(size_t new_capacity) {								// Reserve raw memory
			if (new_capacity <= data_.Capacity()) { return; }
			RawMemory<T> new_data(new_capacity);

			// should move elements unless their move constructor throws exceptions or they do not have a copy constructor

			if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {	// constexpr if statement will be evaluated at compile time
				std::uninitialized_move_n(	// apply move semantics when conditions are met, moves count elements from a range beginning at first to an uninitialized memory area beginning at d_first
					data_.GetAddress(),		// range beginning at first
					size_,					// count elements
					new_data.GetAddress()	// an uninitialized memory area
				);
			}
			else {
				std::uninitialized_copy_n(	// apply copy when conditions are not met, Copies count elements from a range beginning at first to an uninitialized memory area beginning
					data_.GetAddress(),		// range beginning at first
					size_,					// count elements
					new_data.GetAddress()	// an uninitialized memory area
				);
			}
			std::destroy_n(			// delete old data, Destroys the n objects in the range starting at first
				data_.GetAddress(), // range starting
				size_				// n objects
			);
			data_.Swap(new_data);
		}

		void Swap(Vector& other) noexcept {
			data_.Swap(other.data_);
			std::swap(size_, other.size_);
		}

		void Resize(size_t new_size) {
			if (new_size > size_) {
				Reserve(new_size);							// Reserve raw memory
				std::uninitialized_value_construct_n(		// Constructs n objects in the uninitialized storage starting at first by value-initialization
					data_.GetAddress() + size_,				// starting at
					new_size - size_						// n objects
				);
			}
			else { 
				std::destroy_n(						// delete old data, Destroys the n objects in the range starting at first
					data_.GetAddress() + new_size,	// range starting
					size_ - new_size				// n objects
				); 
			}
			size_ = new_size;
		}

		template <typename Type>
		void PushBack(Type&& value) { EmplaceBack(std::forward<Type>(value)); }

		void PopBack() {
			if (size_ > 0) {
				std::destroy_at(data_.GetAddress() + size_ - 1); //  calls the destructor of the pointed object (last)
				--size_;	 // reduction size after removal
			}
		}

		template <typename... Args>
		T& EmplaceBack(Args&&... args) {
			T* result = nullptr;
			int size_factor = 2; // raw memory increase factor if necessary

			if (size_ == Capacity()) {
				RawMemory<T> new_data(size_ == 0 ? 1 : size_ * size_factor);
				result = new(new_data + size_) T(std::forward<Args>(args)...);

				// should move elements unless their move constructor throws exceptions or they do not have a copy constructor

				if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {	// constexpr if statement will be evaluated at compile time
					std::uninitialized_move_n(	// apply move semantics when conditions are met, moves count elements from a range beginning at first to an uninitialized memory area beginning at d_first
						data_.GetAddress(),		// range beginning at first
						size_,					// count elements
						new_data.GetAddress()	// an uninitialized memory area
					);
				}
				else {
					try { 
						std::uninitialized_copy_n(	// apply copy when conditions are not met, Copies count elements from a range beginning at first to an uninitialized memory area beginning
							data_.GetAddress(),		// range beginning at first
							size_,					// count elements
							new_data.GetAddress()	// an uninitialized memory area
						); 
					}
					catch (...) {
						std::destroy_n(						// in case of trowing exception, destroys the n objects in the range starting at first
							new_data.GetAddress() + size_,	// range starting
							1								// count of objects (only last)
						);
						throw;
					}
				}
				std::destroy_n(				// destroys the n objects in the range starting at first
					data_.GetAddress(),		// range starting
					size_					// count of objects
				);
				data_.Swap(new_data);
			}
			else {
				result = new (data_ + size_) T(std::forward<Args>(args)...);	// perfect forwarding 
			}
			++size_;	// increase size of the vector by one element
			return *result;
		}

		template<typename ...Args>
		T* Emplace(const T* pos, Args &&... args) {

			T* result = nullptr;
			size_t offset = pos - begin();
			int size_factor = 2; // raw memory increase factor if necessary

			if (size_ == Capacity()) {
				RawMemory<T> new_data(size_ == 0 ? 1 : size_ * size_factor);
				result = new(new_data + offset) T(std::forward<Args>(args)...);

				// should move elements unless their move constructor throws exceptions or they do not have a copy constructor

				if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) { // // constexpr if statement will be evaluated at compile time
					std::uninitialized_move_n(	// moves count elements from a range beginning at first to an uninitialized memory area beginning at d_first
						begin(),				// range beginning at first
						offset,					// count elements
						new_data.GetAddress()	// an uninitialized memory area
					);
					std::uninitialized_move_n(				// tail - moves count elements from a range beginning at first to an uninitialized memory area beginning at d_first
						begin() + offset,					// range beginning at first
						size_ - offset,						// count elements
						new_data.GetAddress() + offset + 1	// an uninitialized memory area
					);
				}
				else {
					try {
						std::uninitialized_copy_n(	// apply copy when conditions are not met, Copies count elements from a range beginning at first to an uninitialized memory area beginning
							begin(),				// range beginning at first
							offset,					// count elements
							new_data.GetAddress()	// an uninitialized memory area
						);
						std::uninitialized_copy_n(				// tail - // apply copy when conditions are not met, Copies count elements from a range beginning at first to an uninitialized memory area beginning
							begin() + offset,					// range beginning at first
							size_ - offset,						// count elements
							new_data.GetAddress() + offset + 1	// an uninitialized memory area
						);
					}
					catch (...) {
						std::destroy_n(						// destroys the n objects in the range starting at first
							new_data.GetAddress() + offset,	// range starting
							1								// count of objects
						);
						throw;
					}
				}
				std::destroy_n(
					begin(), 
					size_
				);
				data_.Swap(new_data); // performing reallocation
			}
			else {
				if (size_ != 0) {
					new (data_ + size_) T(std::move(*(end() - 1)));
					try { 
						std::move_backward(		// Moves the elements from the range [first, last), to another range ending at d_last. The elements are moved in reverse order (the last element is moved first), but their relative order is preserved.
							begin() + offset,	// first
							end(),				// last
							end() + 1			// range ending at d_last
						); 
					}
					catch (...) {
						std::destroy_n(		// destroys the n objects in the range starting at first
							end(),			// range starting
							1				// count of objects
						);
						throw;
					}
					std::destroy_at(begin() + offset);	//  calls the destructor of the pointed object (last)
				}
				result = new (data_ + offset) T(std::forward<Args>(args)...);
			}
			++size_;
			return result;
		}

		T* Erase(const T* pos) {

			size_t shift = pos - begin();
			std::move(					// Moves the elements in the range [first, last), to another range beginning at d_first, starting from first and proceeding to last - 1
				begin() + shift + 1,	// first
				end(),					// last
				begin() + shift			// beginning at d_first
			);
			PopBack();
			return begin() + shift;
		}

		// --- Iterators ---

		T* Insert(const T* pos, const T& value) { return Emplace(pos, value)           ; }
		T* Insert(const T* pos, T&& value     ) { return Emplace(pos, std::move(value)); }

		T* begin() noexcept { return data_.GetAddress()        ; } // getting an iterator at the beginning of the vector
		T* end()   noexcept { return data_.GetAddress() + size_; } // getting an iterator at the beginning of the vector

		const T* begin()  const noexcept { return data_.GetAddress()        ; } // get a const iterator at the beginning of the vector
		const T* end()    const noexcept { return data_.GetAddress() + size_; } // get a const iterator at the end of the vector
		const T* cbegin() const noexcept { return begin()                   ; } // getting a const iterator at the beginning of the vector
		const T* cend()   const noexcept { return end()                     ; } // getting a const iterator at the end of the vector

	private:

		RawMemory<T> data_;	// Allocated raw memory
		size_t size_ = 0;
};