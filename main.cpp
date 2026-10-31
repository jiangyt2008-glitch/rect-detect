#include <opencv2/opencv.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

bool isRectangle(const std::vector<cv::Point>& polygon, const cv::Size& imageSize) {
    if (polygon.size() != 4) {
        return false;
    }

    if (!cv::isContourConvex(polygon)) {
        return false;
    }

    const double imageArea = static_cast<double>(imageSize.width) * imageSize.height;
    const double rectangleArea = std::abs(cv::contourArea(polygon));
    const double minimumArea = std::max(100.0, imageArea * 0.0005);

    if (rectangleArea < minimumArea) {
        return false;
    }

    for (int i = 0; i < 4; ++i) {
        const cv::Point2f current = polygon[i];
        const cv::Point2f previous = polygon[(i + 3) % 4];
        const cv::Point2f next = polygon[(i + 1) % 4];

        const cv::Point2f side1 = previous - current;
        const cv::Point2f side2 = next - current;

        const double sideLength1 = cv::norm(side1);
        const double sideLength2 = cv::norm(side2);
        if (sideLength1 < 8.0 || sideLength2 < 8.0) {
            return false;
        }

        const double cosine = std::abs(side1.dot(side2) /
                                       (sideLength1 * sideLength2));
        if (cosine > 0.35) {
            return false;
        }
    }

    const cv::Rect box = cv::boundingRect(polygon);
    const bool nearImageBorder = box.x <= 1 && box.y <= 1 &&
                                 box.x + box.width >= imageSize.width - 1 &&
                                 box.y + box.height >= imageSize.height - 1;

    if (nearImageBorder && rectangleArea > imageArea * 0.95) {
        return false;
    }

    return true;
}

std::vector<cv::Point> orderCorners(std::vector<cv::Point> points) {
    std::sort(points.begin(), points.end(), [](const cv::Point& a, const cv::Point& b) {
        if (a.y != b.y) {
            return a.y < b.y;
        }
        return a.x < b.x;
    });

    cv::Point topLeft = points[0];
    cv::Point topRight = points[1];
    if (topLeft.x > topRight.x) {
        std::swap(topLeft, topRight);
    }

    cv::Point bottomLeft = points[2];
    cv::Point bottomRight = points[3];
    if (bottomLeft.x > bottomRight.x) {
        std::swap(bottomLeft, bottomRight);
    }

    return {topLeft, topRight, bottomRight, bottomLeft};
}

int main(int argc, char* argv[]) {
    const fs::path inputDirectory = argc >= 2 ? argv[1] : "/input";
    const fs::path outputDirectory = argc >= 3 ? argv[2] : "/out";

    if (!fs::exists(inputDirectory) || !fs::is_directory(inputDirectory)) {
        std::cerr << "Input directory does not exist: "
                  << inputDirectory.string() << '\n';
        return 1;
    }

    fs::create_directories(outputDirectory);

    // 找到输入目录第一层中的所有 PNG 文件。
    std::vector<fs::path> imageFiles;
    for (const fs::directory_entry& entry : fs::directory_iterator(inputDirectory)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        std::string extension = entry.path().extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
                       [](unsigned char ch) {
                           return static_cast<char>(std::tolower(ch));
                       });

        if (extension == ".png") {
            imageFiles.push_back(entry.path());
        }
    }

    std::sort(imageFiles.begin(), imageFiles.end());

   
    for (const fs::path& imagePath : imageFiles) {
        cv::Mat image = cv::imread(imagePath.string(), cv::IMREAD_COLOR);
        if (image.empty()) {
            std::cerr << "Failed to read image: " << imagePath.string() << '\n';
            continue;
        }

        cv::Mat grayImage;
        cv::cvtColor(image, grayImage, cv::COLOR_BGR2GRAY);
        cv::GaussianBlur(grayImage, grayImage, cv::Size(5, 5), 0);

        cv::Mat edgeImage;
        cv::Canny(grayImage, edgeImage, 50, 150);

     
        const cv::Mat kernel = cv::getStructuringElement(
            cv::MORPH_RECT, cv::Size(3, 3));
        cv::morphologyEx(edgeImage, edgeImage, cv::MORPH_CLOSE, kernel);

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(edgeImage, contours, cv::RETR_EXTERNAL,
                         cv::CHAIN_APPROX_SIMPLE);

        std::vector<std::vector<cv::Point>> rectangles;

        for (const std::vector<cv::Point>& contour : contours) {
            const double perimeter = cv::arcLength(contour, true);
            if (perimeter < 40.0) {
                continue;
            }

            std::vector<cv::Point> polygon;
            cv::approxPolyDP(contour, polygon, perimeter * 0.02, true);

            if (isRectangle(polygon, image.size())) {
                rectangles.push_back(orderCorners(polygon));
            }
        }

        std::sort(rectangles.begin(), rectangles.end(),
                  [](const std::vector<cv::Point>& a,
                     const std::vector<cv::Point>& b) {
                      const cv::Rect boxA = cv::boundingRect(a);
                      const cv::Rect boxB = cv::boundingRect(b);
                      if (std::abs(boxA.y - boxB.y) > 10) {
                          return boxA.y < boxB.y;
                      }
                      return boxA.x < boxB.x;
                  });

        cv::Mat outputImage = image.clone();
        for (const std::vector<cv::Point>& rectangle : rectangles) {
            cv::polylines(outputImage, rectangle, true,
                          cv::Scalar(0, 255, 0), 2);
        }

        const fs::path outputPath = outputDirectory / imagePath.filename();
        cv::imwrite(outputPath.string(), outputImage);

        std::cout << imagePath.filename().string() << ": ";

        if (rectangles.empty()) {
            std::cout << "null\n";
            continue;
        }

        for (std::size_t i = 0; i < rectangles.size(); ++i) {
            if (i > 0) {
                std::cout << " | ";
            }

            std::cout << "rect" << i + 1 << ' ';
            for (std::size_t j = 0; j < 4; ++j) {
                if (j > 0) {
                    std::cout << "; ";
                }

                const int x = rectangles[i][j].x;
                const int y = image.rows - 1 - rectangles[i][j].y;
                std::cout << '(' << x << ", " << y << ')';
            }
        }

        std::cout << '\n';
    }

    return 0;
}
