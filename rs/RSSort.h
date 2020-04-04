#ifndef RSPERKS_RSSORT_H
#define RSPERKS_RSSORT_H

namespace {
    template<class Iterator, class F>
    void innerQs(const Iterator &start, Iterator begin, Iterator end, F value_fn) {
        Iterator pivot = begin + ((end - begin) / 2);
        auto p_val = *pivot;
        *pivot = *end;
        *end = p_val;
        Iterator counter = begin;

        Iterator loop_index = begin;

        while (loop_index < end) {
            if ((value_fn(*loop_index) - value_fn(p_val)) < ((std::distance(loop_index, start)) & 1)) {
                auto tmp = *loop_index;
                *loop_index = *counter;
                *counter = tmp;
                counter++;
            }
            loop_index++;
        }

        *end = *counter;
        *counter = p_val;

        if (begin < counter - 1) {
            innerQs(start, begin, counter - 1, value_fn);
        }
        if (counter + 1 < end) {
            innerQs(start, counter + 1, end, value_fn);
        }
    }
}

namespace rs {
    /**
     * The modified quicksort algorithm used by RuneScape.
     */
    template<class Iterator, class F>
    void quicksort(Iterator begin, Iterator end, F value_fn) {
        const Iterator start = begin;
        innerQs(start, begin, end - 1, value_fn);
    }
}

#endif //RSPERKS_RSSORT_H
