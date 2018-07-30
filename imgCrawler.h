#pragma once
#ifndef MVEE_IMGCRAWLER_H
#define MVEE_IMGCRAWLER_H
#include <opencv2/core/core.hpp>
#include <cmath>
#include <iostream>
using namespace cv;

namespace MVEE {
	#define PI 3.14159265
	#define CRAWLER_MODE_EDGE 0
	#define CRAWLER_MODE_LINE 1
	//Bellow are points relative to current position. You can look at it as: 8 1 2
	//																		 7 0 3
	//																		 6 5 4
	#define POS_CURR 0
	#define POS_N 1
	#define POS_NE 2
	#define POS_E 3
	#define POS_SE 4
	#define POS_S 5
	#define POS_SW 6
	#define POS_W 7
	#define POS_NW 8
	/*Used to crawl the (2d grayscale) image - */
	class imgCrawler
	{

	private:
		Mat image;			//Image to crawl
		Point startingLoc;	//Starting location
		Point currLoc;		//Current location
		Point tempLoc;		//Temporary location
		Point* p;			//Dynamic array of points
		int pointArrSize;	//Just to make it easier to keep track of P's size
		int pointArrCounter;//Keeps track of our current number of elements in p
		int color;			//Current color
		double movementAngle;		//The angle at which we are moving-normilized to the first quadrant (0-90).
									/*int approxAngleLimit;	//The limit for the array length of the array apporximating the line in needed angle. --------- EDIT: Useless atm, but I'll keep it for possible future use ---------
									Pointer to the approximation array. The structure is:
									[CRAWLER_HORIZONTAL (mainDirection), Main Step Size, (repeats until) 1st correction, 2nd correction, ...]
									What it means is that if, for example, we have angle 70, we build [3,39,62] (meaining approxAngleLimit = 3), and angleQuadrant = 1.
									We move 3 pixels up (main direction), 1 pixel right (secondary direction), and repeat it 39 times until we move 1 extra pixel up.
									After we repeated the above proccess 62 times, we would move 2 extra pixels up once.
									The farther our initial ratio is from a round number (up until 0.5), the lower the prescision of our line given a static constant approximation array limit.

									int* approxArray;*/
		int mode;				//CRAWLER_MODE_EDGE or CRAWLER_MODE_LINE - currently CRAWLER_MODE_EDGE is the only implemented one

		bool findStartPointInLine(int color, int eps, int x, int y, int d, bool col);
		bool findStartPointByQuadrats(int color, int eps, int x, int y, int dx, int dy);
	public:
		imgCrawler::imgCrawler();
		imgCrawler::imgCrawler(Mat image);/*, int approxAngleLimit)*/
		imgCrawler::~imgCrawler();
		void imgCrawler::setAngle(Point from, Point to);
		bool imgCrawler::run(int color, int eps1, int eps2, int x = 0, int y = 0);
		bool imgCrawler::findStartPoint(int color, int eps, int x = 0, int y = 0);
		bool imgCrawler::findCorner(int num);
		bool imgCrawler::inShape(int where = 0);
		bool imgCrawler::moveCurrent(int whereTo);
		Point imgCrawler::getPointAt(int whereTo = 0);
		bool imgCrawler::checkPointLegal(Point p);
		int* imgCrawler::getAngleDirections();
		double getAngleData(bool convertToRad = true);
		void imgCrawler::expandP();
		float imgCrawler::pointDist(Point p1, Point p2);
		void imgCrawler::printState();
		/*int* getApproxArray();*/
	};

}


#endif  // MVEE_IMGCRAWLER_H