#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <chrono>
#include "imgCrawler.h"

using namespace cv;
using namespace std;
using namespace std::chrono;

int main(int argc, char** argv)
{
	String imageToLoad = "";
	int eps1 = 3;
	double eps2 = 0.08;
	bool debug = false;
	int color = 0;
	long timeLimit = 0;
	if ( (argc < 2) || (argc > 7))
	{
		cout << " Usage: imgCrawler.exe ImageToLoadAndDisplay [0 < eps1 < 1,000,000] [0.00 < eps2 < 1.00] [debug = 0|1] [0<=color<=255] [0<timeLimit(milliseconds)<1000000000]" << endl;
		return -1;
	}
	switch (argc) {
	case 7:
		if (strtol(argv[6], NULL, 10) > 0 && strtol(argv[6], NULL, 10) < 1000000000)
			timeLimit = strtol(argv[6], NULL, 10);
	case 6:
		if (strtol(argv[5], NULL, 10) != 0)
			color = strtol(argv[5], NULL, 10);
	case 5:
		if (strtol(argv[4], NULL, 10) == 1)
			debug = true;
	case 4:
		if (strtod(argv[3], NULL) > 0 && strtod(argv[3], NULL) < 1)
			eps2 = strtod(argv[3], NULL);
	case 3:
		if (strtol(argv[2], NULL, 10) > 0 && strtol(argv[2], NULL, 10) < 1000000)
			eps1 = strtol(argv[2], NULL, 10);
	case 2:
		imageToLoad = argv[1];
	default:
		break;
	}

	Mat image;
	image = imread(imageToLoad, CV_LOAD_IMAGE_GRAYSCALE); // Read the file
	MVEE::imgCrawler imgCrwl(image);

	if (!image.data) // Check for invalid input
	{
		cout << "Could not open or find the image" << endl;
		return -1;
	}

	//Print Image matrix
	cout << "M = " << image.cols << " x " << image.cols << endl;
	/*
	for (int i = 0; i < image.rows; i+=5) {
		for (int  j = 0; j < image.cols; j+=5) {
			Scalar colour = image.at<uchar>(Point(j, i));
			cout << " ";
			if (colour.val[0] < 100)
				cout << 0;
			cout << (int)colour.val[0]/10;
		}
		cout << endl;
	}*/
	long startTime, endTime;
	startTime = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	std::cout << "Started at " << startTime << std::endl;
	if(debug)
		imgCrwl.setDebug(true);
	imgCrwl.run(color, eps1, eps2,timeLimit);
	imgCrwl.test();
	imgCrwl.printState();
	endTime = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	std::cout << "Ended at "<< endTime<<" After delta " <<endTime - startTime<< std::endl;
	waitKey(0); // Wait for a keystroke in the window
	return 0;
}