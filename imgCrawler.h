/*VERSION 0.1.0.2*/

#pragma once
#ifndef MVEE_IMGCRAWLER_H
#define MVEE_IMGCRAWLER_H
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <cmath>
#include <iostream>
#include <chrono>
using namespace cv;
using namespace std;
using namespace std::chrono;

namespace MVEE {
	#define PI 3.14159265
	#define CORNER_POINT_LIMIT = 1000; 
	#define CRAWLER_MODE_EDGE 0
	#define CRAWLER_MODE_LINE 1
	//Bellow are points relative to current position. You can look at it as: 8 1 2 or in the real world 6 5 4 since y starts from the upper left corner
	//																		 7 0 3						7 0 3
	//																		 6 5 4						8 1 2
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
		Mat Q;				//Used in the algorithm
		Point startingLoc;	//Starting location
		Point currLoc;		//Current location
		Point tempLoc;		//Temporary location
		Point* p;			//Dynamic array of points
		float* u;			//Dynamic array of point weights
		bool debug;			//Debug mode active.
		int pixelCounter;	//Used for debugging
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
		void imgCrawler::setAngle(Point from, Point to);							//Sets movement angle
		bool imgCrawler::run(int color, int eps1, double eps2, long timeLimit, int x = 0, int y = 0);	//Runes the algorithm with color being 0-255 color of our shape, eps1 being smallest square to search, eps2 from the alg. 
		bool imgCrawler::findStartPoint(int color, int eps, int x = 0, int y = 0);	//Finds starting point to run from in the picture. 
		bool imgCrawler::findCorner(int num);										//Finds corner from starting point. Will place it into index num in the point array.
		bool imgCrawler::jumpToBorder(Mat tempMat = Mat::zeros(1,1,CV_32F), bool ellipseBorder = false);		//Jumps to the shape border from current location - can pain progress on tempMat if it's given.
		bool imgCrawler::crawlToCorner(Mat tempMat = Mat::zeros(1, 1, CV_32F), bool elipsDist = false);//Crawls to closest corner in the legal directions
		bool imgCrawler::handleBump(bool elipsDist = false);						//Handles line approximation making the shape not convex
		bool imgCrawler::inShape(int where = 0, Point p = Point(-1,-1));			//Returns true if pointAt(where) is insiide the shape
		bool imgCrawler::moveCurrent(int whereTo);									//Moves current location to WhereTo
		Point imgCrawler::getPointAt(int whereTo = 0, Point start = Point(-1, -1));	//Gets the point relative to current location
		bool imgCrawler::checkPointLegal(Point p);									//Checks if a point is within matrix boundries
		bool imgCrawler::checkBorderPoint(Point p = Point(-1,-1), bool ellipse = false);//Checks if a point is a border point
		int* imgCrawler::getAngleDirections(int angle = 361);						//Helper function that returns an array of legal directions depending on the current angle
		double getAngleData(bool convertToRad = true);								//Helper function that returns current angle - converts it to radians by defult.
		void imgCrawler::expandArr();													//Expands dynamic array P
		void imgCrawler::calcQ();													//Calculates Q
		float imgCrawler::pointDist(Point p1, Point p2);							//Returns distance between points.
		float imgCrawler::ellipseDist(Point p);										//Returns distance from point p to the  ellipsoid.
		void imgCrawler::printState();												//Prints current state of the crawler.
		void imgCrawler::setDebug(bool val);										//Sets the crawler into debug mode.
		/*int* getApproxArray();*/
		void imgCrawler::test();													//General testing
		void imgCrawler::drawCross(cv::Mat image, Point p);							//Debugging
		void imgCrawler::slowEllipseDraw(int color = 100);							//Slow and innacurate drawing of the ellipsoid
		void imgCrawler::drawNearbyEllipse(Point p, int color);						//Recursive function to draw nearby elipse pixels
		float* imgCrawler::checkDistNearMe(bool onlyBorder = true, bool onlyInShape = true, bool toPrint = true);
	};

}


#endif  // MVEE_IMGCRAWLER_H