#pragma once

#include <cassert>
#include <cstdlib>
#include <new>
#include <memory>
#include <utility>

template <typename T>
class RawMemory
{
public:

	RawMemory() = default;

	RawMemory(const RawMemory&) = delete;

	RawMemory(RawMemory&& other) noexcept{
		this->Swap(other);
	}

	explicit RawMemory(size_t capacity): buffer_(Allocate(capacity)), capacity_(capacity) {}

	RawMemory& operator=(const RawMemory& rhs) = delete;

	RawMemory& operator=(RawMemory&& rhs) noexcept{
		this->Swap(rhs);
		return *this;
	}

	~RawMemory(){
		Deallocate(buffer_);
	}

	T* operator+(size_t offset) noexcept{
		assert(offset <= capacity_);
		return buffer_ + offset;
	}

	const T* operator+(size_t offset) const noexcept{
		return const_cast<RawMemory&>(*this) + offset;
	}

	const T& operator[](size_t index) const noexcept{
		return const_cast<RawMemory&>(*this)[index];
	}

	T& operator[](size_t index) noexcept{
		assert(index < capacity_);
		return buffer_[index];
	}

	void Swap(RawMemory& other) noexcept{
		std::swap(buffer_, other.buffer_);
		std::swap(capacity_, other.capacity_);
	}

	const T* GetAddress() const noexcept{
		return buffer_;
	}

	T* GetAddress() noexcept{
		return buffer_;
	}

	size_t Capacity() const{
		return capacity_;
	}

private:

	static T* Allocate(size_t n){
		return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
	}


	static void Deallocate(T* buf) noexcept{
		operator delete(buf);
	}

	T* buffer_ = nullptr;
	size_t capacity_ = 0;
};

template <typename T>
class Vector{
public:

	Vector() = default;

	explicit Vector(size_t size): data_(size), size_(size){
		std::uninitialized_value_construct_n(data_.GetAddress(), size);
	}

	Vector(const Vector& other): data_(other.size_), size_(other.size_){
		std::uninitialized_copy_n(other.data_.GetAddress(), other.size_, data_.GetAddress());
	}

	Vector(Vector&& other) noexcept{
		this->Swap(other);
	}

	Vector& operator=(const Vector& other){
		if (other.size_ > data_.Capacity()){
			Vector tmp(other);
			Swap(tmp);
		}
		else{
			size_t i = size_ < other.size_ ? size_ : other.size_;
			std::copy_n(other.begin(), i, begin());
			if (size_ < other.size_)
			{
				std::uninitialized_copy_n(other.data_.GetAddress(), other.size_ - size_, data_.GetAddress() + size_);
			}
			else if (size_ > other.size_)
			{
				std::destroy_n(data_.GetAddress() + other.size_, size_ - other.size_);
			}
			size_ = other.size_;
		}
		return *this;
	}

	Vector& operator=(Vector&& rhs) noexcept{
		this->Swap(rhs);
		return *this;
	}

	void Swap(Vector& other) noexcept{
		data_.Swap(other.data_);
		std::swap(size_, other.size_);
	}

	~Vector(){
		std::destroy_n(data_.GetAddress(), size_);
	}

	void Reserve(size_t new_capacity){
		if (new_capacity <= data_.Capacity()){
			return;
		}
		RawMemory<T> new_data(new_capacity);
		if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>){
			std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
		}
		else{
			std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
		}
		std::destroy_n(data_.GetAddress(), size_);
		data_.Swap(new_data);
	}

	size_t Size() const noexcept{
		return size_;
	}

	size_t Capacity() const noexcept{
		return data_.Capacity();
	}

	void Resize(size_t new_size){
		if (size_ > new_size){
			std::destroy_n(data_.GetAddress() + new_size, size_ - new_size);
		}
		else if (size_ < new_size){
			Reserve(new_size);
			std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
		}
		size_ = new_size;
	}

	template <typename... Args>
	T& EmplaceBack(Args&&... args){
		if (size_ == data_.Capacity()){
			RawMemory<T> new_data((size_ == 0) ? 1 : (size_ * 2));
			auto elem = new (new_data.GetAddress() + size_) T(std::forward<Args>(args)...);
			if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>){
				std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
			}
			else{
				std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
			}
			std::destroy_n(data_.GetAddress(), size_);
			data_.Swap(new_data);
			++size_;
			return *elem;
		}
		else{
			auto elem = new (data_ + size_) T(std::forward<Args>(args)...);
			++size_;
			return *elem;
		}
	}

	void PushBack(const T& value){
		EmplaceBack(value);
	}

	void PushBack(T&& value){
		EmplaceBack(std::move(value));
	}

	void PopBack() noexcept{
		if (size_ > 0){
			std::destroy_at(data_.GetAddress() + size_);
			--size_;
		}
	}

	const T& operator[](size_t index) const noexcept{
		return data_[index];
	}

	T& operator[](size_t index) noexcept{
		assert(index < size_);
		return data_[index];
	}

	using iterator = T*;
	using const_iterator = const T*;

	iterator begin() noexcept{
		return data_.GetAddress();
	}

	iterator end() noexcept{
		return data_.GetAddress() + size_;
	}

	const_iterator begin() const noexcept{
		return  data_.GetAddress();
	}

	const_iterator end() const noexcept{
		return data_.GetAddress() + size_;
	}

	const_iterator cbegin() const noexcept{
		return data_.GetAddress();
	}

	const_iterator cend() const noexcept{
		return data_.GetAddress() + size_;
	}

	template <typename... Args>
	iterator Emplace(const_iterator pos, Args&&... args){
		assert(pos >= cbegin() && pos <= cend());
		const size_t res_index = pos - begin();
		if (size_ == res_index){
			EmplaceBack(std::forward<Args>(args)...);
		}
		else if (size_ == data_.Capacity()){
			RawMemory<T> new_data((size_ == 0) ? 1 : (size_ * 2));
			new(new_data + res_index) T(std::forward<Args>(args)...);
			if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>){
				std::uninitialized_move_n(begin(), res_index, new_data.GetAddress());
				std::uninitialized_move_n(begin() + res_index, size_ - res_index, new_data.GetAddress() + res_index + 1);
			}
			else{
				std::uninitialized_copy_n(begin(), res_index, new_data.GetAddress());
				std::uninitialized_copy_n(begin() + res_index, size_ - res_index, new_data.GetAddress() + res_index + 1);
			}
			std::destroy_n(data_.GetAddress(), size_);
			data_.Swap(new_data);
			++size_;
		}
		else{
			T tmp = T(std::forward<Args>(args)...);
			new (data_ + size_) T(std::move(*(end() - 1)));
			std::move_backward(begin() + res_index, end() - 1, end());
			data_[res_index] = std::move(tmp);
			++size_;
		}
		return begin() + res_index;
	}

	iterator Insert(const_iterator pos, const T& value){
		return Emplace(pos, value);
	}

	iterator Insert(const_iterator pos, T&& value){
		return Emplace(pos, std::move(value));
	}

	iterator Erase(const_iterator pos) noexcept(std::is_nothrow_move_assignable_v<T>){
		assert(pos >= cbegin() && pos < cend());
		size_t it = pos - cbegin();
		if (it + 1 < size_)
		{
			std::move(begin() + it + 1, end(), begin() + it);
		}
		PopBack();
		return begin() + it;
	}

private:

	RawMemory<T> data_;
	size_t size_ = 0;

	static void Destroy(T* buf) noexcept{
		buf->~T();
	}
};
