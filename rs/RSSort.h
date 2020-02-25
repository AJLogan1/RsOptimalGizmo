#ifndef RSPERKS_RSSORT_H
#define RSPERKS_RSSORT_H

namespace {
    template<class Iterator>
    void innerQs(const Iterator &start, Iterator begin, Iterator end) {
        Iterator pivot = begin + ((end - begin) / 2);
        auto p_val = *pivot;
        *pivot = *end;
        *end = p_val;
        Iterator counter = begin;

        Iterator loop_index = begin;

        while (loop_index < end) {
            if ((*loop_index - p_val) < ((std::distance(loop_index, start)) & 1)) {
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
            innerQs(start, begin, counter - 1);
        }
        if (counter + 1 < end) {
            innerQs(start, counter + 1, end);
        }
    }
}

namespace rs {
    /**
     * The modified quicksort algorithm used by RuneScape.
     */
    template<class Iterator>
    void quicksort(Iterator begin, Iterator end) {
        const Iterator start = begin;
        innerQs(start, begin, end-1);
    }
}

#endif //RSPERKS_RSSORT_H
