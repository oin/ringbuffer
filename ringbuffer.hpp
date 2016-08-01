#pragma once
/**
 * C++11 header-only implementation of a ring-buffer. This is a work-in-progress.
 * (C) 2016 Jonathan Aceituno <join@oin.name> (http://oin.name)
 *
 * License: MIT
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <array>
#include <utility>
#include <vector>

#ifdef RINGBUFFER_DEBUG
#include <iostream>
#endif

template <typename T, bool Flag>
struct ringbuffer_constify;
template <typename T>
struct ringbuffer_constify<T, true> {
	typedef const T type;
};
template <typename T>
struct ringbuffer_constify<T, false> {
	typedef T type;
};

struct ringbuffer_overwrite {
	static bool can_push_back(size_t /*capacity*/, size_t /*size*/) { return true; }
	static void advance_push_back(size_t capacity, size_t& head, size_t& size, size_t next) {
		if(size == capacity) {
			head = next;
		} else {
			++size;
		}
	}
};
struct ringbuffer_no_overwrite {
	static bool can_push_back(size_t capacity, size_t size) { return size < capacity; }
	static void advance_push_back(size_t /*capacity*/, size_t& /*head*/, size_t& size, size_t /*next*/) {
		++size;
	}
};

template <typename T, size_t N>
struct ringbuffer_c_array {
	typedef T buffer_t[N];
	typedef typename std::allocator<T> allocator_type;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	typedef T value_type;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T* pointer;
	typedef T* iterator;
	static void initialize(buffer_t& /*buf*/, size_type /*n*/) {};
	static size_type capacity(const buffer_t& /*buf*/) { return N; }
	static const T* data(const buffer_t& buf) { return buf; }
	static iterator begin(buffer_t& buf) { return buf; }
	static iterator end(buffer_t& buf) { return buf + N; }
};

template <typename T, size_t N>
struct ringbuffer_array {
	typedef typename std::array<T, N> buffer_t;
	typedef typename std::allocator<T> allocator_type;
	typedef typename buffer_t::size_type size_type;
	typedef typename buffer_t::difference_type difference_type;
	typedef typename buffer_t::value_type value_type;
	typedef typename buffer_t::reference reference;
	typedef typename buffer_t::const_reference const_reference;
	typedef typename buffer_t::pointer pointer;
	typedef typename buffer_t::iterator iterator;
	static void initialize(buffer_t& /*buf*/, size_type /*n*/) {};
	static size_type capacity(const buffer_t& /*buf*/) { return N; }
	static const T* data(const buffer_t& buf) { return buf.data(); }
	static iterator begin(buffer_t& buf) { return buf.begin(); }
	static iterator end(buffer_t& buf) { return buf.end(); }
};

template <typename T>
struct ringbuffer_vector {
	typedef typename std::vector<T> buffer_t;
	typedef typename buffer_t::allocator_type allocator_type;
	typedef typename buffer_t::size_type size_type;
	typedef typename buffer_t::difference_type difference_type;
	typedef typename buffer_t::value_type value_type;
	typedef typename buffer_t::reference reference;
	typedef typename buffer_t::const_reference const_reference;
	typedef typename buffer_t::pointer pointer;
	typedef typename buffer_t::iterator iterator;
	static void initialize(buffer_t& buf, size_type n) { buf.reserve(n); }
	static size_type capacity(const buffer_t& buf) { return buf.capacity(); }
	static const T* data(const buffer_t& buf) { return buf.data(); }
	static iterator begin(buffer_t& buf) { return buf.begin(); }
	static iterator end(buffer_t& buf) { return buf.end(); }
};

template <typename ContainerStrategy, class OverwritePolicy = ringbuffer_no_overwrite>
class ringbuffer {
public:
	typedef OverwritePolicy overwrite_policy;
	typedef typename ContainerStrategy::allocator_type allocator_type;
	typedef typename ContainerStrategy::value_type value_type;
	typedef typename ContainerStrategy::reference reference;
	typedef typename ContainerStrategy::const_reference const_reference;
	typedef typename ContainerStrategy::difference_type difference_type;
	typedef typename ContainerStrategy::size_type size_type;
	typedef typename ContainerStrategy::buffer_t container_type;
	typedef typename ContainerStrategy::iterator container_iterator;

	ringbuffer(size_t capacity = 0) : head_(0), size_(0) { ContainerStrategy::initialize(buffer_, capacity); }
	ringbuffer(const ringbuffer& buf) : buffer_(buf.buffer_), head_(buf.head_), size_(buf.size_) {}
	~ringbuffer() {}

	ringbuffer& operator=(const ringbuffer& buf) {
		if(&buf != this) {
			buffer_ = buf.buffer_;
			head_ = buf.head_;
			size_ = buf.size_;
		}
		return *this;
	}

	bool operator==(const ringbuffer& buf) const {
		return buffer_ == buf.buffer_ && head_ == buf.head_ && size_ == buf.size_;
	}
	bool operator!=(const ringbuffer& buf) const { return !(*this == buf); }

	size_type size() const { return size_; }
	size_type capacity() const { return ContainerStrategy::capacity(buffer_); }
	bool empty() const { return size_ == 0; }
	bool full() const { return size_ == capacity(); }

	const_reference front() const { return buffer_[head_]; }
	const_reference back() const { return buffer_[tail()]; }
	reference at(difference_type i) const { return buffer_[tail(i)]; }

	template <bool IsConst=false>
	class ringbuffer_iterator {
	public:
		typedef ringbuffer container_type;
		typedef typename ContainerStrategy::difference_type difference_type;
		typedef typename ContainerStrategy::value_type value_type;
		typedef typename ringbuffer_constify<typename ContainerStrategy::reference, IsConst>::type reference;
		typedef typename ringbuffer_constify<typename ContainerStrategy::pointer, IsConst>::type pointer;
		typedef std::random_access_iterator_tag iterator_category;
	private:
		ringbuffer* buffer_;
		difference_type offset_;

		friend class ringbuffer;
	public:
		ringbuffer_iterator(ringbuffer& buffer, difference_type offset = 0) : buffer_(&buffer), offset_(offset) {}
		ringbuffer_iterator(const ringbuffer_iterator<true>& it) : buffer_(it.buffer_), offset_(it.offset_) {}
		ringbuffer_iterator(const ringbuffer_iterator<false>& it) : buffer_(it.buffer_), offset_(it.offset_) {}
		~ringbuffer_iterator() {}
		ringbuffer_iterator& operator=(const ringbuffer_iterator& it) {
			if(&it != this) {
				buffer_ = it.buffer_;
				offset_ = it.offset_;
			}
			return *this;
		}
		
		bool operator==(const ringbuffer_iterator& it) const {
			return offset_ == it.offset_;
		}
		bool operator!=(const ringbuffer_iterator& o) const { return !(*this == o); }
		bool operator<(const ringbuffer_iterator& o) const {
			return offset_ < o.offset_;
		}
		bool operator>(const ringbuffer_iterator& o) const { return !(*this < o); }

		ringbuffer_iterator& operator++() {
			++offset_;
			return *this;
		}
		ringbuffer_iterator& operator--() {
			--offset_;
			return *this;
		}
		ringbuffer_iterator& operator+=(const difference_type& n) {
			offset_ += n;
			return *this;
		}
		ringbuffer_iterator& operator-=(const difference_type& n) {
			return *this += -n;
		}
		ringbuffer_iterator operator+(const difference_type& n) {
			return ringbuffer_iterator(buffer_, offset_ + n);
		}
		ringbuffer_iterator operator-(const difference_type& n) {
			return *this + (-n);
		}
		difference_type operator-(const ringbuffer_iterator& it) {
			return offset_ - it.offset_;
		}

		reference operator*() const { return buffer_->buffer_[buffer_->index(offset_)]; }
		pointer operator->() const { return &(buffer_->buffer_[buffer_->index(offset_)]); }
	};
	typedef ringbuffer_iterator<> iterator;
	typedef ringbuffer_iterator<true> const_iterator;

	iterator begin() { return iterator(*this, head_); }
	iterator end() { return iterator(*this, head_ + size_); }
	iterator offset(size_type i) { return iterator(*this, head_ + i); }

	container_type& data() { return buffer_; }
	container_iterator data_begin() { return ContainerStrategy::begin(buffer_); }
	container_iterator data_end() { return ContainerStrategy::end(buffer_); }

	bool contiguous() const { return tail() > head_ || empty(); }
	iterator begin_contig1() {
		return begin();
	}
	iterator end_contig1() {
		return !contiguous()? iterator(*this, capacity()) : end();
	}
	std::pair<const value_type*, size_t> contig1() const {
		return std::make_pair(ContainerStrategy::data(buffer_) + head_, !contiguous()? capacity() - head_ : size_);
	}

	size_type push_back(const_reference e) {
		if(!OverwritePolicy::can_push_back(capacity(), size_)) {
			return 0;
		}
		buffer_[tail()] = e;
		OverwritePolicy::advance_push_back(capacity(), head_, size_, index(head_ + 1));
		return 1;
	}

	template <class InputIt>
	size_type push_back(InputIt first, InputIt last) {
		size_type former_size = size();

		std::copy(first, last, std::back_inserter(*this));
		
		difference_type count = size() - former_size;
		return count;
	}

	// size_type push_back(const value_type* buffer, size_t length) {
	// 	difference_type overflow = size() + length - capacity();
	// 	size_t actual_length = overflow > 0? length - overflow : ;
	// 	difference_type diff = tail() - index(tail() + length);
	// 	if(diff > 0) {
	// 		// We need to copy the buffer in two contiguous parts
			
	// 	} else {
	// 		std::copy(buffer, buffer + length, );
	// 	}
	// } // TODO: optimize buffer copy by copying contiguous parts

	size_type pop_front() {
		if(empty()) {
			return 0;
		}
		head_ = index(head_ + 1);
		--size_;
		return 1;
	}

	size_type pop_front(iterator it) {
		difference_type diff = std::min(size(), it.offset_ - head_);
		if(diff > 0) {
			head_ = index(head_ + diff);
			size_ -= diff;
			return static_cast<size_type>(diff);
		}
		return 0;
	}

	void clear() { // ~= pop_front(end())
		head_ =  tail();
		size_ = 0;
	}

#ifdef RINGBUFFER_DEBUG
	friend std::ostream& operator<<(std::ostream& stream, const ringbuffer& rb) {
		stream << std::endl << "  ";

		size_t tail = rb.tail();
		size_t capacity = rb.capacity();

		for(size_t i=0; i<capacity; ++i) {
			bool valid = rb.buffer_index_valid(i);
			// std::cout << "head: " << rb.head_ << ", tail: " << rb.tail() << ", size: " << rb.size_ << std::endl;
			stream << (valid? " " : "(") << (rb.buffer_[i]) << (valid? " " : ")") << (i < capacity - 1? "-" : "");
		}

		stream << std::endl;

		for(size_t i=0; i<capacity; ++i) {
			if(i == rb.head_ && i == tail) {
				stream << " >|<";
			} else if(rb.head_ == i) {
				stream << "  |<";
			} else if(tail == i) {
				stream << ">|  ";
			} else {
				bool valid = rb.buffer_index_valid(i);
				if(valid) {
					stream << "----";
				} else {
					stream << "    ";
				}
			}
		}

		stream << std::endl;

		return stream;
	}
	void print() {
		std::cout << *this << std::endl;
	}
#endif

private:
	container_type buffer_;
	size_type head_;
	size_type size_;
	size_type index(size_type i) const { return i % capacity(); }
	size_type tail() const { return index(head_ + size_); }
	bool buffer_index_valid(size_type i) const {
		size_type tl = tail();
		if(tl < head_) {
			return i < tl || i >= head_;
		}
		return full() || (i >= head_ && i < tl);
	}
};

template <typename T, size_t N, class overwrite_policy = ringbuffer_no_overwrite>
using stack_ringbuffer = ringbuffer<ringbuffer_array<T,N>, overwrite_policy>;

template <typename T, class overwrite_policy = ringbuffer_no_overwrite>
using heap_ringbuffer = ringbuffer<ringbuffer_vector<T>, overwrite_policy>;
