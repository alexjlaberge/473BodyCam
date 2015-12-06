#include <fstream>
#include <iostream>
#include <string>

#include <opencv2/opencv.hpp>

using cv::Mat;
using cv::imdecode;
using cv::imshow;
using cv::namedWindow;
using cv::waitKey;
using std::cout;
using std::endl;
using std::ifstream;
using std::string;
using std::vector;

int main(int argc, char **argv)
{
        bool in_mjpeg{false};
        ifstream input;
        string fname;
        vector<uint8_t> mjpeg;
        vector<uint8_t> raw;

        if (argc != 2)
        {
                cout << "Usage: " << argv[0] << " <video>" << endl;
                return 1;
        }

        fname = string{argv[1]};
        input.open(fname);

        while (input.good())
        {
                raw.push_back(input.get());
        }

        namedWindow("thingy");

        for (size_t j = 0; j < (raw.size() - 1); j++)
        {
                if (!in_mjpeg && raw[j] == 0xFF && raw[j + 1] == 0xD8)
                {
                        mjpeg.push_back(raw[j]);
                        in_mjpeg = true;
                }
                else if (in_mjpeg && raw[j] == 0xFF && raw[j + 1] == 0xD9)
                {
                        mjpeg.push_back(raw[j]);
                        mjpeg.push_back(raw[j + 1]);
                        j++;

                        Mat tmp = imdecode(mjpeg, 1);
                        imshow("thingy", tmp);
                        waitKey(100);

                        mjpeg.clear();

                        in_mjpeg = false;
                }
                else if (in_mjpeg)
                {
                        mjpeg.push_back(raw[j]);
                }
        }

        return 0;
}
