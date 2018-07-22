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
														/*this->approxAngleLimit = approxAngleLimit;		//Set approximation limit
														this->approxArray = new int[approxAngleLimit];	//initialize ApproxArray based on said limit'*/
		this->p = new Point[4];
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
		double dx = x1 - x2;
		double dy = y1 - y2;
		//Calculate angle
		if (dy != 0 && dx !=0 ) {
			this->movementAngle = atan(dx / dy) * 180 / PI;
			if (dx < 0)							//Same for if dy<0 and dy>0. If dx>0 and dy<0 then atan gives the correct resault anyway
				this->movementAngle += 180;
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
	cv:Point left(this->startingLoc.x - 1, this->startingLoc.y);
		this->setAngle(this->startingLoc, left);						//Set movement to the left to find the first point
		if (!this->findCorner(0))										//Try to find the first point
			return false;
		this->setAngle(this->p[0], this->startingLoc);					//Set the angle from the point we found to the starting point
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
		int bhop = 1;								//This is how much we "hop" each time
		double dx = cos(this->getAngleData(true));	//Delta x
		double dy = sin(this->getAngleData(true));	//Delta y

		this->currLoc  = this->startingLoc;
		Point prev = Point(0,0);

		//Hop in big jumps, increasing x2 every time, until you leave the shape
		while (this->inShape) {
			bhop *= 2;
			prev.x = this->currLoc.x;
			prev.y = this->currLoc.y;
			this->currLoc.x += (int)(bhop*dx);
			this->currLoc.y += (int)(bhop*dy);
		}

		//Once we leave the shape, return back in with jumps becoming x2 smaller each time
		while (bhop > 1) {
			bhop /= 2;
			prev.x = this->currLoc.x;
			prev.y = this->currLoc.y;
			if (this->inShape) {
				this->currLoc.x += (int)(bhop*dx);
				this->currLoc.y += (int)(bhop*dy);
			}
			else {
				this->currLoc.x -= (int)(bhop*dx);
				this->currLoc.y -= (int)(bhop*dy);
			}
		}

		//Because it is possible to end up just outside the border instead of inside, do a small correction
		if (!this->inShape) {
			this->currLoc.x -= (int)(dx);
			this->currLoc.y -= (int)(dy);
			//Now, we might still be 1 pixel outside the shape (becaus we rounded down!), so check if we are in and if not, go to a neighbour that's in
			if (!this->inShape()) {
				//Check neighbouring pixels, and go to the one that's in the shape and break
				for (int i = 1; i < 9; i++) {
					if (this->inShape(i)) {
						this->moveCurrent(i);
						break;
					}
				}
				//This means some problem occured
				if (!this->inShape())
					return false;
			}
		//At this point, we are inside the shape
		}

		//Now, it's time to crawl along the border until we get to a point where any legal movement brings us closer to the starting point

		//Get potential legal movement pixels
		int* legalPixels = this->getAngleDirections;
		//Choose the best direction to go, then go in that direction. Repeat until you reached a point where any movement makes you lose distance from the start
		int dirToGo;		//This represents the direction to go in each round
		int distFromStart;	//This represents the distance of the current point from the start
		double maxDist;		//This will be the "farthest" possible pixel to go to
		double tempDist;	//Temporary distance
		bool keepGoing = true;
		while (keepGoing) {
			maxDist = 0;
			dirToGo = 0;
			distFromStart = this->pointDist(this->currLoc, this->startingLoc);
			//Check if there are legal pixels to go to, see if they are farthest, and decide to which to mive
			for (int i = 1; i < 9; i++) {
				if (legalPixels[i] != 0 && this->inShape[i]) {
					tempDist = this->pointDist(this->getPointAt[i], this->startingLoc);
					if (tempDist > distFromStart && tempDist > maxDist) {
						maxDist = tempDist;
						dirToGo = i;
					}
				}
			}
			//Move to the decided pixel.. 
			if (dirToGo != 0) {
				this->moveCurrent(dirToGo);
			}
			//..or stop and declare current position as the corner we seek
			else {
				this->p[num] = this->currLoc;
				return true;
			}
		}
	}

	//Checks if the specified location is inside the shape (aka of the same color as current color)
	bool imgCrawler::inShape(int where)
	{
		Point pos = this->getPointAt(where);
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

	Point imgCrawler::getPointAt(int whereTo)
	{
		Point pos = Point(0, 0);
		switch (whereTo) {
		case POS_N:
			Point pos = Point(this->currLoc.x, this->currLoc.y + 1);
			break;
		case POS_NE:
			Point pos = Point(this->currLoc.x + 1, this->currLoc.y + 1);
			break;
		case POS_E:
			Point pos = Point(this->currLoc.x + 1, this->currLoc.y);
			break;
		case POS_SE:
			Point pos = Point(this->currLoc.x + 1, this->currLoc.y - 1);
			break;
		case POS_S:
			Point pos = Point(this->currLoc.x, this->currLoc.y - 1);
			break;
		case POS_SW:
			Point pos = Point(this->currLoc.x - 1, this->currLoc.y - 1);
			break;
		case POS_W:
			Point pos = Point(this->currLoc.x - 1, this->currLoc.y);
			break;
		case POS_NW:
			Point pos = Point(this->currLoc.x - 1, this->currLoc.y + 1);
			break;
		default:
			Point pos = Point(this->currLoc.x, this->currLoc.y);
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

	//Returns an array of size 9 where the i-th member is 1 if that direction can be in currentAngle's way, 0 otherwise. dirArr[0] is always 0.
	int * imgCrawler::getAngleDirections()
	{
		int dirArr[9] = {};	
		static double tempAngle;																		//Based on our angle we set members to 1 or 0
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

	/*int* imgCrawler::getApproxArray() {
	return this->approxArray;
	}
	*/

}