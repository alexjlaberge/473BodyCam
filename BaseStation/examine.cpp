#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <array>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <opencv2/opencv.hpp>

#include "common.hpp"
#include "pkt.hpp"

using bodycam::Packet;
using bodycam::PacketType;
using bodycam::ParseError;
using cv::Mat;
using cv::Size;
using cv::cvtColor;
using cv::imshow;
using cv::waitKey;
using std::array;
using std::cout;
using std::dec;
using std::endl;
using std::hex;
using std::ifstream;
using std::ofstream;
using std::string;
using std::vector;

static const char *net_img_start = NET_IMG_START;
static const char *net_img_end = NET_IMG_END;

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        cout << "Usage: " << argv[0] << " <raw_file> [frame_id]" << endl;
        return 1;
    }

    ifstream in{argv[1]};
    struct stat st;
    char *buf{nullptr};
    size_t file_size;
    size_t i{0};
    array<char, 160 * 120 * 2> raw;
    vector<Packet> pkts;
    vector<uint8_t> ids;

    if (stat(argv[1], &st) != 0)
    {
        cout << "Could not stat " << argv[1] << endl;
        return 1;
    }

    file_size = static_cast<size_t>(st.st_size);
    buf = new char[file_size];
    in.read(buf, file_size);

    if (static_cast<size_t>(in.tellg()) != file_size)
    {
        cout << "Did not reach EOF" << endl;
        cout << "read " << in.tellg() << "/" << file_size << endl;
        cout << "file is good: " << static_cast<int>(in.good()) << endl;
        return 1;
    }

    uint8_t id = 0;
    while (i < file_size)
    {
        Packet tmp;

        try
        {
            tmp.parse(reinterpret_cast<uint8_t*>(buf + i), file_size - i);
        }
        catch (const ParseError &e)
        {
            cout << "Caught a parsing error - \"" << e.getMessage() << "\"" <<
                endl;
            break;
        }

        if (id != tmp.getID())
        {
            id = tmp.getID();
            cout << "id: " << static_cast<int>(tmp.getID()) << " ";
            cout << "len: " << static_cast<int>(tmp.getLength()) << endl;
        }
        pkts.push_back(tmp);

        i += tmp.getRawLength();
    }

    cout << "Parsed " << pkts.size() << " packets" << endl;

    for (int k = 2; k < argc; k++)
    {
        ids.push_back(static_cast<uint8_t>(std::stoi(string{argv[k]})));
    }

    if (ids.size() == 0)
    {
        return 0;
    }

    cout << "examining IDs ";
    for (const auto &id : ids)
    {
        cout << static_cast<int>(id) << " ";
    }
    cout << endl;

    for (const auto &id : ids)
    {
        vector<uint8_t> raw;
        PacketType type;

        cout << "examining " << static_cast<int>(id) << endl;

        for (const auto &pkt : pkts)
        {
            if (pkt.getID() == id)
            {
                size_t new_length = pkt.getOffset() +
                    static_cast<size_t>(pkt.getLength());

                type = pkt.getType();

                if (new_length > raw.size())
                {
                    raw.resize(new_length, 0);
                }

                for (size_t k = 0; k < pkt.getLength(); k++)
                {
                    raw[pkt.getOffset() + k] = pkt.getData()[k];
                }
            }
        }

        cout << "type: " << type << endl;

        cout << hex;
        for (size_t j = 0; j < 512; j++)
        {
            if (j > 0 && (j % 32) == 0)
            {
                cout << endl;
            }
            cout << (static_cast<unsigned int>(raw[j]) & 0xFF) << " ";
        }
        cout << endl;

        Mat img{Size(160, 120), CV_8UC3, raw.data()};
        Mat dst;

        //cvtColor(img, dst, CV_YUV2GRAY_UYVY);
        //cvtColor(img, dst, CV_YUV2GRAY_YUY2);
        //cvtColor(img, dst, CV_YUV2BGR_UYVY);
        //cvtColor(img, dst, CV_YUV2BGRA_UYVY);
        //cvtColor(img, dst, CV_YUV2BGR_YUY2);
        //cvtColor(img, dst, CV_YUV2BGR_YVYU);
        //cvtColor(img, dst, CV_YUV2BGRA_YUY2);
        //cvtColor(img, dst, CV_YUV2BGRA_YVYU);
        //cvtColor(img, dst, CV_YUV2GRAY_NV12);
        //cvtColor(img, dst, CV_YUV2BGR_NV12);
        cvtColor(img, dst, CV_YCrCb2BGR);

        imshow("Police Video", dst);
        waitKey(0);

        ofstream out_jpeg{"out.jpg", ofstream::binary};
        for (size_t j = 0; j < raw.size(); j++)
        {
            if (raw[j] == 0xFF && raw[j + 1] == 0xD8)
            {
                cout << "hit the start" << endl;
                out_jpeg.write(reinterpret_cast<char *>(raw.data() + j), 2);
                j++;
            }
            else if (raw[j] == 0xFF && raw[j + 1] == 0xDB)
            {
                cout << "hit the start 2" << endl;
                out_jpeg.write(reinterpret_cast<char *>(raw.data() + j), 2);
                j++;
            }
            else if (raw[j] == 0xFF && raw[j + 1] == 0xC0)
            {
                cout << "hit the start 3" << endl;
                out_jpeg.write(reinterpret_cast<char *>(raw.data() + j), 2);
                j++;
            }
            else if (raw[j] == 0xFF && raw[j + 1] == 0xD9)
            {
                cout << "hit the end" << endl;
                out_jpeg.write(reinterpret_cast<char *>(raw.data() + j), 2);
                return 0;
            }
            else
            {
                out_jpeg.write(reinterpret_cast<char *>(raw.data() + j), 1);
            }
        }
    }

    return 0;
}
