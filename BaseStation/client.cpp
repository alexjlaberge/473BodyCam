#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <fstream>
#include <string>

#include <opencv2/opencv.hpp>

#include "client.hpp"
#include "pkt.hpp"

#define RECV_BUF_SIZE 1024
#define POLL_DELAY 100
#define DISCONNECTED_IMAGE_TIMEOUT 50

using bodycam::Client;
using bodycam::Packet;
using bodycam::PacketType;
using bodycam::ParseError;
using cv::Mat;
using cv::Size;
using cv::imdecode;
using cv::imread;
using std::cout;
using std::endl;
using std::lock_guard;
using std::mutex;
using std::ofstream;
using std::runtime_error;
using std::string;
using std::to_string;
using std::vector;

volatile static bool running = true;

void listener(Client *client);
Mat assembleFrame(const vector<Packet> &pkts, ofstream &out);

void listener(Client *client)
{
    int sd;
    int err;
    int ntimeouts{DISCONNECTED_IMAGE_TIMEOUT};
    ofstream output_file{"data.raw"};
    struct sockaddr_in skaddr;
    char buf[RECV_BUF_SIZE];  
    vector<Packet> pkts;
    vector<uint8_t> data;
    struct pollfd pfd;
    Mat frame;

    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd < 0)
    {
        perror("Problem creating socket");
        return;
    }

    skaddr.sin_family = AF_INET;
    skaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    skaddr.sin_port = htons(8888);

    err = bind(sd, (struct sockaddr *) &skaddr, sizeof(skaddr));
    if (err < 0)
    {
        perror("Problem binding");
        return;
    }

    pfd.fd = sd;
    pfd.events = POLLIN;
    while (running)
    {
        err = poll(&pfd, 1, POLL_DELAY);
        if (err == 0)
        {
            ntimeouts++;
            if (ntimeouts > DISCONNECTED_IMAGE_TIMEOUT)
            {
                client->gotDisconnected();
            }

            continue;
        }
        ntimeouts = 0;

        err = recvfrom(sd, buf, RECV_BUF_SIZE, 0, nullptr, nullptr);
        if (err == -1)
        {
            perror("recvfrom() failed");
            break;
        }

        for (int i = 0; i < err; i++)
        {
            data.push_back(buf[i]);
        }

        try
        {
            Packet pkt;
            vector<uint8_t> new_data;

            pkt.parse(data.data(), data.size());

            if (pkts.size() > 0 && pkt.getID() != pkts[0].getID())
            {
                Mat tmp = assembleFrame(pkts, output_file);
                if (!tmp.empty())
                {
                    client->displayImage(tmp);
                    client->setFrameID(pkts[0].getID());
                }
                pkts.clear();
            }

            pkts.push_back(pkt);

            if (pkt.getRawLength() < data.size())
            {
                vector<uint8_t> new_data(
                        data.begin() + pkt.getRawLength(),
                        data.end()
                        );
                data = new_data;
            }
            else
            {
                data.clear();
            }
        }
        catch (const ParseError &e)
        {
        }
    }
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    Client c;

    QObject::connect(&app, SIGNAL(lastWindowClosed()), &c, SLOT(quit()));

    c.show();

    return app.exec();
}

Client::Client()
{
    QHBoxLayout *lay;
    QGroupBox *center;

    center = new QGroupBox();
    lay = new QHBoxLayout();
    video = new QLabel();
    lay->addWidget(video);

    frameID = new QLabel(
            QString::fromStdString(string{"Frame ID: "} + to_string(0)));
    lay->addWidget(frameID);

    connect(this, SIGNAL(gotNewImage()), this, SLOT(draw()));

    center->setLayout(lay);
    setCentralWidget(center);

    disconnected = imread("disconnected-4x3-160x120.png");

    future = QtConcurrent::run(listener, this);
}

void Client::setFrameID(unsigned int id)
{
    frameID->setText(
            QString::fromStdString(string{"Frame ID: "} + to_string(id)));
    return;
}

void Client::displayImage(const Mat &image)
{
    {
        lock_guard<mutex> g(imLock);
        im = image.clone();
    }

    emit gotNewImage();
}

void Client::gotDisconnected()
{
    displayImage(disconnected);
}

void Client::draw()
{
    Mat tmp;

    if (im.empty())
    {
        im = Mat{Size{160, 120}, CV_8UC3};
        return;
    }

    {
        lock_guard<mutex> g(imLock);
        cvtColor(im, tmp, CV_BGR2BGRA);
    }

    QImage buf(tmp.data, tmp.cols, tmp.rows, QImage::Format_ARGB32);

    QPixmap pmap = QPixmap::fromImage(buf);

    video->setPixmap(pmap);
}

void Client::quit()
{
    running = false;
    future.waitForFinished();
}

Mat assembleFrame(const vector<Packet> &pkts, ofstream &out)
{
    Mat frame;
    vector<uint8_t> raw;
    vector<uint8_t> mjpeg;
    bool in_mjpeg{false};

    for (const auto &pkt : pkts)
    {
        size_t size = pkt.getOffset() + pkt.getLength();

        if (size > raw.size())
        {
            raw.resize(size);
        }

        for (size_t i = 0; i < pkt.getLength(); i++)
        {
            raw[pkt.getOffset() + i] = pkt.getData()[i];
        }
    }

    if (pkts[0].getType() == PacketType::MJPEG)
    {
        for (size_t j = 0; j < (raw.size() - 1); j++)
        {
            if (raw[j] == 0xFF && raw[j + 1] == 0xD8)
            {
                mjpeg.push_back(raw[j]);
                in_mjpeg = true;
            }
            else if (raw[j] == 0xFF && raw[j + 1] == 0xD9)
            {
                mjpeg.push_back(raw[j]);
                mjpeg.push_back(raw[j + 1]);
                break;
            }
            else if (in_mjpeg)
            {
                mjpeg.push_back(raw[j]);
            }
        }
    }

    switch (pkts[0].getType())
    {
    case PacketType::UNCOMP:
        out.write(reinterpret_cast<char *>(raw.data()), raw.size());
        frame = Mat(Size(160, 120), CV_8UC2, raw.data());
        cvtColor(frame, frame, CV_YUV2BGR_YUY2);
        break;
    case PacketType::MJPEG:
        try
        {
            out.write(reinterpret_cast<char *>(mjpeg.data()), mjpeg.size());
            frame = imdecode(mjpeg, 1);
        }
        catch (const std::exception &e)
        {
            frame = Mat{};
        }
        break;
    default:
        break;
    }

    return frame;
}
