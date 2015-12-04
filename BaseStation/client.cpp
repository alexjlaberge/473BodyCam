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

#include <opencv2/opencv.hpp>

#include "client.hpp"
#include "pkt.hpp"

#define RECV_BUF_SIZE 1024

using bodycam::Client;
using bodycam::Packet;
using bodycam::ParseError;
using cv::Mat;
using cv::Size;
using std::cout;
using std::endl;
using std::lock_guard;
using std::mutex;
using std::runtime_error;
using std::vector;

vector<char> data(38400);
volatile static bool running = true;

void listener(Client *client);
Mat assembleFrame(const vector<Packet> &pkts);

void listener(Client *client)
{
    int sd;
    int err;
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
        size_t i{0};

        err = poll(&pfd, 1, 1000);
        if (err == 0)
        {
            continue;
        }

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

        while (i < data.size())
        {
            Packet pkt;

            try
            {
                pkt.parse(data.data() + i, data.size() - i);
            }
            catch (const ParseError &e)
            {
                break;
            }

            if (pkt.getID() != pkts[0].getID())
            {
                client->displayImage(assembleFrame(pkts));
                pkts.clear();
            }

            pkts.push_back(pkt);
            i += pkt.getRawLength();
        }

        data.clear();
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
    QHBoxLayout *layout;

    layout = new QHBoxLayout();
    video = new QLabel();
    layout->addWidget(video);

    connect(this, SIGNAL(gotNewImage()), this, SLOT(draw()));

    setLayout(layout);

    future = QtConcurrent::run(listener, this);
}

void Client::displayImage(const Mat &image)
{
    {
        lock_guard<mutex> g(imLock);
        im = image.clone();
    }

    emit gotNewImage();
}

void Client::draw()
{
    Mat tmp;

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

Mat assembleFrame(const vector<Packet> &pkts)
{
    Mat frame;
    vector<uint8_t> raw;

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

    frame = Mat(Size(160, 120), CV_8UC2, data.data());
    cvtColor(frame, frame, CV_YUV2BGRA_YUY2);

    return frame;
}
