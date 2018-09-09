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
														/*this->approxAngleLimit = approxAngleLimit;		//Set approximation limit
														this->approxArray = new int[approxAngleLimit];	//initialize ApproxArray based on said limit'*/
		this->p = new Point[this->pointArrSize];
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
		if (dy != 0 && dx !=0 ) {
			this->movementAngle = atan(dy / dx) * 180 / PI;
			if (dx < 0) {						//Remember - angle is mirrored vertically due to image representation
				this->movementAngle < 0 ? this->movementAngle = (180+this->movementAngle)*(-1): this->movementAngle = (180 - this->movementAngle);	//Correct the resaults depending on the quardrant
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
	bool imgCrawler::run(int color, int eps1, int eps2, int x, int y) {
		if (!this->findStartPoint(color, eps1, x, y))			//Find the first pixel of existing color
			return false;
		cv::Point left(this->startingLoc.x - 1, this->startingLoc.y);
		cv::Point right(this->startingLoc.x + 1, this->startingLoc.y);
		this->setAngle(this->startingLoc, left);						//Set movement to the left to find the first point
		if (!this->findCorner(0))										//Try to find the first point
			return false;
		this->setAngle(this->startingLoc, right);					//Set the angle from the point we found to the starting point
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
		//Now we should have our 4 initial points, and can run the 2nd part of the algorithm
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
			if(!col){
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
		else if (deltaX < eps || deltaY < eps)						//If we reached our limit, exit
			return false;
		else if (this->startingLoc.x != 0)			//if startingLoc.x isn't 0, it must mean y isn't either, so they were modified
			return true;
		else{
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
		if (this->debug) {							//DEBUG
			std::cout << "Finding corner " << num << " at angle " <<this->getAngleData(false) << std::endl;
			this->image.copyTo(tempMat);
			tempMat.at<uchar>(this->startingLoc) = 255;
		}
		int bhop = 1;								//This is how much we "hop" each time
		double dx = cos(this->getAngleData(true));	//Delta x
		double dy = sin(this->getAngleData(true));	//Delta y

		this->currLoc  = this->startingLoc;
		Point prev = Point(0,0);

		//Hop in big jumps, increasing x2 every time, until you leave the shape
		while (this->inShape() && !this->checkBorderPoint(this->currLoc)) {
			bhop *= 2;
			prev.x = this->currLoc.x;
			prev.y = this->currLoc.y;
			this->currLoc.x += (int)(bhop*dx);
			if (bhop*dx - int(bhop*dx) >0.00000001)	this->currLoc.x += 1; //Round up
			this->currLoc.y += (int)(bhop*dy);
			if (bhop*dy - int(bhop*dy) >0.00000001)	this->currLoc.y += 1; //Round up
			if (this->debug) {						//DEBUG
				tempMat.at<uchar>(this->currLoc) = 190;
			}
		}

		//Once we leave the shape, return back in with jumps becoming x2 smaller each time
		while (bhop > 1) {
			//If we are at the border, we can exit - it will get fixed in the next step anyway.
			if (this->checkBorderPoint(this->currLoc))
				break;
			bhop /= 2;
			prev.x = this->currLoc.x;
			prev.y = this->currLoc.y;
			if (this->inShape()) {
				this->currLoc.x += (int)(bhop*dx);
				this->currLoc.y += (int)(bhop*dy);
			}
			else {
				this->currLoc.x -= (int)(bhop*dx);
				this->currLoc.y -= (int)(bhop*dy);
			}
			if (this->debug) {						//DEBUG
				tempMat.at<uchar>(this->currLoc) = 145;
			}
		}

		//Because it is possible to end up just outside the border instead of inside, do a small correction
		if (!this->inShape()) {
			this->currLoc.x -= (int)(dx);
			this->currLoc.y -= (int)(dy);
			//Now, we might still be 1 pixel outside the shape (becaus we rounded down!), so check if we are in and if not, go to a neighbour that's in
			bool tempTry = !this->inShape();
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
				if(tempTry)
					for (int i = 1; i < 9; i++) {
						if (this->checkBorderPoint(this->getPointAt(i))) {
							this->moveCurrent(i);
						}
						else {
							tempTry = false;
						}
					}
			}
		//This means some problem occured
		if (!this->inShape()) {
			if (this->debug) {
				std::cout<< "Unexpected error - could not find near pixel in shape. At pixel "<< this->currLoc << std::endl;
				String wName = "Corner_" + std::to_string(num) + "_debug";
				cv::namedWindow(wName, WINDOW_NORMAL); // Create a window for display.
				imshow(wName, tempMat); // Show our image inside it.
				waitKey(0); // Wait for a keystroke in the window
			}
			return res;
		}

		if (this->debug) {						//DEBUG
			tempMat.at<uchar>(this->currLoc) = 145;
		}
		//At this point, we are inside the shape
		}

		//Now, it's time to crawl along the border until we get to a point where any legal movement brings us closer to the starting point

		//Get potential legal movement pixels
		int* legalPixels = this->getAngleDirections();
		if (this->debug) {						//DEBUG
			String tempStr = "";
			for (int i = 0; i < 9; i++)
				tempStr += std::to_string(legalPixels[i]) + " ";
			std::cout<< "Legal directions are " << tempStr << std::endl;
		}
		//Choose the best direction to go, then go in that direction. Repeat until you reached a point where any movement makes you lose distance from the start
		//Also, account for the fact there might be 2 possible directions, only one of them being a legal one.
		int dirToGo = 0;		//This represents the direction to go in each round
		int altDir = 0;			//This is the 2nd poissible direction
		Point altStart = Point(-1,-1);		//Alternative point to start at
		float distFromStart = this->pointDist(this->currLoc, this->startingLoc);	//This represents the distance of the current point from the start
		double maxDist = 0;		//This will be the "farthest" possible pixel to go to
		double tempDist;	//Temporary distance
		Point tempLoc = Point(-1, -1);	//In case of 2 paths, we will save the first point we reached here to compare
		bool keepGoing = true;

		//Check if there are legal pixels to go to, see if they are farthest, and decide to which to move, but remember - here we might find an alternative location.
		for (int i = 1; i < 9; i++) {
			if ((legalPixels[i] != 0) && this->inShape(i) && this->checkBorderPoint(this->getPointAt(i))) {
				tempDist = this->pointDist(this->getPointAt(i), this->startingLoc);
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
				if (this->debug) {						//DEBUG
					tempMat.at<uchar>(this->currLoc) = 100;
				}
			}
			//..or stop and declare current position as the corner we seek - and potentially switch to the other potential path
			else {
				//This means we are done, and only went in one direction
				if (altStart.x == -1 && altStart.y == -1) {
					this->p[num] = this->currLoc;
					this->pointArrCounter++;
					if ((this->pointArrCounter + 1) == this->pointArrSize)
						this->expandP();
					if (this->debug) {						//DEBUG
						String wName = "Corner_" + std::to_string(num) + "_debug";
						cv::namedWindow(wName, WINDOW_NORMAL); // Create a window for display.
						imshow(wName, tempMat); // Show our image inside it.
						waitKey(0); // Wait for a keystroke in the window
					}
					keepGoing = false;
					res = true;
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
					}
					//This means we finished the 2nd direction - if the first point is farther than current location, swap them
					else {
						if (this->pointDist(this->currLoc, this->startingLoc) < this->pointDist(tempLoc, this->startingLoc)) {
							this->currLoc.x = tempLoc.x;
							this->currLoc.y = tempLoc.y;
						}	
						this->p[num] = this->currLoc;
						this->pointArrCounter++;
						if ((this->pointArrCounter + 1) == this->pointArrSize)
							this->expandP();
						if (this->debug) {						//DEBUG
							String wName = "Corner_" + std::to_string(num) + "_debug";
							cv::namedWindow(wName, WINDOW_NORMAL); // Create a window for display.
							imshow(wName, tempMat); // Show our image inside it.
							waitKey(0); // Wait for a keystroke in the window
						}
						keepGoing = false;
						res = true;
					}
				}
			}

			maxDist = 0;
			dirToGo = 0;
			distFromStart = this->pointDist(this->currLoc, this->startingLoc);
			//Check if there are legal pixels to go to, see if they are farthest, and decide to which to mive
			for (int i = 1; i < 9; i++) {
				if ( (legalPixels[i] != 0) && this->inShape(i) && this->checkBorderPoint(this->getPointAt(i)) ) {
					tempDist = this->pointDist(this->getPointAt(i), this->startingLoc);
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
		Point pos = this->getPointAt(whereTo);
		//A point outside of the image bounderies is obviously not part of our object
			if(!this->checkPointLegal(pos))
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
		if (p.x <= 0 || p.y <= 0 ||
			p.x > this->image.cols || p.y > this->image.rows)
			return false;
		else
			return true;
	}
	//Returns true if a point is a border point - aka one of its neighboring pixels is of a different color.
	bool imgCrawler::checkBorderPoint(Point p)
	{
		//Default point is current location
		if (p.x < 0 || p.y < 0)
			p = this->currLoc;
		bool res = false;
		bool expectedRes;																		//False if selected point is inside the shape, true otherwise.
		this->image.at<uchar>(p) == this->color ? expectedRes = false : expectedRes = true;
		for (int i = 1; i < 9; i++) {
			if (this->inShape(i, p) == expectedRes) {
				res = true;
				break;
			}
		}
		return res;
	}

	//Returns an array of size 9 where the i-th member is 1 if that direction can be in currentAngle's way, 0 otherwise. dirArr[0] is always 0.
	int * imgCrawler::getAngleDirections(int angle)
	{
		int dirArr[9] = {};	
		double tempAngle;
		(abs(angle)>360)? tempAngle= this->getAngleData(false) : tempAngle = angle;																		//Based on our angle we set members to 1 or 0
		//Can't do a switch cause it's a double =(
		if (tempAngle <= 180 && tempAngle >= 0)
			dirArr[1] = 1;
		if (tempAngle <= 135 && tempAngle >= -45)
			dirArr[2] = 1;
		if (tempAngle <= 90 && tempAngle >= -90)
			dirArr[3] = 1;
		if (tempAngle <= 45 && tempAngle >= -135)
			dirArr[4] = 1;
		if (tempAngle <= 0 || tempAngle == 180)
			dirArr[5] = 1;
		if (tempAngle <= -45 || tempAngle >= 135)
			dirArr[6] = 1;
		if (tempAngle <= -90 || tempAngle >= 90)
			dirArr[7] = 1;
		if (tempAngle <= -135 || tempAngle >= 45)
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
	void imgCrawler::expandP() {
		int oldSize = this->pointArrSize;
		int newSize = oldSize * 2;
		Point* temp = new Point[oldSize];
		for (int i = 0; i < oldSize; i++)
			temp[i] = this->p[i];
		delete[] this->p;
		this->p = new Point[newSize];
		for (int i = 0; i < oldSize; i++)
			this->p[i] = temp[i];
		delete[] temp;
		this->pointArrSize = newSize;
	}

	//Euclidean distance between 2 points
	float imgCrawler::pointDist(Point p1, Point p2)
	{
		cv::Point2f diff = p1 - p2;
		return cv::sqrt(diff.x*diff.x + diff.y*diff.y);
	}

	void imgCrawler::printState() {
		std::cout << "Starting Location:" << this->startingLoc << std::endl;
		std::cout << "Current Location:" << this->currLoc << std::endl;
		std::cout << "Temp Location:" << this->tempLoc << std::endl;
		std::cout << "Current Color:" << this->color << std::endl;
		std::cout << "Movement Angle:" << this->movementAngle << std::endl;
		std::cout << "Corner number:" << this->pointArrCounter << std::endl;
		std::cout << "Corner array size:" << this->pointArrSize << std::endl;
		for(int i = 0; i<this->pointArrSize; i++)
			std::cout << "Point " << i << ": " << this->p[i] << std::endl;

	
	}

	void imgCrawler::setDebug(bool val)
	{
		this->debug = val;
	}
	//General testing
	void imgCrawler::test()
	{
		cv::Mat tempMat;
		this->image.copyTo(tempMat);
		for (int i = 0; i < 50; i++) {
			tempMat.at<uchar>(Point(20, i)) = 125;
		}
		cv::namedWindow("test", WINDOW_NORMAL); // Create a window for display.
		imshow("test", tempMat); // Show our image inside it.
		waitKey(0); // Wait for a keystroke in the window
	}

	/*int* imgCrawler::getApproxArray() {
	return this->approxArray;
	}
	*/

}