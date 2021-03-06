#include "imgCrawler.h"


namespace MVEE {
	//Empty Constructor
	imgCrawler::imgCrawler()
	{
	}

	//Actual Constructor
	imgCrawler::imgCrawler(Mat image)/*, int approxAngleLimit)*/
	{
		this->image = image;							//Initialize image
		this->startingLoc = Point(0, 0);
		this->pointArrSize = 4;
		this->pointArrCounter = 0;
		this->pixelCounter = 0;
		/*this->approxAngleLimit = approxAngleLimit;		//Set approximation limit
		this->approxArray = new int[approxAngleLimit];	//initialize ApproxArray based on said limit'*/
		this->p = new Point[this->pointArrSize];
		this->u = new float[this->pointArrSize];
	}

	//Destructor
	imgCrawler::~imgCrawler()
	{
		/*delete[] this->approxArray;
		this->approxArray = NULL;*/
	}

	//Calculate angle between 2 points and set the normilized angle, the approximation array and the quadrant.
	void imgCrawler::setAngle(Point from, Point to) {

		double angle = 0;
		double x1 = from.x;
		double x2 = to.x;
		double y1 = from.y;
		double y2 = to.y;

		//Then, calculate the angle normalized to the first quadrant
		double dx = x2 - x1;
		double dy = y2 - y1;
		//Calculate angle
		if (dy != 0 && dx != 0) {
			this->movementAngle = atan(dy / dx) * 180 / PI;
			if (dx < 0) {						//Remember - angle is mirrored vertically due to image representation
				this->movementAngle < 0 ? this->movementAngle = (180 + this->movementAngle)*(-1) : this->movementAngle = (180 - this->movementAngle);	//Correct the resaults depending on the quardrant
			}
		}
		else if (dx == 0 && dy != 0) {
			(dy < 0) ? this->movementAngle = -90 : this->movementAngle = 90;
		}
		else if (dx != 0 && dy == 0) {
			(dx < 0) ? this->movementAngle = 180 : this->movementAngle = 0;
		}
		else if (dx == 0 && dy == 0) {
			throw "You can't set an angle from a point to itself!";
		}
		/*---------Pretty useless now, but might need it later
		dx = abs(dx);
		dy = abs(dy);
		//Build the approximation array for the angle, since we are in L1 space
		double ratio = 0;
		double residue = 0;
		if (dy == 0)
		dy = 0.00000001;
		(dx > dy) ? ratio = dx / dy : ratio = dy / dx;			//Initialize the ratio
		this->approxArray[0] = (int)ratio;					//Set the current ratio to be the next one in the array
		for (int i = 1; i < this->approxAngleLimit; i++) {
		residue = fmod(ratio, (int)ratio);				//Calculate how many times we can repeat the current ration before we need to compensate for the loss
		int mul = (int)(1 / residue)+1;					//^
		this->approxArray[i] = mul;						//After approxArray[i] repeats, we'll need to go 1 step in the "main" direction to compensate for the loss.
		ratio *= mul;									//Update the new ratio that we get after approxArray[i] repeats, to calculate the "mistake" left after those repeats.
		}
		*/
	}

	/*The main function. Once it stops, you should have */
	bool imgCrawler::run(int color, int eps1, double eps2, long timeLimit, int x, int y) {
		long startTime, currTime;
		float maxDist;
		int maxIndex;
		startTime = std::chrono::duration_cast<milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

		if (!this->findStartPoint(color, eps1, x, y))			//Find the first pixel of existing color
			return false;
		if (this->debug) {							//DEBUG
			std::cout << "Starting Point:" << this->startingLoc << std::endl;
		}
		Point left(this->startingLoc.x - 1, this->startingLoc.y);
		Point right(this->startingLoc.x + 1, this->startingLoc.y);
		this->setAngle(this->startingLoc, left);						//Set movement to the left to find the first point
		if (!this->findCorner(0))										//Try to find the first point
			return false;
		this->setAngle(this->startingLoc, right);					//Set the angle from the point we found to the starting point
		this->startingLoc.x = (this->p[0].x);		//Update the starting point to be the point we found.
		this->startingLoc.y = (this->p[0].y);		//^
		if (!this->findCorner(1))										//Try to find the 2nd point
			return false;
		this->setAngle(this->p[0], this->p[1]);							//Set the angle to be between the 2 points we found.
		this->startingLoc.x = (this->p[0].x + this->p[1].x) / 2;		//Update the starting point to be between the points we found.
		this->startingLoc.y = (this->p[0].y + this->p[1].y) / 2;		//^
		this->movementAngle += 90;										//Try to find the 3nd point
		if (!this->findCorner(2))
			return false;
		this->movementAngle -= 180;										//Try to find the 4th point
		if (!this->findCorner(3))
			return false;
		//Remember to destroy windows on debug
		if (this->debug)
			cv::destroyAllWindows();

		//Now we should have our 4 initial points, and can run the 2nd part of the algorithm
		double tempAngles[4];											//Find 4 possible angles
		Point tempPoints[4];											//4 possible points
		std::vector<float> oldU(this->pointArrCounter, 8);				//The old u vector - allocate extra space
		cv::Mat svdMat;													//Svd each time
		cv::Mat vMat;
		cv::Mat dMat;
		cv::Mat uMat;
		cv::Mat tempMat;												//Only exists for debugging purposes
		String debugStr = "";											//^
		int debugCounter = 1;

		bool goOn = true;
		while (goOn) {

			//Calculate Q
			this->calcQ();
			//Find 4 directions using svd, and follow them to find the relevant points - all are radians for now
			svdMat = cv::SVD(this->Q, SVD::FULL_UV).vt;
			tempAngles[0] = atan2(svdMat.at<float>(0, 1), svdMat.at<float>(0, 0)) * 180 / PI;					//Radians begone!
			tempAngles[1] = atan2(svdMat.at<float>(1, 1), svdMat.at<float>(1, 0)) * 180 / PI;
			tempAngles[2] = (tempAngles[0] + 180);
			if (tempAngles[2] > 180)
				tempAngles[2] -= 360;
			tempAngles[3] = (tempAngles[1] + 180);
			if (tempAngles[3] > 180)
				tempAngles[3] -= 360;

			//Crawl in each direction, and find the farathest point
			if (this->debug) {							//DEBUG
				std::cout << "Finding farthest point, angles: " << tempAngles[0] << "," << tempAngles[1] << "," << tempAngles[2] << "," << tempAngles[3] << std::endl;
				this->image.copyTo(tempMat);
				tempMat.at<uchar>(this->startingLoc) = 255;
			}

			for (int i = 0; i < 4; i++) {
				tempAngles[i] = tempAngles[i];
				this->movementAngle = tempAngles[i];						//Set angle
				if (!this->jumpToBorder(tempMat)) {							//Find border
					goOn = false;
					break;
				}
				//Try to crawl to the farthest corner
				if (!this->crawlToCorner(tempMat, true)) {
					goOn = false;
					break;
				}
				//Update current location
				tempPoints[i].x = this->currLoc.x;
				tempPoints[i].y = this->currLoc.y;
				this->currLoc.x = this->startingLoc.x;
				this->currLoc.y = this->startingLoc.y;
				if (this->debug) {						//DEBUG
					drawCross(tempMat, tempPoints[i]);
					if (i == 3) {
						debugStr = "";;
						String wName = "Direction " + std::to_string(i) + "_debug";
						cv::namedWindow(wName, WINDOW_NORMAL); // Create a window for display.
						imshow(wName, tempMat); // Show our image inside it.
					}
				}
			}
			if (!goOn)													//It's false only if we failed to get to a border at some point
				break;

			maxDist = 0;
			maxIndex = 0;
			for (int i = 0; i < 4; i++) {
				if (this->ellipseDist(tempPoints[i]) > maxDist) {
					maxIndex = i;
					maxDist = this->ellipseDist(tempPoints[i]);
				}
			}

			if (this->debug) {
				std::cout << "Farthest point is " << tempPoints[maxIndex] << ", with distance " << maxDist << std::endl;
				waitKey(0);					// Wait for a keystroke in the window - to close 4 windows
			}

			//Time to update the points/weights
			float beta = (maxDist - 3) / ((3) * (maxDist - 1));
			if (maxDist < 0)
				break;
			if (goOn) {
				int existingIndex = -1;
				//Update existing weights
				for (int i = 0; i < this->pointArrCounter; i++) {
					oldU[i] = this->u[i];
					this->u[i] *= (1 - beta);
					if (this->p[i].x == tempPoints[maxIndex].x && this->p[i].y == tempPoints[maxIndex].y)
						existingIndex = i;				//Checks if the current corner point happens to be the max distance point we found
				}
				//This is the case where the point we found was already a corner point
				if (existingIndex != -1) {
					this->u[existingIndex] += beta;
				}
				//This is the case where we found a new point
				else {
					oldU[this->pointArrCounter] = 0;
					this->p[pointArrCounter].x = tempPoints[maxIndex].x;
					this->p[pointArrCounter].y = tempPoints[maxIndex].y;
					this->u[pointArrCounter] = beta;
					this->pointArrCounter++;
					if ((this->pointArrCounter + 1) == this->pointArrSize) {
						if (this->debug) {						//DEBUG
							std::cout << "Expanding array!" << std::endl;
							this->expandArr();
						}
					}
				}
			}
			//Check if the distance between the old and current u is smaller than eps2. If it is, exit
			float delta = 0;
			for (int i = 0; i < this->pointArrCounter; i++) {
				delta += pow((this->u[i] - oldU[i]), 2);
			}
			delta = sqrt(delta);

			if (this->debug)
				std::cout << "Delta is " << delta << ", epsilon is " << eps2 << std::endl;

			//Compare delta to eps2
			if (delta <= eps2) {
				goOn = false;

			}
			else {
				goOn = true;
			}

			//Compare time passed if need be
			currTime = std::chrono::duration_cast<milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			if (timeLimit > 0 && !this->debug) {
				if (currTime - startTime > timeLimit)
					goOn = false;
			}
			else if (this->debug)
				std::cout << "timeLimit does not work in debug mode!" << std::endl;
		}
		this->calcQ();

		//Scale Q by eps2 until the farthest point we found so far is inside the ellipse
		dMat = cv::Mat::zeros(2, 2, CV_32F);
		dMat.at<float>(0, 0) = cv::SVD(this->Q, SVD::FULL_UV).w.at<float>(0, 0);
		dMat.at<float>(1, 1) = cv::SVD(this->Q, SVD::FULL_UV).w.at<float>(0, 1);
		vMat = cv::SVD(this->Q, SVD::FULL_UV).vt;
		uMat = cv::SVD(this->Q, SVD::FULL_UV).u;
		//Increase a and b until the farthest point we found 
		float a = 1 / sqrt(dMat.at<float>(0, 0));
		a *= (1 + eps2);
		float b = 1 / sqrt(dMat.at<float>(1, 1));
		b *= (1 + eps2);
		dMat.at<float>(0, 0) = 1 / pow(a, 2);
		dMat.at<float>(1, 1) = 1 / pow(b, 2);
		this->Q = uMat * dMat * vMat;

		//Display the ellipse on the original image

		cv::Size mainAxes;
		float rotAngle;
		Point center = Point(0, 0);
		if (a > b) {
			mainAxes.width = a;
			mainAxes.height = b;
		}
		else {
			mainAxes.width = b;
			mainAxes.height = a;
		}
		(a < b) ?
			rotAngle = atan2(vMat.at<float>(1, 1), vMat.at<float>(1, 0)) * 180 / PI :
			rotAngle = atan2(vMat.at<float>(0, 1), vMat.at<float>(0, 0)) * 180 / PI;


		for (int i = 0; i < this->pointArrCounter; i++) {
			center.x += this->p[i].x * this->u[i];
			center.y += this->p[i].y * this->u[i];
		}

		if (this->debug) {
			std::cout << "Center:" << center << ", rotAngle: " << rotAngle << ", cvSize " << a << "x" << b << "|" << mainAxes.width << "x" << mainAxes.height << std::endl;
		}
		//cv::ellipse(this->image, center, mainAxes, rotAngle, 0, 360,100,1);
		this->slowEllipseDraw();
		cv::namedWindow("Final Image", WINDOW_NORMAL); // Create a window for display.
		imshow("Final Image", this->image); // Show our image inside it.
		if (this->debug)
			for (int i = 0; i < this->pointArrCounter; i++) {
				std::cout << "Distance of point " << this->p[i] << " is " << this->ellipseDist(this->p[i]) << std::endl;
			}
		dMat.release();
		uMat.release();
		vMat.release();
		svdMat.release();
		tempMat.release();
	}

	//Search the image for current color. Will search all squares, from the size of the image, n (if it's nxn) down to eps (the smallest shape should enclose an eps x eps square).
	//Can also accept expected row and/or column to perform a binary search at, first. Given both, it will also check that specific pixel for the needed color.
	bool imgCrawler::findStartPoint(int color, int eps, int x, int y) {
		bool found = false;											//Denotes whether we found our starting point
		if (y != 0 && x != 0) {
			Scalar curColor = this->image.at<uchar>(Point(x, y));
			if (curColor.val[0] == color) {
				this->startingLoc = Point(x, y);
				found = true;
			}
		}
		if (!found && x != 0) {
			found = this->findStartPointInLine(color, eps, x, y, (int)(image.rows / 2), true);
		}
		if (!found && y != 0) {
			found = this->findStartPointInLine(color, eps, x, y, (int)(image.cols / 2), false);
		}
		if (!found) {
			found = this->findStartPointByQuadrats(color, eps, x, y, (int)(image.cols / 2), (int)(image.rows / 2));
		}
		return found;
	}

	//Finds the starting point in a row (or Col, if col = true)
	bool imgCrawler::findStartPointInLine(int color, int eps, int x, int y, int d, bool col) {

		int delta = (int)(d / 2);
		Scalar curColor = this->image.at<uchar>(Point(x, y));

		if (curColor.val[0] == color) {
			this->startingLoc = Point(x, y);
			return true;
		}
		else if (this->startingLoc.x != 0)			//if startingLoc.x isn't 0, it must mean y isn't either, so they were modified
			return true;
		else if (delta < eps)						//If we reached our limit, exit
			return false;
		else {
			if (!col) {
				return this->findStartPointInLine(color, eps, x - delta, y, delta, col) == false ||
					this->findStartPointInLine(color, eps, x + delta, y, delta, col) == false;
			}
			else {
				return this->findStartPointInLine(color, eps, x, y - delta, delta, col) == false ||
					this->findStartPointInLine(color, eps, x, y + delta, delta, col) == false;
			}
		}
	}
	//Finds the starting point in the whole image
	bool imgCrawler::findStartPointByQuadrats(int color, int eps, int x, int y, int dx, int dy) {

		int deltaX = (int)(dx / 2);
		int deltaY = (int)(dy / 2);
		Scalar curColor = this->image.at<uchar>(Point(x, y));

		if (curColor.val[0] == color) {
			this->startingLoc = Point(x, y);
			return true;
		}

		else if (deltaX < eps || deltaY < eps || x<0 || y<0)		//If we reached our limit, exit
			return false;

		else if (this->startingLoc.x != 0)			//if startingLoc.x isn't 0, it must mean y isn't either, so they were modified
			return true;

		else {
			return findStartPointByQuadrats(color, eps, x + deltaX, y + deltaY, deltaX, deltaY) ||
				findStartPointByQuadrats(color, eps, x + deltaX, y - deltaY, deltaX, deltaY) ||
				findStartPointByQuadrats(color, eps, x - deltaX, y + deltaY, deltaX, deltaY) ||
				findStartPointByQuadrats(color, eps, x - deltaX, y - deltaY, deltaX, deltaY);
		}
	}

	//Finds the num-th corner of the shape, and put that point into p[num]
	bool imgCrawler::findCorner(int num) {
		bool res = false;							//Result to return
		cv::Mat tempMat;							//Only exists for debugging purposes
		String tempStr = "";						//^
		float* tempArr;								//^
		if (this->debug) {							//DEBUG
			std::cout << "Finding corner " << num << " at angle " << this->getAngleData(false) << std::endl;
			this->image.copyTo(tempMat);
			tempMat.at<uchar>(this->startingLoc) = 255;
		}

		//Find shape border
		if (!this->jumpToBorder(tempMat)) {
			return false;
		}

		//Try to crawl to the farthest corner
		if (!this->crawlToCorner(tempMat)) {
			res = false;
		}
		//If it worked, update the results
		else {
			this->p[num] = this->currLoc;
			this->u[num] = 1.f / 4;
			this->pointArrCounter++;
			if ((this->pointArrCounter + 1) == this->pointArrSize) {
				if (this->debug) {						//DEBUG
					std::cout << "Expanding array!";
					this->expandArr();
				}
			}
			if (this->debug) {						//DEBUG
				tempStr = "";
				drawCross(tempMat, this->currLoc);
				String wName = "Corner_" + std::to_string(num) + "_debug";
				cv::namedWindow(wName, WINDOW_NORMAL); // Create a window for display.
				imshow(wName, tempMat); // Show our image inside it.
				waitKey(0); // Wait for a keystroke in the window
			}
			res = true;
		}

		tempMat.release();
		return res;
	}

	bool imgCrawler::jumpToBorder(Mat tempMat)
	{
		int bhop = 1;								//This is how much we "hop" each time
		Point prevP;
		double dx = cos(this->getAngleData(true));	//Delta x
		double dy = sin(this->getAngleData(true));	//Delta y
		int leftOverX = 0, leftOverY = 0;			//Leftovers we might need to deal with
		bool toColor = true;
		if (tempMat.cols == 1 && tempMat.rows == 1)
			toColor = false;

		this->currLoc = this->startingLoc;

		//Hop in big jumps, increasing x2 every time, until you leave the shape
		while (this->inShape() && !(this->checkBorderPoint(this->currLoc) && bhop > 2)) {
			prevP.x = this->currLoc.x;
			prevP.y = this->currLoc.y;

			bhop *= 2;

			this->currLoc.x += (int)(bhop*dx);
			if (bhop*dx - int(bhop*dx) >0.00000001)
				this->currLoc.x += 1; //Round up
			this->currLoc.y += (int)(bhop*dy);
			if (bhop*dy - int(bhop*dy) >0.00000001)
				this->currLoc.y += 1; //Round up

									  //If we go bellow 0, there will be leftover we need to deal with
			if (this->currLoc.x < 0) {
				leftOverX = this->currLoc.x;
				this->currLoc.x = 0;
				if (this->debug)
					std::cout << "Leftovers on axe x:" << leftOverX << std::endl;
			}
			if (this->currLoc.y < 0) {
				leftOverY = this->currLoc.y;
				this->currLoc.y = 0;
				if (this->debug)
					std::cout << "Leftovers on axe y:" << leftOverY << std::endl;
			}

			if (this->debug && toColor) {						//DEBUG
				tempMat.at<uchar>(this->currLoc) = 190;
			}
		}

		//Once we leave the shape, return back in with jumps becoming x2 smaller each time
		while (bhop > 1) {
			//If we are at the border, we can exit - it will get fixed in the next step anyway.
			if (this->checkBorderPoint(this->currLoc))
				break;
			bhop /= 2;
			int jumpX = (int)(bhop*dx);
			int jumpY = (int)(bhop*dy);
			if (jumpX == 0 && (dx < 0))
				jumpX = -1;
			if (jumpX == 0 && (dx > 0))
				jumpX = 1;
			if (jumpY == 0 && (dy < 0))
				jumpY = -1;
			if (jumpY == 0 && (dy > 0))
				jumpY = 1;

			if (this->inShape(0)) {
				this->currLoc.x += jumpX;
				this->currLoc.y += jumpY;
			}
			else {
				//We might have leftover on axe x
				if (leftOverX < 0) {
					leftOverX -= (int)(bhop*dx);
					if (leftOverX > 0) {
						this->currLoc.x += leftOverX;
						leftOverX = 0;
					}
				}
				else {
					this->currLoc.x -= jumpX;
				}
				//We might have leftover on axe y
				if (leftOverY < 0) {
					leftOverY -= (int)(bhop*dy);
					if (leftOverY > 0) {
						this->currLoc.y += leftOverY;
						leftOverY = 0;
					}
				}
				else {
					this->currLoc.y -= jumpY;
				}

			}

			if (this->debug && toColor) {						//DEBUG
				tempMat.at<uchar>(this->currLoc) = 145;
			}
		}

		//Because it is possible to end up just outside the border instead of inside, do a small correction
		int timeout = 100;
		while (!(this->inShape(0) && this->checkBorderPoint(this->currLoc))) {
			int jumpX = (int)(bhop*dx);
			int jumpY = (int)(bhop*dy);
			if (jumpX == 0 && (dx < 0))
				jumpX = -1;
			if (jumpX == 0 && (dx > 0))
				jumpX = 1;
			if (jumpY == 0 && (dx < 0))
				jumpY = -1;
			if (jumpY == 0 && (dx > 0))
				jumpY = 1;

			if (!this->inShape(0)) {
				this->currLoc.x -= jumpX;
				this->currLoc.y -= jumpY;
			}
			else {
				this->currLoc.x += jumpX;
				this->currLoc.y += jumpY;
			}
			//Now, we might still be 1 pixel outside the shape (becaus we rounded down!), so check if we are in and if not, go to a neighbour that's in
			bool tempTry = !this->inShape(0);
			while (tempTry) {
				//Check neighbouring pixels, and go to the one that's in the shape and break
				for (int i = 1; i < 9; i++) {
					if (this->inShape(i)) {
						this->moveCurrent(i);
						tempTry = false;
						break;
					}
				}
				//It's possible that we are 2 pixels away due to rounding - then, just go to the nearest border pixel
				if (tempTry)
					for (int i = 1; i < 9; i++) {
						if (this->checkBorderPoint(this->getPointAt(i))) {
							this->moveCurrent(i);
						}
						else {
							tempTry = false;
						}
					}

				if (this->debug && toColor) {
					tempMat.at<uchar>(this->currLoc) = 145;
				}
			}

			//This means some problem occured
			if (!this->inShape(0)) {
				timeout -= 1;
				if (timeout < 1) {
					if (this->debug) {
						std::cout << "Unexpected error - could not find near pixel in shape. At pixel " << this->currLoc << "dx dy are " << dx << " " << dy << std::endl;
						if (toColor) {
							String wName = "Corner_debug";
							cv::namedWindow(wName, WINDOW_NORMAL); // Create a window for display.
							imshow(wName, tempMat); // Show our image inside it.
							waitKey(0); // Wait for a keystroke in the window
						}
					}
					tempMat.release();
					return false;
				}
			}

			if (this->debug && toColor) {						//DEBUG
				tempMat.at<uchar>(this->currLoc) = 145;
			}

		}

		//At this point, we are inside the shape
		if (this->inShape(0)) {
			return true;
		}

	}

	bool imgCrawler::crawlToCorner(Mat tempMat, bool elipsDist)
	{
		//Choose the best direction to go, then go in that direction. Repeat until you reached a point where any movement makes you lose distance from the start
		//Also, account for the fact there might be 2 possible directions, only one of them being a legal one.
		int dirToGo = 0;		//This represents the direction to go in each round
		int altDir = 0;			//This is the 2nd poissible direction
		Point altStart = Point(-1, -1);		//Alternative point to start at
		float distFromStart;	//This represents the distance of the current point from the start
		double maxDist = 0;		//This will be the "farthest" possible pixel to go to
		double tempDist;	//Temporary distance
		Point tempLoc = Point(-1, -1);	//In case of 2 paths, we will save the first point we reached here to compare
		bool keepGoing = true;
		bool res = false;
		String tempStr;
		bool toColor = true;
		if (tempMat.cols == 1 && tempMat.rows == 1)
			toColor = false;
		if (!elipsDist) {
			distFromStart = this->pointDist(this->currLoc, this->startingLoc);
		}
		else {
			distFromStart = this->ellipseDist(this->currLoc);
		}


		//int* legalPixels = this->getAngleDirections();
		//For some reason the pointer was making problems. So, since it only appears here, I'll omit the function and use getAngleDirection here mantually
		int legalPixels[9] = {};
		legalPixels[0] = 0;
		double tempAngle;
		tempAngle = this->getAngleData(false);
		if (tempAngle < -135 || tempAngle > -45)
			legalPixels[1] = 1;
		else
			legalPixels[1] = 0;
		if (tempAngle < 180 && tempAngle > -90)
			legalPixels[2] = 1;
		else
			legalPixels[2] = 0;
		if (tempAngle < 135 && tempAngle > -135)
			legalPixels[3] = 1;
		else
			legalPixels[3] = 0;
		if (tempAngle < 90 && tempAngle > -180)
			legalPixels[4] = 1;
		else
			legalPixels[4] = 0;
		if (tempAngle < 45 || tempAngle > 135)
			legalPixels[5] = 1;
		else
			legalPixels[5] = 0;
		if (tempAngle < 0 || tempAngle > 90)
			legalPixels[6] = 1;
		else
			legalPixels[6] = 0;
		if (tempAngle < -45 || tempAngle > 45)
			legalPixels[7] = 1;
		else
			legalPixels[7] = 0;
		if (tempAngle < -90 || tempAngle > 0)
			legalPixels[8] = 1;
		else
			legalPixels[8] = 0;


		if (this->debug) {						//DEBUG
			tempStr = "";
			for (int i = 0; i < 9; i++)
				tempStr += std::to_string(legalPixels[i]) + " ";
			std::cout << "Legal directions are " << tempStr << std::endl;
		}

		//Check if there are legal pixels to go to, see if they are farthest, and decide to which to move, but remember - here we might find an alternative location.
		for (int i = 1; i < 9; i++) {
			if ((legalPixels[i] != 0) && this->inShape(i) && this->checkBorderPoint(this->getPointAt(i))) {
				if (!elipsDist)
					tempDist = this->pointDist(this->getPointAt(i), this->startingLoc);
				else {
					tempDist = this->ellipseDist(this->getPointAt(i));
				}
				if (tempDist > distFromStart) {
					if (tempDist > maxDist) {
						maxDist = tempDist;
						altDir = 0;
						altStart = Point(-1, -1);
						dirToGo = i;
					}
					else if (tempDist = maxDist) {
						altDir = i;
						altStart.x = this->currLoc.x;
						altStart.y = this->currLoc.y;
					}
				}
			}
		}

		//This is the main movement loop
		while (keepGoing) {
			//Move to the decided pixel.. 
			if (dirToGo != 0) {
				this->moveCurrent(dirToGo);
				if (this->debug && toColor) {						//DEBUG
					tempMat.at<uchar>(this->currLoc) = 100;
				}
			}
			//..or stop and declare current position as the corner we seek - and potentially switch to the other potential path
			else {
				//This means we just had a small bump to cross
				if (this->handleBump(elipsDist)) {
					if (this->debug && toColor) {						//DEBUG
						tempMat.at<uchar>(this->currLoc) = 175;
					}
				}
				else {
					//This means we are done, and only went in one direction
					if (altStart.x == -1 && altStart.y == -1) {
						res = true;
						keepGoing = false;
					}
					//This means there was another direction
					else {
						//This means we finished the first direction, and need to check the 2nd.
						if (tempLoc.x == -1 && tempLoc.y == -1) {
							tempLoc.x = this->currLoc.x;
							tempLoc.y = this->currLoc.y;
							this->currLoc.x = altStart.x;
							this->currLoc.y = altStart.y;
							this->moveCurrent(altDir);
							if (this->debug) {
								tempMat.at<uchar>(this->currLoc) = 175;
							}
						}
						//This means we finished the 2nd direction - if the first point is farther than current location, swap them
						else {
							if (!elipsDist) {
								if (this->pointDist(this->currLoc, this->startingLoc) < this->pointDist(tempLoc, this->startingLoc)) {
									this->currLoc.x = tempLoc.x;
									this->currLoc.y = tempLoc.y;
								}
							}
							else {
								if (this->ellipseDist(this->currLoc) < this->ellipseDist(tempLoc)) {
									this->currLoc.x = tempLoc.x;
									this->currLoc.y = tempLoc.y;
								}
							}

							keepGoing = false;
							res = true;
						}
					}
				}
			}

			maxDist = 0;
			dirToGo = 0;
			if (!elipsDist) {
				distFromStart = this->pointDist(this->currLoc, this->startingLoc);
			}
			else {
				distFromStart = this->ellipseDist(this->currLoc);
			}
			//Check if there are legal pixels to go to, see if they are farthest, and decide to which to mive
			for (int i = 1; i < 9; i++) {
				if ((legalPixels[i] != 0) && this->inShape(i) && this->checkBorderPoint(this->getPointAt(i))) {
					if (!elipsDist) {
						tempDist = this->pointDist(this->getPointAt(i), this->startingLoc);
					}
					else {
						tempDist = this->ellipseDist(this->getPointAt(i));
					}

					if (tempDist > distFromStart && tempDist > maxDist) {
						maxDist = tempDist;
						dirToGo = i;
					}
				}
			}
			//If we are moving on our first path out of 2, moving diagonally means it must be the correct path
			if (altStart.x != -1 || altStart.y != -1)
			{
				if (dirToGo == 2 || dirToGo == 4 || dirToGo == 6 || dirToGo == 8) {
					altStart.x = -1;
					altStart.y = -1;
				}
			}

		}

		return res;
	}

	//Part of findCorner. Detects possible "bump" - an incorrect detection of corner due to the shape being an approximate convex one and not really convex.
	//Handles the case where we are stuck on a corner while we should have continued, or where we are stuck on flat line that we should have travarsed because it represents an angle.
	bool imgCrawler::handleBump(bool elipsDist)
	{
		int botDir = 0;														//Our relative "bottom"
		int side1 = 0, side2 = 0, diag1 = 0, diag2 = 0, upSide = 0;			//The sides in which we should move, the diagonal, and which side contains the "jump up"
		float initDist;														//Initial distance
		float dist1, dist2;
		bool earlyReturn = false;
		Point tempLocA = Point(-1, -1);
		Point tempLocB = Point(-1, -1);
		Point tempLocC = Point(-1, -1);
		if (this->debug) {		//Debug
			std::cout << "Bump@ " << this->currLoc << ", ";
		}
		if (!elipsDist) {
			initDist = this->pointDist(this->currLoc, this->startingLoc);
		}
		else {
			initDist = this->ellipseDist(this->currLoc);
		}
		//Find out our relative "down" direction. Remember that if we really are stuck on a bump, there has to be one and only one.
		if (!this->inShape(8) && !this->inShape(1) && !this->inShape(2))
			botDir = 1;
		if (!this->inShape(2) && !this->inShape(3) && !this->inShape(4))
			if (botDir == 0)
				botDir = 3;
			else
				earlyReturn = true;
		if (!this->inShape(4) && !this->inShape(5) && !this->inShape(6))
			if (botDir == 0 && !earlyReturn)
				botDir = 5;
			else
				earlyReturn = true;
		if (!this->inShape(6) && !this->inShape(7) && !this->inShape(8))
			if (botDir == 0 && !earlyReturn)
				botDir = 7;
			else
				earlyReturn = true;
		//Something went wrong
		if (botDir == 0 || earlyReturn) {
			if (this->debug)
				std::cout << "early return." << std::endl;
			return false;
		}
		if (this->debug) {		//Debug
			std::cout << ":|" << botDir << "|";
		}

		//Set the potential sides depending - the side of our "step" is side1, the side along the "bottom" is side2
		//Remember that we *must* be stuck in a corner!
		switch (botDir) {
		case 1:
			this->inShape(3) ? side1 = 7 : side1 = 3;

			break;
		case 3:
			this->inShape(5) ? side1 = 1 : side1 = 5;
			break;
		case 5:
			this->inShape(7) ? side1 = 3 : side1 = 7;
			break;
		case 7:
			this->inShape(1) ? side1 = 5 : side1 = 1;
			break;
		}
		side2 = (side1 + 4) % 8;

		//Handle diagonals - diag1 should be currently in shape, diag2 should be currently outside of shape
		this->inShape(side1 + 1) ? diag1 = side1 + 1 : diag1 = side1 - 1;
		if (diag1 == 0)
			diag1 = 8;
		this->inShape(side2 + 1) ? diag2 = side2 - 1 : diag2 = side2 + 1;
		if (diag2 == 0)
			diag2 = 8;
		if (this->debug)
			std::cout << side1 << "|" << diag1 << "|" << side2 << "|" << diag2 << "|;";

		//Move to each side, and set the temp locations accordingly. 
		//Side 1:
		tempLocA.x = this->currLoc.x;
		tempLocA.y = this->currLoc.y;
		this->moveCurrent(diag1);
		while (this->inShape(side1)) {
			this->moveCurrent(side1);
		}
		if (!elipsDist) {
			dist1 = this->pointDist(this->currLoc, this->startingLoc);	//Save the distance
		}
		else {
			dist1 = this->ellipseDist(this->currLoc);
		}
		//Swap locations - I know it could be done a bit pretteir, but Im way past wasting even moment on this
		tempLocB.x = this->currLoc.x;
		tempLocB.y = this->currLoc.y;
		this->currLoc.x = tempLocA.x;
		this->currLoc.y = tempLocA.y;
		tempLocA.x = tempLocB.x;
		tempLocA.y = tempLocB.y;

		//Side 2:
		tempLocB.x = this->currLoc.x;
		tempLocB.y = this->currLoc.y;
		while (!this->inShape(diag2) && this->inShape(side2)) {
			this->moveCurrent(side2);
		}
		//This means we had 2 "upsides" - meaning there i no way we were in a "bump".
		if (!this->inShape(diag2)) {
			upSide = 1;
		}
		else {
			this->moveCurrent(diag2);
			if (!elipsDist) {
				dist2 = this->pointDist(this->currLoc, this->startingLoc);	//Save the distance
			}
			else {
				dist2 = this->ellipseDist(this->currLoc);
			}
		}
		tempLocC.x = this->currLoc.x;
		tempLocC.y = this->currLoc.y;
		this->currLoc.x = tempLocB.x;
		this->currLoc.y = tempLocB.y;
		tempLocB.x = tempLocC.x;
		tempLocB.y = tempLocC.y;

		if (this->debug) {		//Debug
			if (upSide == 1)
				std::cout << "-upS-";
			std::cout << "points: " << tempLocA << "," << tempLocB;
		}

		//Handle possible cases
		if (upSide == 1) {
			if (dist1 > initDist) {
				this->currLoc.x = tempLocA.x;
				this->currLoc.y = tempLocA.y;
				if (this->debug) {		//Debug
					std::cout << ":J@ " << tempLocA << std::endl;
				}
				return true;
			}
		}
		else {
			if (dist1 > dist2) {
				if (dist1 > initDist) {
					this->currLoc.x = tempLocA.x;
					this->currLoc.y = tempLocA.y;
					if (this->debug) {		//Debug
						std::cout << ":J@ " << tempLocA << std::endl;
					}
					return true;
				}
			}
			else {
				if (dist2 > initDist) {
					this->currLoc.x = tempLocB.x;
					this->currLoc.y = tempLocB.y;
					if (this->debug) {		//Debug
						std::cout << ":J@ " << tempLocB << std::endl;
					}
					return true;
				}
			}
		}
		if (this->debug) {		//Debug
			std::cout << ":Stay." << std::endl;
			std::cout << "DIni: " << initDist << ", D1: " << dist1 << ", D2: " << dist2 << std::endl;
		}
		return false;
	}

	//Checks if the specified location is inside the shape (aka of the same color as current color)
	bool imgCrawler::inShape(int where, Point p)
	{
		Point pos = this->getPointAt(where, p);
		//A point outside of the image bounderies is obviously not part of our object
		if (!this->checkPointLegal(pos))
			return false;

		Scalar curColor = this->image.at<uchar>(pos);
		if (curColor.val[0] == this->color)
			return true;
		else
			return false;
	}


	//Checks if the specified location is inside the shape (aka of the same color as current color)
	bool imgCrawler::moveCurrent(int whereTo)
	{
		if (whereTo < 0 || whereTo > 8)
			return false;
		else if (whereTo == 0)
			return true;
		Point pos = this->getPointAt(whereTo);
		//A point outside of the image bounderies is obviously not part of our object
		if (!this->checkPointLegal(pos))
			return false;
		//If it is, move there
		else {
			this->currLoc.x = pos.x;
			this->currLoc.y = pos.y;
			return true;
		}


	}

	//For definitions of Directions for whereTo, see the definition section at the header file.
	Point imgCrawler::getPointAt(int whereTo, Point start)
	{
		Point pos;
		if (start.x < 0 || start.y < 0)
			pos = this->currLoc;
		else
			pos = start;
		switch (whereTo) {
		case POS_N:
			pos = Point(pos.x, pos.y + 1);
			break;
		case POS_NE:
			pos = Point(pos.x + 1, pos.y + 1);
			break;
		case POS_E:
			pos = Point(pos.x + 1, pos.y);
			break;
		case POS_SE:
			pos = Point(pos.x + 1, pos.y - 1);
			break;
		case POS_S:
			pos = Point(pos.x, pos.y - 1);
			break;
		case POS_SW:
			pos = Point(pos.x - 1, pos.y - 1);
			break;
		case POS_W:
			pos = Point(pos.x - 1, pos.y);
			break;
		case POS_NW:
			pos = Point(pos.x - 1, pos.y + 1);
			break;
		default:
			pos = Point(pos.x, pos.y);
		}
		return pos;
	}

	//Checks if a point is inside the image bounderies
	bool imgCrawler::checkPointLegal(Point p)
	{
		this->pixelCounter++;

		if (p.x <= 0 || p.y <= 0 ||
			p.x > this->image.cols || p.y > this->image.rows)
			return false;
		else
			return true;
	}

	//Returns true if a point is a border point - aka one of its neighboring pixels is of a different color.
	bool imgCrawler::checkBorderPoint(Point p, bool ellipse)
	{
		//Default point is current location
		if (p.x < 0 || p.y < 0)
			return false;
		bool res = false;
		bool expectedRes;
		//False if selected point is inside the shape, true otherwise.
		if (!ellipse)
			this->image.at<uchar>(p) == this->color ? expectedRes = false : expectedRes = true;
		else
			this->ellipseDist(p) < 3 ? expectedRes = false : expectedRes = true;
		//Check surrounding points
		for (int i = 1; i < 9; i++) {
			if (!ellipse) {
				if (this->inShape(i, p) == expectedRes) {
					res = true;
					break;
				}
			}
			else {
				if (this->ellipseDist(this->getPointAt(i, p)) < 3) {
					if (expectedRes) {
						res = true;
						break;
					}
				}
				else {
					if (!expectedRes) {
						res = true;
						break;
					}
				}
			}
		}
		return res;
	}

	//Returns an array of size 9 where the i-th member is 1 if that direction can be in currentAngle's way, 0 otherwise. dirArr[0] is always 0.
	int * imgCrawler::getAngleDirections(int angle)
	{
		int dirArr[9] = {};
		double tempAngle;
		(abs(angle)>360) ? tempAngle = this->getAngleData(false) : tempAngle = angle;																		//Based on our angle we set members to 1 or 0
																																							//Can't do a switch cause it's a double =(
																																							//Also, I have to remember the image is mirrored vertically - as such, this should be mirrored as well.
		if (tempAngle < -135 || tempAngle > -45)
			dirArr[1] = 1;
		if (tempAngle < 180 && tempAngle > -90)
			dirArr[2] = 1;
		if (tempAngle < 135 && tempAngle > -135)
			dirArr[3] = 1;
		if (tempAngle < 90 && tempAngle > -180)
			dirArr[4] = 1;
		if (tempAngle < 45 || tempAngle > 135)
			dirArr[5] = 1;
		if (tempAngle < 0 || tempAngle > 90)
			dirArr[6] = 1;
		if (tempAngle < -45 || tempAngle > 45)
			dirArr[7] = 1;
		if (tempAngle < -90 || tempAngle > 0)
			dirArr[8] = 1;
		return dirArr;
	}

	//Gets data on angle. Can be converted to radians
	double imgCrawler::getAngleData(bool convertToRad) {
		double res = this->movementAngle;
		if (convertToRad)
			res = res * PI / 180;
		return res;
	}

	//Expands our dynamic array P - trivial, all allocations/deletes delt with
	void imgCrawler::expandArr() {

		int oldSize = this->pointArrSize;
		int newSize = oldSize * 2;

		Point* temp1 = new Point[oldSize];
		float* temp2 = new float[oldSize];

		for (int i = 0; i < oldSize; i++) {
			temp1[i] = this->p[i];
			temp2[i] = this->u[i];
		}

		delete[] this->p;
		delete[] this->u;

		this->p = new Point[newSize];
		this->u = new float[newSize];

		for (int i = 0; i < oldSize; i++) {
			this->p[i] = temp1[i];
			this->u[i] = temp2[i];
		}

		delete[] temp1;
		delete[] temp2;

		this->pointArrSize = newSize;
	}

	//Calculates Q, according to the paper - alg (28).
	void imgCrawler::calcQ()
	{

		Mat Ui = Mat::eye(this->pointArrCounter, this->pointArrCounter, CV_32F);
		Mat ui = Mat(this->pointArrCounter, 1, CV_32F);
		Mat P = Mat(2, this->pointArrCounter, CV_32F);

		//Calculate U
		for (int i = 0; i < this->pointArrCounter; i++) {
			Ui.at<float>(i, i) *= this->u[i];
		}
		/*if (debug) {
		std::cout << "Ui: "<<std::endl;
		for (int i = 0; i < this->pointArrCounter; i++) {
		for (int j = 0; j < this->pointArrCounter; j++) {
		std::cout << Ui.at<float>(i, j);
		if (j < this->pointArrCounter - 1)
		std::cout << " ";
		else
		std::cout << std::endl;
		}
		}
		}*/

		//Make u into a 1x|p| matrix
		for (int i = 0; i < this->pointArrCounter; i++) {
			ui.at<float>(i, 0) = this->u[i];
		}
		/*if (debug) {
		std::cout << "u: (";
		for (int i = 0; i < this->pointArrCounter; i++) {
		std::cout << ui.at<float>(i, 0);
		if (i < this->pointArrCounter - 1)
		std::cout << " ";
		}
		std::cout << ")^t" << std::endl;
		}*/
		//Initialize P
		for (int i = 0; i < this->pointArrCounter; i++) {
			P.at<float>(0, i) = this->p[i].x;
			P.at<float>(1, i) = this->p[i].y;
		}
		/*if (debug) {
		std::cout << "P:"<< std::endl;
		for (int i = 0; i < this->pointArrCounter; i++) {
		std::cout << P.at<float>(0, i) << "," << P.at<float>(1, i)<<std::endl;
		}
		}*/

		//Calculate Q according to (28)
		/*std::cout << "P: " << P.rows << "x" << P.cols << std::endl;
		std::cout << "Ui: " << Ui.rows << "x" << Ui.cols << std::endl; */
		this->Q = P * Ui * P.t();
		/*std::cout << "Q1: " << std::endl;
		std::cout << this->Q.at<float>(0, 0) << " ";
		std::cout << this->Q.at<float>(0, 1) << std::endl;
		std::cout << this->Q.at<float>(1, 0) << " ";
		std::cout << this->Q.at<float>(1, 1) << std::endl; */
		this->Q = this->Q - (P * ui)*((P * ui).t());
		/*std::cout << "Q2: " << std::endl;
		std::cout << this->Q.at<float>(0, 0) << " ";
		std::cout << this->Q.at<float>(0, 1) << std::endl;
		std::cout << this->Q.at<float>(1, 0) << " ";
		std::cout << this->Q.at<float>(1, 1) << std::endl; */
		this->Q = this->Q.inv();
		/*std::cout << "Q3: " << std::endl;
		std::cout << this->Q.at<float>(0, 0) << " ";
		std::cout << this->Q.at<float>(0, 1) << std::endl;
		std::cout << this->Q.at<float>(1, 0) << " ";
		std::cout << this->Q.at<float>(1, 1) << std::endl; */
		this->Q = this->Q *(1.f / 2);
		/*std::cout << "Q4: " << std::endl;
		std::cout << this->Q.at<float>(0, 0) << " ";
		std::cout << this->Q.at<float>(0, 1) << std::endl;
		std::cout << this->Q.at<float>(1, 0) << " ";
		std::cout << this->Q.at<float>(1, 1) << std::endl;*/
		//Release the k.. memory
		Ui.release();
		ui.release();
		P.release();

	}

	//Euclidean distance between 2 points
	float imgCrawler::pointDist(Point p1, Point p2)
	{
		cv::Point2f diff = p1 - p2;
		return cv::sqrt(diff.x*diff.x + diff.y*diff.y);
	}

	//Distance between point p and the edge of the elipsoid - alg (45)
	float imgCrawler::ellipseDist(Point p)
	{
		//Current Point
		Mat pk = Mat::zeros(2, 1, CV_32F);
		pk.at<float>(0, 0) = p.x;
		pk.at<float>(1, 0) = p.y;
		//Ellipsoid Center - weigted sum of points
		Mat ci = Mat::zeros(2, 1, CV_32F);
		ci.at<float>(0, 0) = 0;
		for (int i = 0; i < this->pointArrCounter; i++) {
			ci.at<float>(0, 0) += this->p[i].x * this->u[i];
		}
		ci.at<float>(1, 0) = 0;
		for (int i = 0; i < this->pointArrCounter; i++) {
			ci.at<float>(1, 0) += this->p[i].y * this->u[i];
		}
		//Equasion
		Mat tempRes = Mat::zeros(1, 1, CV_32F);
		tempRes = 2 * ((pk - ci).t() * this->Q * (pk - ci)) + 1;

		float res = tempRes.at<float>(0, 0);
		//Release memory
		pk.release();
		ci.release();
		tempRes.release();
		return res;
	}

	//Prints current state - for debugging
	void imgCrawler::printState() {
		std::cout << "Total pixels exemined:" << this->pixelCounter << std::endl;
		std::cout << "Starting Location:" << this->startingLoc << std::endl;
		std::cout << "Current Location:" << this->currLoc << std::endl;
		std::cout << "Temp Location:" << this->tempLoc << std::endl;
		std::cout << "Current Color:" << this->color << std::endl;
		std::cout << "Movement Angle:" << this->movementAngle << std::endl;
		std::cout << "Corner number:" << this->pointArrCounter << std::endl;
		std::cout << "Corner array size:" << this->pointArrSize << std::endl;
		//std::cout << "Pixels touched:" << this->pointArrSize << std::endl;
		for (int i = 0; i<this->pointArrSize; i++)
			std::cout << "Point " << i << ": " << this->p[i] << ", " << std::endl;
		for (int i = 0; i<this->pointArrSize; i++)
			std::cout << "Weight " << i << ": " << this->u[i] << ", " << std::endl;
		std::cout << "Q: " << std::endl;
		std::cout << this->Q.at<float>(0, 0) << " ";
		std::cout << this->Q.at<float>(0, 1) << std::endl;
		std::cout << this->Q.at<float>(1, 0) << " ";
		std::cout << this->Q.at<float>(1, 1) << std::endl;

	}

	//Sets the crawler into debug mode
	void imgCrawler::setDebug(bool val)
	{
		this->debug = val;
	}

	//General testing
	void imgCrawler::test()
	{


	}

	//For debuging purposes
	void imgCrawler::drawCross(cv::Mat image, Point p) {
		image.at<uchar>(p) = 255;
		image.at<uchar>(this->getPointAt(1, p)) = 25;
		image.at<uchar>(this->getPointAt(3, p)) = 25;
		image.at<uchar>(this->getPointAt(5, p)) = 25;
		image.at<uchar>(this->getPointAt(7, p)) = 25;
	}

	//Goes over all pixels in the image.
	//If a pixel is outside the elipse but one of the pixels near it is inside the ellipse, colors it
	void imgCrawler::slowEllipseDraw(int color) {
		std::cout << "Drawing Ellipse!" << std::endl;
		/*	for (int i = 1; i < this->image.cols; i++) {
		for (int j = 1; j < this->image.rows; j++) {
		if (this->checkBorderPoint(Point(i, j), true))
		this->image.at<uchar>(Point(i, j)) = 100;
		}
		}		*/

		//1
		this->currLoc.x = this->startingLoc.x;
		this->currLoc.y = this->startingLoc.y;
		while (!this->checkBorderPoint(this->currLoc, true) && (this->currLoc.x < this->image.cols - 2))
			this->currLoc.x += 1;
		if (this->checkBorderPoint(this->currLoc, true)) {
			if (this->debug)
				std::cout << "Ellipse border point: " << this->currLoc << " with distance " << this->ellipseDist(this->currLoc) << std::endl;
			this->drawNearbyEllipse(this->currLoc, color);
			return;
		}
		//2
		this->currLoc.x = this->startingLoc.x;
		this->currLoc.y = this->startingLoc.y;
		while (!this->checkBorderPoint(this->currLoc, true) && (this->currLoc.x > 2))
			this->currLoc.x -= 1;
		if (this->checkBorderPoint(this->currLoc, true)) {
			if (this->debug)
				std::cout << "Ellipse border point: " << this->currLoc << " with distance " << this->ellipseDist(this->currLoc) << std::endl;
			this->drawNearbyEllipse(this->currLoc, color);
			return;
		}
		//3
		this->currLoc.x = this->startingLoc.x;
		this->currLoc.y = this->startingLoc.y;
		while (!this->checkBorderPoint(this->currLoc, true) && (this->currLoc.y < this->image.rows - 2))
			this->currLoc.y += 1;
		if (this->checkBorderPoint(this->currLoc, true)) {
			if (this->debug)
				std::cout << "Ellipse border point: " << this->currLoc << " with distance " << this->ellipseDist(this->currLoc) << std::endl;
			this->drawNearbyEllipse(this->currLoc, color);
			return;
		}
		//4 - forgive me for this mess, I have little time
		this->currLoc.x = this->startingLoc.x;
		this->currLoc.y = this->startingLoc.y;
		while (!this->checkBorderPoint(this->currLoc, true) && (this->currLoc.y > 2))
			this->currLoc.y -= 1;
		if (this->checkBorderPoint(this->currLoc, true)) {
			if (this->debug)
				std::cout << "Ellipse border point: " << this->currLoc << " with distance " << this->ellipseDist(this->currLoc) << std::endl;
			this->drawNearbyEllipse(this->currLoc, color);
			return;
		}

		//this->jumpToBorder(Mat::zeros(1, 1, CV_32F), true); 


	}

	//Recursively draws nearby border ellipse pixels
	void imgCrawler::drawNearbyEllipse(Point p, int color) {
		if (this->ellipseDist(p) > 3 &&
			this->image.at<uchar>(p) != color &&
			this->checkBorderPoint(p, true))
			this->image.at<uchar>(p) = color;

		for (int i = 1; i < 9; i++) {
			if (this->checkBorderPoint(this->getPointAt(i, p), true) &&
				this->ellipseDist(this->getPointAt(i, p)) > 3 &&
				this->image.at<uchar>(this->getPointAt(i, p)) != color)
				this->drawNearbyEllipse(this->getPointAt(i, p), color);
		}
	}

	//Returns an array of the distances of all points near you from the start
	float * imgCrawler::checkDistNearMe(bool onlyBorder, bool onlyInShape, bool toPrint)
	{
		float resArr[9] = {};
		for (int i = 0; i < 9; i++) {
			resArr[i] = 1;
		}
		for (int i = 0; i < 9; i++) {
			if (resArr[i] == 0) {
				resArr[i] = 0;
			}
			else {
				resArr[i] = this->pointDist(this->startingLoc, this->getPointAt(i));
				if (onlyBorder && !this->checkBorderPoint(this->getPointAt(i))) {
					resArr[i] = 0;
				}
				if (onlyInShape && !this->inShape(i))
					resArr[i] = 0;
			}

		}
		if (toPrint) {
			String tempStr = "";
			for (int i = 0; i < 9; i++) {
				\
					tempStr += std::to_string(resArr[i]) + " ";
			}
			std::cout << "Distaces from near " << this->currLoc << " to " << this->startingLoc << " are " << tempStr << std::endl;
		}

		return resArr;
	}
	/*int* imgCrawler::getApproxArray() {
	return this->approxArray;
	}
	*/

}