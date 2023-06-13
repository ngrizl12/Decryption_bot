#include <opencv2/opencv.hpp>
cv::Mat loadImage(const std::string& imagePath);
std::string decrypt_image(const std::string& path, const std::string file_name, int quantizationStep);
int crypt_image(const std::string& path, const std::string& file_name, const std::string open_text, int quantizationStep);
std::string jpg_to_png(const std::string& path, const std::string& file_name);