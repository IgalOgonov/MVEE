#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include "imgCrawler.h"

using namespace cv;
using namespace std;

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		cout << " Usage: display_image ImageToLoadAndDisplay" << endl;
		return -1;
	}

	Mat image;
	image = imread(argv[1], CV_LOAD_IMAGE_GRAYSCALE); // Read the file
	MVEE::imgCrawler imgCrwl(image);
	double a = 2.654;
	int b = (int)a;
	cout << a << " minus " << b << " is " << (a - b) << endl;
	cout << a << " modulo " << b << " is " << fmod(a,b) << endl;

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
	imgCrwl.setDebug(true);
	imgCrwl.run(0, 3, 0.01);
	imgCrwl.test();
	imgCrwl.printState();
	waitKey(0); // Wait for a keystroke in the window
	return 0;
}