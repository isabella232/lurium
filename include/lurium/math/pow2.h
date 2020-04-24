#pragma once

static inline unsigned int get_pow2(unsigned int v) {
	// https://graphics.stanford.edu/~seander/bithacks.html#IntegerLogObvious
	unsigned int r = 0; // r will be lg(v)

	while (v >>= 1) // unroll for more speed...
	{
		r++;
	}
	return r;

	/*
	// OR (IF YOU KNOW v IS A POWER OF 2):
	static const unsigned int b[] = { 0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0,
	0xFF00FF00, 0xFFFF0000 };
	register unsigned int r = (v & b[0]) != 0;
	for (i = 4; i > 0; i--) // unroll for speed...
	{
	r |= ((v & b[i]) != 0) << i;
	}*/
}

static inline unsigned int make_pow2(unsigned int v) {
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}
