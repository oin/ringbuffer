#include <cstdio>
#define RINGBUFFER_DEBUG
#include "ringbuffer.hpp"
#include <algorithm>
#include <string>

enum { RINGBUFFER_COUNT = 14 };

typedef stack_ringbuffer<uint8_t, RINGBUFFER_COUNT, ringbuffer_no_overwrite> ringbuffer_t;
// typedef heap_ringbuffer<uint8_t, ringbuffer_no_overwrite> ringbuffer_t;

void print_rb(ringbuffer_t& rb) {
	printf("--- (print) ---\n%lu elements: ", rb.size());
	for(auto c : rb) {
		printf("%c ", c);
	}
	printf("\n");
	rb.print();
	printf("{begin_|end_}contig1: from %c to %c (excluded)\n", *rb.begin_contig1(), *rb.end_contig1());
	printf("contig1 as a buffer: ");
	auto c = rb.contig1();
	// printf(" (%lu) %s\n", c.second, rb.contiguous()? "(contiguous)":"(non-contiguous)");
	for(size_t i=0; i<c.second; ++i) {
		printf("%c", c.first[i]);
	}
	printf("\n");
	printf("---\n");
}

void pop_rb(ringbuffer_t& rb) {
	if(rb.empty()) {
		printf("Cannot pop 1 element\n");
	} else {
		auto e = rb.front();
		rb.pop_front();
		printf("Popping 1 element: %c\n", e);
	}
}

template <class RingBufferIterator, class OverwritePolicy = typename RingBufferIterator::container_type::overwrite_policy>
struct ringbuffer_iterator_update {
	static void update(RingBufferIterator&, RingBufferIterator) {}
};

template <class RingBufferIterator>
struct ringbuffer_iterator_update<RingBufferIterator, ringbuffer_overwrite> {
	static void update(RingBufferIterator& it, RingBufferIterator it2) {
		it = it2;
		printf("coucou\n");
	}
};

void test_ringbuffer_iterator_update(ringbuffer_t& rb) {
	// printf("starting test\n");
	auto it = rb.offset(1);
	ringbuffer_iterator_update<decltype(it)>::update(it, rb.begin());
	// printf("ending test\n");
}

int main() {
	ringbuffer_t rb(RINGBUFFER_COUNT);

	std::fill(rb.data_begin(), rb.data_end(), ' ');

	print_rb(rb);
	std::string elts = "ABCDE";
	printf("Inserting '%s'\n", elts.c_str());
	std::copy(begin(elts), end(elts), std::back_inserter(rb));
	print_rb(rb);
	printf("Clearing '%s'\n", elts.c_str());
	rb.pop_front(std::end(rb));
	print_rb(rb);
	printf("Inserting '%s'\n", elts.c_str());
	std::copy(begin(elts), end(elts), std::back_inserter(rb));
	print_rb(rb);
	printf("Removing 3 elements\n");
	// for(size_t i=0; i<3; ++i) pop_rb(rb);
	rb.pop_front(rb.offset(3));
	print_rb(rb);
	elts = "abcde";
	printf("Inserting '%s'\n", elts.c_str());
	std::copy(begin(elts), end(elts), std::back_inserter(rb));
	print_rb(rb);

	printf("%ld is the distance\n", std::distance(std::begin(rb), std::end(rb)));

	rb.clear();
	rb.push_back('a');
	rb.push_back('b');
	rb.push_back('c');

	test_ringbuffer_iterator_update(rb); // this function is empty if ringbuffer_no_overwrite is chosen, but not if ringbuffer_overwrite is chosen

	return 0;
}