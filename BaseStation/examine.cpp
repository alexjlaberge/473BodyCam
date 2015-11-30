#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <array>
#include <fstream>
#include <iostream>

#include <opencv2/opencv.hpp>

#include "common.hpp"

using cv::Mat;
using cv::Size;
using cv::cvtColor;
using cv::imshow;
using cv::waitKey;
using std::array;
using std::cerr;
using std::dec;
using std::endl;
using std::hex;
using std::ifstream;

static const char *net_img_start = NET_IMG_START;
static const char *net_img_end = NET_IMG_END;

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        cerr << "Usage: " << argv[0] << " <raw_file>" << endl;
        return 1;
    }

    ifstream in{argv[1]};
    struct stat st;
    char *buf{nullptr};
    size_t file_size;
    size_t i{0};
    array<char, 160 * 120 * 2> raw;

    if (stat(argv[1], &st) != 0)
    {
        cerr << "Could not stat " << argv[1] << endl;
        return 1;
    }

    file_size = static_cast<size_t>(st.st_size);
    buf = new char[file_size];
    in.read(buf, file_size);

    if (static_cast<size_t>(in.tellg()) != file_size)
    {
        cerr << "Did not reach EOF" << endl;
        cerr << "read " << in.tellg() << "/" << file_size << endl;
        cerr << "file is good: " << static_cast<int>(in.good()) << endl;
        return 1;
    }

    while (i < file_size)
    {
        uint16_t len{0};
        uint8_t id{0};
        uint32_t offset{0};

        cerr << "i: " << i << " ";

        len = *reinterpret_cast<uint16_t *>(buf + i);
        i += sizeof(uint16_t);

        id = *reinterpret_cast<uint8_t *>(buf + i);
        i += sizeof(uint8_t);

        offset = *reinterpret_cast<uint32_t *>(buf + i);
        i += sizeof(uint32_t);

        cerr << "id: " << static_cast<int>(id) << " ";
        cerr << "len: " << len << " ";
        cerr << "offset: " << offset << endl;

        if (id == 1)
        {
            for (size_t j = 0; j < static_cast<size_t>(len); j++)
            {
                raw[offset + j] = buf[i + j];
            }
        }

        i += static_cast<size_t>(len);
    }

    cerr << hex;
    for (size_t j = 0; j < 128; j++)
    {
        if (j > 0 && (j % 4) == 0)
        {
            cerr << endl;
        }
        cerr << (static_cast<unsigned int>(raw[j + 38268]) & 0xFF) << " ";
    }
    cerr << endl;

    Mat img{Size(160, 120), CV_8UC2, raw.data()};
    Mat dst{Size(160, 120), CV_8UC3};

    //cvtColor(img, dst, CV_YUV2GRAY_UYVY);
    //cvtColor(img, dst, CV_YUV2GRAY_YUY2);
    //cvtColor(img, dst, CV_YUV2BGR_UYVY);
    //cvtColor(img, dst, CV_YUV2BGRA_UYVY);
    cvtColor(img, dst, CV_YUV2BGR_YUY2);
    //cvtColor(img, dst, CV_YUV2BGR_YVYU);
    //cvtColor(img, dst, CV_YUV2BGRA_YUY2);
    //cvtColor(img, dst, CV_YUV2BGRA_YVYU);
    //cvtColor(img, dst, CV_YUV2GRAY_NV12);
    //cvtColor(img, dst, CV_YUV2BGR_NV12);

    imshow("Police Video", dst);
    waitKey(0);

    return 0;
}
