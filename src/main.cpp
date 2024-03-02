/* MIT License
 *
 * Copyright (c) 2019 - Folke Vesterlund
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
#include <iostream>

#include "CCL.h"
#include "timer.h"
#include "utils.hpp"
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

int main(int argc, char **argv) {

  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " <image file>" << std::endl;
    return (-1);
  }
  const std::string fileName = argv[1];

  // Read image
  const cv::Mat image = cv::imread(fileName, cv::IMREAD_GRAYSCALE);
  if (!image.data) {
    std::cerr << "Couldn't open file" << std::endl;
    return (-1);
  }

  if (!image.isContinuous()) {
    std::cerr << "Image is not allocated with continuous data. Exiting..."
              << std::endl;
    return (-1);
  }
  const int numCols = image.cols;
  const int numRows = image.rows;
  const int numPixels = numRows * numCols;

  std::cout << "Read image with dimension: " << numCols << "," << numRows
            << std::endl;

  for (int i = 0; i < 50; i++) {
    // Run and time kernel
    Timer timer;
    std::vector<unsigned char> imaged(image.begin<unsigned char>(),
                                      image.end<unsigned char>());
    std::vector<unsigned int> labels(numPixels, 0);

    timer.Start();
    connectedComponentLabeling(
        std::execution::par_unseq, numCols, numRows,
        [image = imaged.data()](int idx0, int idx1) {
          return image[idx0] > 0 && image[idx1] > 0 &&
                 std::abs(image[idx0] - image[idx1]) < 10;
        },
        labels.data());
    timer.Stop();
    std::cout << "code ran in: " << timer.Elapsed() << "ms" << std::endl;

    // Count components
    unsigned int components = util::countComponents(labels.data(), numPixels);
    std::cout << "Number of components: " << components << std::endl;

    // Plot result
    cv::Mat finalImage = util::postProc(labels.data(), numCols, numRows);
    cv::imwrite("image_labelled.png", finalImage);
  }
}
