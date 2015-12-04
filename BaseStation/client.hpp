#ifndef BODYCAM_CLIENT_HPP
#define BODYCAM_CLIENT_HPP

#include <mutex>

#include <QtGui/QtGui>

#include <opencv2/opencv.hpp>

namespace bodycam
{

class Client : public QMainWindow
{
	Q_OBJECT

public:
	/**
	 * @param filename Either a video device or video file to display.
	 */
	Client();

	void displayImage(const cv::Mat &image);

	bool isRunning() const;


public slots:
	void draw();
	void quit();

signals:
	void gotNewImage();

private:
	cv::Mat im;
	std::mutex imLock;
	QFuture<void> future;

	// Qt UI widgets
	QLabel *video;
	QLabel *frameID;
};

}

#endif /* BODYCAM_CLIENT_HPP */
