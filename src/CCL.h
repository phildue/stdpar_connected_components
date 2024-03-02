/* MIT License
 *
 * Copyright (c) 2024 - Philipp Dürnay
 *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef CCL_H_
#define CCL_H_
#include "reduction.h"
#include <algorithm>
#include <cstddef>
#include <execution>
#include <iostream>
#include <numeric>
#include <ranges>

/* Connected component labeling on binary images based on
 * the article by Playne and Hawick
 * https://ieeexplore.ieee.org/document/8274991. */
template <class Compare, class ExecutionPolicy>
void connectedComponentLabeling(ExecutionPolicy policy, size_t w, size_t h,
                                Compare compare, unsigned int *labels) {
  auto idx = std::views::iota(0, (int)(h * w));
  std::for_each(policy, idx.begin(), idx.end(),
                [labels, w, compare](int i) { init(labels, w, i, compare); });

  std::for_each(policy, idx.begin(), idx.end(),
                [labels](int i) { labels[i] = find_root(labels, labels[i]); });

  std::for_each(policy, idx.begin(), idx.end(),
                [labels, w, compare](int i) { reduce(labels, w, i, compare); });

  std::for_each(policy, idx.begin(), idx.end(),
                [labels](int i) { labels[i] = find_root(labels, labels[i]); });
}

template <class Compare>
void init(unsigned int *labels, int w, int idx, Compare compare) {
  // Calculate index
  const unsigned int ix = idx % w;
  const unsigned int iy = (idx - ix) / w;

  // Neighbour Connections
  const bool nym1x = (iy > 0) && compare(idx, idx - w);
  const bool nyxm1 = (ix > 0) && compare(idx, idx - 1);
  const bool nym1xm1 = ((iy > 0) && (ix > 0)) && compare(idx, idx - w - 1);
  const bool nym1xp1 = ((iy > 0) && (ix < w - 1)) && compare(idx, idx - w + 1);

  // Label
  unsigned int label;

  // Initialise Label
  // Label will be chosen in the following order:
  // NW > N > NE > E > current position
  label = (nyxm1) ? idx - 1 : idx;
  label = (nym1xp1) ? idx - w + 1 : label;
  label = (nym1x) ? idx - w : label;
  label = (nym1xm1) ? idx - w - 1 : label;

  // Write to Global Memory
  labels[idx] = label;
}

template <class Compare>
void reduce(unsigned int *labels, int w, int idx, Compare compare) {
  // Calculate index
  const unsigned int ix = idx % w;
  const unsigned int iy = (idx - ix) / w;

  // Compare Image Values
  const bool nym1x = (iy > 0) && compare(idx, idx - w);

  if (!nym1x) {
    // Neighbouring values
    const bool nym1xm1 = ((iy > 0) && (ix > 0)) && compare(idx, idx - w - 1);
    const bool nyxm1 = (ix > 0) && compare(idx, idx - 1);
    const bool nym1xp1 =
        ((iy > 0) && (ix < w - 1)) && compare(idx, idx - w + 1);

    if (nym1xp1) {
      // Check Criticals
      // There are three cases that need a reduction
      if ((nym1xm1 && nyxm1) || (nym1xm1 && !nyxm1)) {
        // Get labels
        unsigned int label1 = labels[idx];
        unsigned int label2 = labels[idx - w + 1];

        // Reduction
        reduction(labels, label1, label2);
      }

      if (!nym1xm1 && nyxm1) {
        // Get labels
        unsigned int label1 = labels[idx];
        unsigned int label2 = labels[idx - 1];

        // Reduction
        reduction(labels, label1, label2);
      }
    }
  }
}

#endif /* CCL_H_ */
