#include "ring_buffer.h"

#define MIN(a, b) (((a) > (b)) ? (b) : (a))

void rb_buffer_init(rb_buffer* rb, void* mem, size_t size)
{
    rb->mem = mem;
    rb->r = 0;
    rb->w = 0;
    rb->h = size;
    rb->size = size;
}

void* rb_buffer_reserve(rb_buffer* rb, size_t size)
{
    // There are two possible states:
    //  1) r <= w -- normal setup
    //  2) r >  w -- write overtook read
    if (rb->r <= rb->w) {
        if (rb->h - rb->w >= size) {
            return rb->mem + rb->w;
        }
        // otherwise check the left side of rb->r
        if (rb->r > size) {
            return rb->mem;
        }
    } else {
        if (rb->r - rb->w > size) {
            return rb->mem + rb->w;
        }
    }
    // not enough space
    return NULL;
}

void rb_buffer_commit(rb_buffer* rb, void* ptr)
{
    // wraparound, need to update watermark
    if (ptr < rb->mem + rb->w) {
        rb->h = rb->w;
    }
    rb->w = ptr - rb->mem;
}

void* rb_buffer_read(rb_buffer* rb, size_t* actual_size, size_t max_size)
{
    size_t size = 0;
    void* ptr = rb->mem + rb->r;
    if (rb->w >= rb->r) {
        // Case 1:
        // | r | ... | w |
        size = MIN(rb->w - rb->r, max_size);
        rb->r += size;
        if (size > 0 && rb->r == rb->w) {
            rb->r = 0;
            rb->w = 0;
        }
    } else {
        // Case 2:
        // ... | w | ... | r | ... | h |
        // (Note: we only get here iff at some point
        //  w was >= r, and h was set to *that* value
        //  of w, so rb->h should be >= rb->r.)
        size = MIN(rb->h - rb->r, max_size);
        rb->r += size;
        if (size > 0 && rb->r == rb->h) {
            // if rb->r reaches 'end', skip ahead
            rb->r = 0;
            rb->h = rb->size;
        }
        // Don't need to check if rb->r == rb->w.
        // if rb->w == rb->r == rb->mem, it is handled.
        // Otherwise rb->w < rb->r. (Case 2)
    }
    *actual_size = size;
    return size > 0 ? ptr : NULL;
}

size_t rb_buffer_total(rb_buffer* rb)
{
    if (rb->w >= rb->r) {
        // Case 1:
        // ... | r | ... | w |
        return rb->w - rb->r;
    } else {
        // Case 2:
        // ... | w | ... | r | ... | h |
        return rb->h - rb->r + rb->w;
    }
}
