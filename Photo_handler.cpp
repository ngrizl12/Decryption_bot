#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <bitset>
#include <opencv2/opencv.hpp>

//Helper for load img
cv::Mat loadImage(const std::string& imagePath) {
    cv::Mat image = cv::imread(imagePath);
    if (image.empty()) {
        std::cout << "Не удалось загрузить изображение." << std::endl;
    }
    return image;
}
//Decrypt handler
std::string decrypt_image(const std::string& path, const std::string file_name, int quantizationStep) {
    std::string path_in = path + file_name;

    if (quantizationStep % 2 != 0) {
        std::cout << "Шаг квантования должен быть четным числом!" << std::endl;
        return "";
    }

    cv::Mat image = cv::imread(file_name);

    if (image.empty()) {
        std::cout << "Не удалось загрузить изображение." << std::endl;
        return "";
    }

    int width = image.cols;
    int height = image.rows;

    std::string message;
    std::vector<char> messageBits;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            cv::Vec3b pixel = image.at<cv::Vec3b>(y, x);
            for (int channel = 0; channel < 3; channel++) {
                uint8_t channelValue = pixel[channel];
                char bit = ((channelValue % quantizationStep) >= (quantizationStep / 2)) ? '1' : '0';
                messageBits.push_back(bit);
            }
        }
    }

    for (int i = messageBits.size() - 1; i >= 0; i -= 8) {
        std::bitset<8> byteBits;
        for (int j = 0; j < 8; j++) {
            byteBits[j] = (i - j >= 0) ? messageBits[i - j] - '0' : 0;
        }
        char byte = byteBits.to_ulong();
        if (byte != 0) {
            message += byte;
        }
    }

    std::reverse(message.begin(), message.end());
    return message;
}
//Crypt handler
int crypt_image(const std::string& path, const std::string& file_name, const std::string open_text, int quantizationStep) {
    const std::string path_in = path + file_name;
    cv::Mat image = cv::imread(file_name);
    if (image.empty()) {
        std::cout << "Не удалось загрузить изображение." << std::endl;
        return 1;
    }
    int width = image.cols;
    int height = image.rows;
    int channels = image.channels();
    int totalPixels = width * height * channels;

    std::string message = open_text;
    std::vector<int32_t> messageBytes(message.begin(), message.end());
    if (messageBytes.size() > totalPixels) {
        std::cout << "Количество байт " << messageBytes.size() << " больше количества элементов RGB " << totalPixels << "." << std::endl;
        return 1;
    }

    if (messageBytes.size() < totalPixels) {
        messageBytes.resize(totalPixels, 0);
    }

    std::string binaryMessage;
    for (int32_t byte : messageBytes) {
        std::string byteString = std::bitset<8>(byte).to_string();
        binaryMessage += byteString;
    }

    if (quantizationStep % 2 != 0) {
        std::cout << "Шаг квантования должен быть четным числом!" << std::endl;
        return 1;
    }

    int messageIndex = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            cv::Vec3b& pixel = image.at<cv::Vec3b>(y, x);
            for (int c = 0; c < channels; c++) {
                int value = pixel[c];
                int quantizedValue = quantizationStep * (value / quantizationStep) + (quantizationStep / 2) * (binaryMessage[messageIndex++] - '0');
                pixel[c] = static_cast<int32_t>(quantizedValue);
            }
        }
    }

    cv::imwrite("output_" + file_name, image);
    return 0;
}
//Helper for jpg
std::string jpg_to_png(const std::string& path, const std::string& file_name) {
    cv::Mat prev_file = cv::imread(path + file_name);
    std::string new_file = file_name;
    new_file = new_file.substr(0, new_file.size() - 4) + ".png";
    imwrite(new_file, prev_file, { cv::IMWRITE_PNG_COMPRESSION, 9 });
    remove(file_name.c_str());
    new_file = new_file.substr(0, new_file.size() - 4) + ".PNG";
    remove(file_name.c_str());
    return new_file;
}