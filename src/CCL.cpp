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
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "CCL.h"
#include "reduction.h"
#include <numeric>
#include <ranges>
namespace stdv = std::views;
namespace stdr = std::ranges;
#include <algorithm>
#include <execution>
#include <iostream>

#include <semaphore>


/* Connected component labeling on binary images based on
 * the article by Playne and Hawick https://ieeexplore.ieee.org/document/8274991. */
void connectedComponentLabeling(unsigned int* outputImg, unsigned char* inputImg, size_t numCols, size_t numRows)
{
	// Initialise labels
	std::cout << "initializing.." << std::endl;
	init_labels(outputImg, inputImg, numCols, numRows);

	// Analysis
	std::cout << "analysis.." << std::endl;
	resolve_labels(outputImg, numCols, numRows);

	// Label Reduction
	std::cout << "reduction.." << std::endl;
	label_reduction(outputImg, inputImg, numCols, numRows);

	// Analysis
	std::cout << "resolving labels.." << std::endl;
	resolve_labels(outputImg, numCols, numRows);

	// Force background to have label zero;
	std::cout << "resolving background.." << std::endl;
	resolve_background(outputImg, inputImg, numCols, numRows);
}

void init_labels(unsigned int* g_labels, const unsigned char *g_image, const size_t numCols, const size_t numRows) {

	auto idx = stdv::iota(0, (int)(numRows*numCols));

	std::for_each(std::execution::par_unseq, idx.begin(),idx.end(),[g_labels,g_image,numCols,numRows](int i){
		// Calculate index
		const unsigned int ix = i % numCols;
		const unsigned int iy = (i-ix)/numCols;
		// Fetch five image values
		const unsigned char pyx = g_image[iy*numCols + ix];

		// Neighbour Connections
		const bool nym1x   =  (iy > 0) 					  	 ? (pyx == g_image[(iy-1) * numCols + ix  ]) : false;
		const bool nyxm1   =  (ix > 0)  		  			 ? (pyx == g_image[(iy  ) * numCols + ix-1]) : false;
		const bool nym1xm1 = ((iy > 0) && (ix > 0)) 		 ? (pyx == g_image[(iy-1) * numCols + ix-1]) : false;
		const bool nym1xp1 = ((iy > 0) && (ix < numCols -1)) ? (pyx == g_image[(iy-1) * numCols + ix+1]) : false;

		// Label
		unsigned int label;

		// Initialise Label
		// Label will be chosen in the following order:
		// NW > N > NE > E > current position
		label = (nyxm1)   ?  iy   *numCols + ix-1 : iy*numCols + ix;
		label = (nym1xp1) ? (iy-1)*numCols + ix+1 : label;
		label = (nym1x)   ? (iy-1)*numCols + ix   : label;
		label = (nym1xm1) ? (iy-1)*numCols + ix-1 : label;

		// Write to Global Memory
		g_labels[iy*numCols + ix] = label;
	
	});	
	
}

// Resolve Kernel
void resolve_labels(unsigned int *g_labels,
		const size_t numCols, const size_t numRows) {
	auto idx = stdv::iota(0, (int)(numRows*numCols));
	std::for_each(std::execution::par_unseq, idx.begin(),idx.end(),[g_labels,numCols,numRows](int i){
		g_labels[i] = find_root(g_labels, g_labels[i]);
	});
	
}

// Label Reduction
void label_reduction(unsigned int *g_labels, const unsigned char *g_image,
		const size_t numCols, const size_t numRows) {
	auto idx = stdv::iota(0, (int)(numRows*numCols));

	#ifdef COMPILE_FOR_GPU
	auto min = [](unsigned int label1, unsigned int label2){
		return atomicMin(&label1, label2);
	};
	#else
	auto min = [semaphore = std::make_shared<std::binary_semaphore>(1)](unsigned int label1, unsigned int label2){
		semaphore->acquire();
		auto min = std::min({label1, label2});
		semaphore->release();
		return min;
	};
	#endif

	std::for_each(std::execution::par_unseq, idx.begin(),idx.end(),[g_labels,g_image,numCols,numRows, min](int i){
		// Calculate index
		const unsigned int ix = i % numCols;
		const unsigned int iy = (i-ix)/numCols;
	
		// Compare Image Values
		const unsigned char pyx = g_image[iy*numCols + ix];
		const bool nym1x = (iy > 0) ? (pyx == g_image[(iy-1)*numCols + ix]) : false;

		if(!nym1x) {
			// Neighbouring values
			const bool nym1xm1 = ((iy > 0) && (ix >  0)) 		 ? (pyx == g_image[(iy-1) * numCols + ix-1]) : false;
			const bool nyxm1   =              (ix >  0) 		 ? (pyx == g_image[(iy  ) * numCols + ix-1]) : false;
			const bool nym1xp1 = ((iy > 0) && (ix < numCols -1)) ? (pyx == g_image[(iy-1) * numCols + ix+1]) : false;

			if(nym1xp1){
				// Check Criticals
				// There are three cases that need a reduction
				if ((nym1xm1 && nyxm1) || (nym1xm1 && !nyxm1)){
					// Get labels
					unsigned int label1 = g_labels[(iy  )*numCols + ix  ];
					unsigned int label2 = g_labels[(iy-1)*numCols + ix+1];

					// Reduction
					reduction(g_labels, label1, label2, min);
				}

				if (!nym1xm1 && nyxm1){
					// Get labels
					unsigned int label1 = g_labels[(iy)*numCols + ix  ];
					unsigned int label2 = g_labels[(iy)*numCols + ix-1];

					// Reduction
					reduction(g_labels, label1, label2, min);
				}
			}
		}
	});
}

// Force background to get label zero;
void resolve_background(unsigned int *g_labels, const unsigned char *g_image,
		const size_t numCols, const size_t numRows){
	
	auto idx = stdv::iota(0, (int)(numRows*numCols));
	std::for_each(std::execution::par_unseq, idx.begin(),idx.end(),[g_labels,g_image,numCols,numRows](int i){
		g_labels[i] = (g_image[i] > 0) ? g_labels[i]+1 : 0;
		
	});
	
}
