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

    template<typename A, typename F>
    void safeQuicksort(int low, int high, std::vector<A> &arr, F value_fn) {
        int pivot_index = (low + high) / 2;
        A pivot_value = arr[pivot_index];
        arr[pivot_index] = arr[high];
        arr[high] = pivot_value;
        int counter = low;
        int loop_index = low;

        while (loop_index < high) {
            // We test ((loop_index+1) & 1) here because in our code we sort from index 1.
            // Therefore, the alternating <=/< would be the wrong way round.
            // This perhaps isn't the most elegant or portable solution at the moment.
            if (value_fn(arr[loop_index]) - value_fn(pivot_value) < ((loop_index + 1) & 1)) {
                A tmp = arr[loop_index];
                arr[loop_index] = arr[counter];
                arr[counter] = tmp;
                counter++;
            }
            loop_index++;
        }

        arr[high] = arr[counter];
        arr[counter] = pivot_value;

        if (low < (counter - 1)) {
            safeQuicksort(low, counter - 1, arr, value_fn);
        }
        if (counter + 1 < high) {
            safeQuicksort(counter + 1, high, arr, value_fn);
        }
    }
}

#endif //RSPERKS_RSSORT_H
