/*************************************************
Copyright: SIFT::panorama
Author: Sally
Date:2016-07-26
Description: Implement Extract SIFT features.
**************************************************/

#include "sift.h"

/*************************************************
Function:    // SIFT
Description: // Constructor of class SIFT
Calls:
Inputs:      // src, input image
			 // sigma, the parameter of Gaussian
			 // scale, the height of scale space
**************************************************/
SIFT::SIFT(Mat& img, int octave, int scale, double sigma){
	// Initial the parameters
	this->octave = octave;
	this->scale = scale;


	// Construct the scale space
	this->gausPyr = generateGaussianPyramid(img, octave, scale, sigma);
	this->DoGs = generateDoGPyramid(this->gausPyr, octave, scale, sigma);

	this->features = findKeypoints();
	Scalar color[3] = {Scalar(0,0,255),Scalar(0,255,0),Scalar(255,0,0)};
	for (int i = 0; i < features.size(); i++) {
		/*
			double x = features[i].x;
			double y = features[i].y;
			circle(this->DoGs[features[i].oct_id  * (scale + 2)], Point(x, y),2.0, Scalar(0,0,1),1,8);
			cout << features[i].oct_id << endl;
			*/
		double x = features[i].x * pow(2.0, (features[i].oct_id-1));
		double y = features[i].y * pow(2.0, (features[i].oct_id-1));
		circle(img, Point(x, y),2.0, color[features[i].oct_id],-1,8);
	}
	imshow("Image",img);
	/*
	for(int i = 0; i < this->octave * (scale + 2);){
		char buffer[20];
		itoa(i,buffer,10);
		string number(buffer);
		string name = "Image " + number;
		namedWindow(name);  
		imshow(name,this->DoGs[i]); 
		i = i + 1;
	}
	*/

	this->mag_Pyramid = new Mat[octave*(scale+3)];
    this->ori_Pyramid = new Mat[octave*(scale+3)];
    for(int i=0;i<octave;i++)
    	for(int j=0;j<scale+3;j++)
    		GetOriAndMag(i,j);

    /**********testing**********/
	if (features.size() != 0) {
		cout << "sucess" << endl;
		cout << features.size() << endl;
	}
	/**********testing**********/

    int num_keypoint = features.size();
    for(int i=0;i<num_keypoint;i++)
    {
    	key_point tmp = features[i];
    	std::vector<double> oris = OrientationAssignment(tmp);
    	features[i].orientation=oris[0];
    	for(int j=1;j<oris.size();j++)
    	{
    		key_point p = tmp;
    		p.orientation = oris[j];
    		features.push_back(p);
    	}
    }

	/*
	for (int i = 0; i < features.size(); i++) {
		double x = features[i].x * pow(2.0, features[i].oct_id);
		double y = features[i].y * pow(2.0, features[i].oct_id);
		circle(img, Point(x, y),1.0, Scalar(0,0,255),1,8);
	}
	imshow("Image",img);
	*/
/*

	for(int i=0;i<features.size();i++)
	{
		GetVector(features[i]);
	}
*/

}

/*************************************************
Function:    convertRGBToGray64F
Description: // convert the input RGB image to a 64F Gray image
Inputs:      // src, input image
			 // dst, output image
**************************************************/
void SIFT::convertRGBToGray64F(const Mat& src, Mat& dst){
	// Determine the type of input image
	if (src.channels() == 1){
		cout << "The input image is a gray image." << endl;
		return;
	}

	// Initial the size of output image.
	dst = cvCreateMat(src.rows, src.cols, CV_64F);

	// Convert the image 
	for (int x = 0; x < src.rows; x++){
		for (int y = 0; y < src.cols; y++){
			int b = (int)((uchar*)src.data)[x * src.step + src.channels() * y + 0];
			int g = (int)((uchar*)src.data)[x * src.step + src.channels() * y + 1];
			int r = (int)((uchar*)src.data)[x * src.step + src.channels() * y + 2];
			// Scale the range from 0 to 1
			((double*)dst.data)[x * dst.cols + y] = (double)(r + g + b) / (double)(255 * 3);
		}
	}
}


/*************************************************
Function:    upSample
Description: // Use linear interpolation to up sample the input image and convert its result
to the output image
Inputs:      // src, input gray image and its bit depth
				is CV_64F
			 // dst, output image
**************************************************/
void SIFT::upSample(const Mat& src, Mat& dst){
	// Determine the type of input image
	if (src.channels() != 1){
		cout << "The input image is not a gray image." << endl;
		return;
	}
	if (src.type() != CV_64F){
		cout << "The bit depth of input image is not CV_64F." << endl;
		return;
	}

	// Initial the size of output image.
	dst = cvCreateMat(src.rows * 2, src.cols * 2, CV_64F);

	// Up sample with  linear interpolation
	for(int srcX = 0; srcX < src.rows - 1; srcX++){
		for (int srcY = 0; srcY < src.cols - 1; srcY++){
			((double*)dst.data)[(srcX * 2) * dst.cols + (srcY * 2)] = ((double*)src.data)[srcX * src.cols + srcY];
			// interpolate x
			double dx = ((double*)src.data)[srcX * src.cols + srcY] + ((double*)src.data)[(srcX + 1) * src.cols + srcY];
			((double*)dst.data)[(srcX * 2 + 1) * dst.cols + (srcY * 2)] = dx / 2.0;
			// interpolate y
			double dy = ((double*)src.data)[srcX * src.cols + srcY] + ((double*)src.data)[srcX * src.cols + (srcY + 1)];
			((double*)dst.data)[(srcX * 2) * dst.cols + (srcY * 2 + 1)] = dy / 2.0;
			// interpolate xy
			/*double dxy = ((double*)src.data)[srcX * src.cols + srcY] + ((double*)src.data)[(srcX + 1) * src.cols + (srcY + 1)];
			((double*)dst.data)[(srcX * 2 + 1) * dst.cols + (srcY * 2 + 1)] = dxy / 2.0;*/
			double dxy = ((double*)src.data)[srcX * src.cols + srcY] + ((double*)src.data)[(srcX + 1) * src.cols + srcY]
				+ ((double*)src.data)[srcX * src.cols + (srcY + 1)] + ((double*)src.data)[(srcX + 1) * src.cols + (srcY + 1)];
			((double*)dst.data)[(srcX * 2 + 1) * dst.cols + (srcY * 2 + 1)] = dxy / 4.0;
		}
	}

	if (dst.cols < 3 || dst.rows < 3)
		return;
	// The last two cols & last two rows
	for (int row = 0; row < dst.rows; row++) {
		((double*)dst.data)[row * dst.cols + dst.cols - 2] = ((double*)src.data)[(row / 2) * src.cols + src.cols - 1];
		((double*)dst.data)[row * dst.cols + dst.cols - 1] = ((double*)src.data)[(row / 2) * src.cols + src.cols - 1];
	}
	for (int col = 0; col < dst.cols; col++) {
		((double*)dst.data)[(dst.rows - 2) * dst.cols + col] = ((double*)src.data)[(src.rows - 1) + col / 2];
		((double*)dst.data)[(dst.rows - 1) * dst.cols + col] = ((double*)src.data)[(src.rows - 1) + col / 2];
	}
}


/*************************************************
Function:    downSample
Description: // Down sample the input image and convert its result to the output image
Inputs:      // src, input gray image and its bit depth is CV_64F
			 // dst, output image
**************************************************/
void SIFT::downSample(const Mat& src, Mat& dst){
	// Determine the type of input image
	if (src.channels() != 1){
		cout << "The input image is not a gray image." << endl;
		return;
	}
	if (src.type() != CV_64F){
		cout << "The bit depth of input image is not CV_64F." << endl;
		return;
	}

	// Initial the size of output image.
	dst = cvCreateMat(src.rows / 2, src.cols / 2, CV_64F);

	for (int dstX = 0; dstX < dst.rows; dstX++){
		for (int dstY = 0; dstY < dst.cols; dstY++){
			double sum = ((double*)src.data)[(dstX * 2)* src.cols + (dstY * 2)]
				+ ((double*)src.data)[(dstX * 2 + 1)* src.cols + (dstY * 2)]
				+ ((double*)src.data)[(dstX * 2)* src.cols + (dstY * 2 + 1)]
				+ ((double*)src.data)[(dstX * 2 + 1)* src.cols + (dstY * 2 + 1)];
			((double*)dst.data)[dstX * dst.cols + dstY] = sum / 4.0;
		}
	}
}


/*************************************************
Function:    convolve
Description: // Convolution of the input image with the filter
Inputs:      // src, input gray image and its bit depth is CV_64F
			 // filter
			 // dst, output image
			 // a, the width of the filter
			 // b, the height of the filter
**************************************************/
void SIFT::convolve(const Mat& src, double filter[], Mat& dst, int a, int b){
	// Initialization
	dst = cvCreateMat(src.rows, src.cols, CV_64F);
	
	// Padding
	Mat srcPadding = cvCreateMat(src.rows + 2 * a, src.cols + 2 * b, CV_64F);
	for(int x = 0; x < srcPadding.rows; x++){
		for(int y = 0; y < srcPadding.cols; y++){
			if(y >= (src.cols + b - 1 ) || x >= ( src.rows + a - 1))
				((double*)srcPadding.data)[x * srcPadding.cols + y] = 0.0;
			else if((y <= b ) || x <= a)
				((double*)srcPadding.data)[x * srcPadding.cols + y] = 0.0;
			else
				((double*)srcPadding.data)[x * srcPadding.cols + y] = ((double*)src.data)[(x - a) * src.cols + y - b];
		}
	}

	int filterSize = min(2 * a + 1, 2 * b + 1);

	for(int x = a; x < srcPadding.rows - a; x++){
		for(int y = b; y < srcPadding.cols - b; y++){
			double sum = 0.0;
			for(int s = -a; s <= a; s++){
				for(int t = -b; t <= b; t++){
					sum += filter[(s + a) * filterSize + (t + b)] 
							* ((double*)srcPadding.data)[(x + s) * srcPadding.cols + (y + t)];
				}
			}
			((double*)dst.data)[(x - a) * dst.cols + (y - b)] = sum;
		}
	} 
}

/*************************************************
Function:    gaussianSmoothing
Description: // Gaussian Smoothing of the input image
Inputs:      // src, input gray image and its bit depth is CV_64F
			 // dst, output image
			 // sigma
**************************************************/
void SIFT::gaussianSmoothing(const Mat& src, Mat& dst, double sigma){
	// Generate the gaussian mask
	GaussianMask gaussianMask(sigma);
	double* filter = gaussianMask.getFullMask();
	int filterSize = gaussianMask.getSize();
	Mat dstCx;
	convolve(src, filter, dstCx, filterSize / 2, 0);
	convolve(dstCx, filter, dst, 0, filterSize / 2);
}


/*************************************************
Function:    Subtraction
Description: // Calculate the difference, the first input image sub the second image
Inputs:      // src1, input gray image and its bit depth is CV_64F
			 // src2
			 // dst, output image
			 // sigma
**************************************************/
void SIFT::subtraction(const Mat& src1, const Mat& src2, Mat& dst){
	if (src1.cols != src2.cols || src1.rows != src2.rows){
		cout << "The sizes are not same." << endl;
		return;
	}
	dst = cvCreateMat(src1.rows, src1.cols, CV_64F);
	for (int x = 0; x < src1.rows; x++){
		for (int y = 0; y < src1.cols; y++){
			((double*)dst.data)[x * dst.cols + y] = ((double*)src1.data)[x * src1.cols + y]
				- ((double*)src2.data)[x * src2.cols + y];
		}
	}
}


/*************************************************
Function:    generateGaussianPyramid
Description: // generate the Gaussian Pyramid
Inputs:      // src, input gray image and its bit depth is CV_64F
		     // octaves, the number of octaves
			 // scale, the height of scale space
			 // sigma, the parameter of Gaussian
Outputs:	 // 1D vector of Gaussian Pyramid
**************************************************/  
Mat* SIFT::generateGaussianPyramid(Mat& src, int octaves, int scales, double sigma){
	double k = pow(2, (1 / (double)this->scale));
	int intervalGaus = scale + 3;

	Mat* gaussPyr = new Mat[octaves * intervalGaus];

	// Convert to Gray image and Up sample 
	Mat initGrayImg; //, initUpSampling;
	convertRGBToGray64F(src, initGrayImg);
	//upSample(initGrayImg, initUpSampling);

	// Generate a list of series of sigma
	double *sigmas = new double[intervalGaus];
	sigmas[0] = sigma;
	for(int i = 1; i < intervalGaus; i++){
		sigmas[i] = sigmas[i - 1] * k;
	}


	this->sigma = new double[octave*intervalGaus];
	for (int i = 0; i < intervalGaus; i++)
		this->sigma[i] = sigmas[i];
	for(int i = 1 ; i < octave ; i++)
		for (int j = 0; j < intervalGaus; j++) {
			if (j == 0)
				this->sigma[i*intervalGaus] = this->sigma[i*intervalGaus - 3];
			else
				this->sigma[i*intervalGaus + j] = this->sigma[i*intervalGaus + j - 1] * k;
		}

	// Generate smoothing images in the first octave
	for (int i = 0; i < intervalGaus; i++){
		Mat smoothing;
		if(i == 0)
			//gaussianSmoothing(initUpSampling, smoothing, sigmas[i]);
			gaussianSmoothing(initGrayImg, smoothing, sigmas[i]);
		else
			gaussianSmoothing(gaussPyr[i-1], smoothing, sigmas[i]);
		gaussPyr[i] = smoothing;
	}
	// Use down sample to generate every first layer of rest octave 
	// And then get smooth images base on it
	 
	for(int i = 1; i < octaves; i++){
		Mat downSampling;
		downSample(gaussPyr[intervalGaus * (i - 1) + 3], downSampling);
		gaussPyr[i * intervalGaus] = downSampling;
		for(int j = 1; j < intervalGaus; j++){
			Mat dsmoothing;
			gaussianSmoothing(gaussPyr[i * intervalGaus + j - 1], dsmoothing, sigmas[j]);
			gaussPyr[i * intervalGaus + j] = dsmoothing; 
		}

	} 
	 
	// Test:
	/*
	 for(int i = 0; i < this->octave * intervalGaus; i++){
		char buffer[20];
		itoa(i,buffer,10);
		string number(buffer);
		string name = "Image " + number;
		namedWindow(name);  
		imshow(name,gaussPyr[i]); 
	}
	*/

	return gaussPyr;
}


/*************************************************
Function:    generateDoGPyramid
Description: // generate the DoG Pyramid
Inputs:      // src, input gray image and its bit depth is CV_64F
			 // octaves, the number of octaves
			 // scale, the height of scale space
			 // sigma, the parameter of Gaussian
Outputs:	 // 1D vector of DoG Pyramid
**************************************************/ 
Mat* SIFT::generateDoGPyramid(Mat* gaussPyr, int octaves, int scales, double sigma){
	int intervalDoGs = scale + 2;
	int intervalGaus = scale + 3;
	Mat* dogPyr = new Mat[this->octave * intervalDoGs];

	for(int i = 0; i < octaves; i++){
		for(int j = 0; j < intervalDoGs; j++){
			int number = i * intervalGaus + j;
			Mat subImg;
			subtraction(gaussPyr[i * intervalGaus + j + 1], gaussPyr[i * intervalGaus + j], subImg);
			dogPyr[i * intervalDoGs + j] = subImg;
		}
	}

	// Test:
	/*
	for(int i = 0; i < this->octave * intervalDoGs; i++){
		char buffer[20];
		itoa(i,buffer,10);
		string number(buffer);
		string name = "Image " + number;
		namedWindow(name);  
		imshow(name,dogPyr[i]); 
	}
	*/
	return dogPyr;
}



/*************************************************
Function:   GaussianMask
Description: // Constructor, Calculate the Gaussian mask
Inputs:      // sigma
**************************************************/
GaussianMask::GaussianMask(double sigma){
	double gaussianDieOff = 0.001;
	vector<double> halfMask;
	for(int i = 0; true; i++){
	//for(int i = 0; i <= 3; i++){
		double d = exp(- i * i / (2 * sigma * sigma));
		if(d < gaussianDieOff) 
			break;
		halfMask.insert(halfMask.begin(), d);
	}

	this->maskSize = 2 * halfMask.size() - 1;
	this->fullMask = new double[this->maskSize];
	vector<double>::iterator it;
	int num = 0;
	for (it = halfMask.begin(); it != halfMask.end(); it++, num++){
		fullMask[num] = *it;
	}
	for (num -= 1; num < this->maskSize; num++){
		fullMask[num] = fullMask[this->maskSize - 1 - num];
	}

	double sum = 0.0;
	for (int i = 0; i < maskSize; i++){
		sum += fullMask[i];
	}
	for (int i = 0; i < maskSize; i++){
		fullMask[i] /= sum;
	}
}



// detect features by finding extrema in DoG scale space
vector<key_point> SIFT::findKeypoints() {
	vector<key_point> keyPoints;
	for (int o = 0; o < octave; o++) {
	    for (int i = 0; i < scale; i++) {
	        for (int r = SIFT_IMG_BORDER; r < DoGs[o * (scale + 2) + i + 1].rows - SIFT_IMG_BORDER; r++) {
				for (int c = SIFT_IMG_BORDER; c < DoGs[o * (scale + 2) + i + 1].cols - SIFT_IMG_BORDER; c++) {
					if (isExtremum(o, i, r, c)) {
						key_point* kp = interpolateExtrema(o, i, r, c);
						if (kp != NULL) {
							key_point new_kp;
							new_kp.x = kp->x;
							new_kp.y = kp->y;
							new_kp.oct_id = kp->oct_id;
							new_kp.scale_id = kp->scale_id;
							new_kp.orientation = kp->orientation;
							new_kp.v = kp->v;
							keyPoints.push_back(new_kp);
						}
					}
				}
			}
		}
	}
	return keyPoints;
}

// check its 26 neighbors in 3x3 regions at the current and adjacent scales
// to find out if it's an extremum
bool SIFT::isExtremum(int octave, int interval, int row, int column) {
	bool maximum = true, minimum = true;

	double cur_value = DoGs[octave * (this->scale + 2) + interval + 1].at<double>(row, column);

	// (for convenience the loop check include current pixel itself)
	for (int i = interval - 1; i <= interval + 1; i++) {
		for (int r = row - 1; r <= row + 1; r++) {
			for (int c = column - 1; c <= column + 1; c++) {
				// current image = DoGs[octave * (scale + 2) + i + 1]
				if (cur_value < DoGs[octave * (scale + 2) + i + 1].at<double>(r, c))
					maximum = false;
				if (cur_value > DoGs[octave * (scale + 2) + i + 1].at<double>(r, c))
					minimum = false;
				if (!maximum && !minimum)
					return false;
			}
		}
	}
	return true;
}

// interpolate extrema to sub-pixel accuracy
key_point* SIFT::interpolateExtrema(int octave, int interval, int row, int column){
	int i = 0;
	int cur_interval = interval;
	int cur_row = row;
	int cur_col = column;
	int layer_index;
	Vec3f dD;
	Vec3f offset;
	double dx, dy, ds, dxx, dyy, dss, dxy, dxs, dys;
	double offset_c, offset_r, offset_i;
	while (i < SIFT_MAX_INTERP_STEPS) {
		layer_index = octave * (scale + 2) + cur_interval + 1;

		// first derivative
		dx = (DoGs[layer_index].at<double>(cur_row, cur_col + 1) -
			DoGs[layer_index].at<double>(cur_row, cur_col - 1)) / 2;
		dy = (DoGs[layer_index].at<double>(cur_row + 1, cur_col) -
			DoGs[layer_index].at<double>(cur_row - 1, cur_col)) / 2;
		ds = (DoGs[layer_index + 1].at<double>(cur_row, cur_col) -
			DoGs[layer_index - 1].at<double>(cur_row, cur_col)) / 2;
		//Vec3f dD(dx, dy, ds);
		dD[0] = dx;
		dD[1] = dy;
		dD[2] = ds;

		// second partial derivative (3 * 3 hessian matrix)
		dxx = DoGs[layer_index].at<double>(cur_row, cur_col + 1) +
			DoGs[layer_index].at<double>(cur_row, cur_col - 1) -
			DoGs[layer_index].at<double>(cur_row, cur_col) * 2;
		dyy = DoGs[layer_index].at<double>(cur_row + 1, cur_col) +
			DoGs[layer_index].at<double>(cur_row - 1, cur_col) -
			DoGs[layer_index].at<double>(cur_row, cur_col) * 2;
		dss = DoGs[layer_index + 1].at<double>(cur_row, cur_col) +
			DoGs[layer_index - 1].at<double>(cur_row, cur_col) -
			DoGs[layer_index].at<double>(cur_row, cur_col) * 2;
		dxy = (DoGs[layer_index].at<double>(cur_row + 1, cur_col + 1) -
			DoGs[layer_index].at<double>(cur_row + 1, cur_col - 1) -
			DoGs[layer_index].at<double>(cur_row - 1, cur_col + 1) +
			DoGs[layer_index].at<double>(cur_row - 1, cur_col - 1)) / 4;
		dxs = (DoGs[layer_index + 1].at<double>(cur_row, cur_col + 1) -
			DoGs[layer_index + 1].at<double>(cur_row, cur_col - 1) -
			DoGs[layer_index - 1].at<double>(cur_row, cur_col + 1) +
			DoGs[layer_index - 1].at<double>(cur_row, cur_col - 1)) / 4;
		dys = (DoGs[layer_index + 1].at<double>(cur_row + 1, cur_col) -
			DoGs[layer_index + 1].at<double>(cur_row - 1, cur_col) -
			DoGs[layer_index - 1].at<double>(cur_row + 1, cur_col) +
			DoGs[layer_index - 1].at<double>(cur_row - 1, cur_col)) / 4;
		Matx33f hessian(dxx, dxy, dxs,
			dxy, dyy, dys,
			dxs, dys, dss);

		// the offset is the inverse of second derivative multiply by first derivative (Lowe's equation 3)
		Matx33f inv_hessian(hessian.inv(DECOMP_SVD));
		offset = -inv_hessian * dD;

		offset_c = offset[0];
		offset_r = offset[1];
		offset_i = offset[2];

		// if the offset is smaller than 0.5 in any dimension, return the offset
		if (abs(offset_c) < 0.5 && abs(offset_r) < 0.5 && abs(offset_i) < 0.5)
			break;

		// if the offset is larger than 0.5 in any dimension, 
		// then it means that the extremum lies closer to a different sample point
		cur_col += cvRound(offset_c);
		cur_row += cvRound(offset_r);
		cur_interval += cvRound(offset_i);

		// if the point is over the border, discards it
		if (cur_interval < 0 || cur_interval > scale - 1 ||
			cur_col < SIFT_IMG_BORDER || cur_row < SIFT_IMG_BORDER ||
			cur_col >= DoGs[octave * (scale + 2) + cur_interval + 1].cols - SIFT_IMG_BORDER ||
			cur_row >= DoGs[octave * (scale + 2) + cur_interval + 1].rows - SIFT_IMG_BORDER)
			return NULL;

		i++;
	}

	// can not find keypoint within the iteration
	if (i >= SIFT_MAX_INTERP_STEPS)
		return NULL;

	// rejecting unstable extrema with low contrast
	// all extrema with a value of |D(x)| less than the threshold (0.03 in Lowe's paper) were discarded
	layer_index = octave * (scale + 2) + cur_interval + 1;
	double Dx = DoGs[layer_index].at<double>(cur_row, cur_col) + dD.dot(offset) / 2;
	if (abs(Dx) < SIFT_CONTR_THR)
		return NULL;

	if (!isEdge(octave, cur_interval, cur_row, cur_col)) {
		// the keypoint's actual coordinate in the image
		double x = (cur_col + offset_c);
		double y = (cur_row + offset_r);

		key_point kp = { x, y, octave, cur_interval, 0.0, NULL };

		return &kp;
	}
	else
		return NULL;
}

// eliminate edge responses
bool SIFT::isEdge(int octave, int interval, int row, int column) {
	int layer_index = octave * (scale + 2) + interval + 1;

	// the principal curvatures can be computed from a 2 * 2 hessian matrix
	double dxx = DoGs[layer_index].at<double>(row, column + 1) +
		DoGs[layer_index].at<double>(row, column - 1) -
		DoGs[layer_index].at<double>(row, column) * 2;
	double dyy = DoGs[layer_index].at<double>(row + 1, column) +
		DoGs[layer_index].at<double>(row - 1, column) -
		DoGs[layer_index].at<double>(row, column) * 2;
	double dxy = (DoGs[layer_index].at<double>(row + 1, column + 1) -
		DoGs[layer_index].at<double>(row + 1, column - 1) -
		DoGs[layer_index].at<double>(row - 1, column + 1) +
		DoGs[layer_index].at<double>(row - 1, column - 1)) / 4;

	double trace = dxx + dyy;
	double det = dxx * dyy - dxy * dxy;

	//if the curvatures have different signs, the point is discarded as not being an extremum
	if (det <= 0)
		return true;

	// eliminates keypoints that have a ratio between the principal curvatures greater than the threshold (10 in Lowe's paper)
	if (trace * trace / det >= (SIFT_CURV_THR + 1) * (SIFT_CURV_THR + 1) / SIFT_CURV_THR)
		return true;

	return false;
}


void SIFT::GetOriAndMag(int oct_id, int scale_id)
{
	double PI=3.14159265358;
	Mat origin= gausPyr[oct_id*6+scale_id];
	int w = origin.cols; 
	int h = origin.rows;
	mag_Pyramid[oct_id*6+scale_id].create(h,w,CV_64FC1);
	ori_Pyramid[oct_id*6+scale_id].create(h,w,CV_64FC1);
	
	Mat mag = mag_Pyramid[oct_id*6+scale_id];
	Mat ori = ori_Pyramid[oct_id*6+scale_id];
	
	for(int y=0;y<h;y++)
	{
		((double*)mag.data)[y*w]=0;
		((double*)ori.data)[y*w]=PI;

		for(int x=1;x<w-1;x++)
		{
			if(y>0 && y<h-1)
			{
				double dy=((double*)origin.data)[(y+1)*w+x]-((double*)origin.data)[(y-1)*w+x];
				double dx=((double*)origin.data)[y*w+x+1]-((double*)origin.data)[y*w+x-1];
				((double*)mag.data)[y*w+x]=sqrt(dx*dx+dy*dy);
				((double*)ori.data)[y*w+x]= (dy==0 && dx==0) ? 0:atan2(dy,dx);
				if(((double*)ori.data)[y*w+x]<0) ((double*)ori.data)[y*w+x] += 2*PI;
			}
			else
			{
				((double*)mag.data)[y*w+x]=0;
				((double*)ori.data)[y*w+x]=PI;
			}
		}

		((double*)mag.data)[y*w+w-1]=0;
		((double*)ori.data)[y*w+w-1]=PI;
	}
}


std::vector<double> SIFT::OrientationAssignment(key_point p)
{
	Mat ori_img=ori_Pyramid[p.oct_id*6+p.scale_id];
	Mat mag_img=mag_Pyramid[p.oct_id*6+p.scale_id];
	double PI=3.14159265358;
	int r=cvRound(this->sigma[p.oct_id*6+p.scale_id] *4.5);
	double hist[36];
	for(int i=0;i<36;i++) hist[i]=0;

	for(int xx=-r;xx<=r;xx++)
	{
		int x=p.x+xx;
		if(x<=0 || x>=ori_img.cols-1) continue;
		for(int yy=-r;yy<=r;yy++)
		{
			int y=p.y+yy;
			if(y<=0 || y>=ori_img.rows-1) continue;
			if(sqr(xx)+sqr(yy)>sqr(r)) continue;
			double ori=ori_img.at<double>(y,x);
			int bin=cvRound(ori/PI*180.0/10.0);
			if (bin==36) bin=0;
			hist[bin]+=mag_img.at<double>(y,x)*exp( -(sqr(xx)+sqr(yy)) / (2*sqr(1.5*this->sigma[p.oct_id * 6 + p.scale_id])) );

		}
	}

	for(int i=0;i<2;i++)
	{
		double hist_tmp[36];
		for(int j=0;j<36;j++)
		{
			double prev=hist[j==0? 35:j-1];
			double next=hist[j==35? 0:j+1];
			hist_tmp[j]=hist[j]*0.5+(prev+next)*0.25;
		}
		for(int j=0;j<36;j++)
		{
			hist[j]=hist_tmp[j];
		}
	}

	double hist_max=0;
	for(int i=0;i<36;i++)
	{
		if(hist[i]>hist_max) hist_max=hist[i];
	}
	double threshold=0.8*hist_max;
	std::vector<double> result;

	for(int i=0;i<36;i++)
	{
		double prev=hist[i==0? 35:i-1];
		double next=hist[i==35? 0:i+1];

		if(hist[i]>threshold && hist[i]>prev && hist[i]>next)
		{
			double a=(next+prev-2*hist[i])*0.5;
			double b=(next-prev)*0.5;
			
			double bin_real = i*1.0 - 0.5*b/a;

			if(bin_real<0) bin_real+=36;

			double orientation = bin_real * 10.0 / 180.0 * PI;
			result.push_back(orientation);
		}
	}
	return result;
}


void SIFT::GetVector(key_point p)
{
	double PI=3.14159265358;
	Mat mag_img=mag_Pyramid[p.oct_id*6+p.scale_id];
	Mat ori_img=ori_Pyramid[p.oct_id*6+p.scale_id];
	int w=ori_img.cols, h=ori_img.rows;

	double ori=p.orientation;
	int r= cvRound(sqrt(0.5)*3*this->sigma[p.oct_id * 6 + p.scale_id]*(4+1));
	double hist[16][8];
	for(int i=0;i<16;i++)
		for(int j=0;j<8;j++)
			hist[i][j]=0;

	double cos_part= cos(ori), sin_part= sin(ori);

	for(int xx=-r;xx<=r;xx++)
	{
		int x=p.x+xx;
		if(x<=0 || x>=w-1) continue;
		for(int yy=-r;yy<=r;yy++)
		{
			int y=p.y+yy;
			if(y<=0 || y>=h-1) continue;
			if(sqr(xx)+sqr(yy)>sqr(r)) continue;

			double bin_y = (-xx*sin_part+yy*cos_part)/(3.0*this->sigma[p.oct_id * 6 + p.scale_id])+1.5;
			double bin_x = (xx*cos_part +yy*sin_part)/(3.0*this->sigma[p.oct_id * 6 + p.scale_id])+1.5;

			if(bin_y<-1 || bin_y>=4 || bin_x<-1 || bin_x>=4) continue;

			double ori_loc = ori_img.at<double>(y,x);
			double mag_loc = mag_img.at<double>(y,x);

			double w_mag = mag_loc * exp( -(sqr(xx)+sqr(yy)) / 8 );
			double r_ori = ori_loc -  ori;

			if(r_ori<0) r_ori += 2*PI;

			double bin_hist = r_ori /PI * 180.0 /45.0;

			TriInterpolation(bin_x,bin_y,bin_hist,w_mag,hist);
 		}
	}
	std::vector<double> result(128);
	for(int i=0;i<16;i++)
		for(int j=0;j<8;j++)
			result[8*i+j]=hist[i][j];

	double sum=0;
	for(int i=0;i<128;i++) sum+=sqr(result[i]);
	sum=sqrt(sum);
    for(int i=0;i<128;i++)
    {
    	result[i]/=sum;
    	if(result[i]>0.2) result[i]=0.2;
    }
    sum=0;
    for(int i=0;i<128;i++) sum+=sqr(result[i]);
	sum=sqrt(sum);
    for(int i=0;i<128;i++) result[i]/=sum;

	p.v = new std::vector<double>;
	for (int i = 0; i < 128; i++)
		p.v->push_back(result[i]);
}


void SIFT::TriInterpolation(double x, double y, double h, double w_mag, double hist[][8])
{
	int xf=floor(x),
	    yf=floor(y),
	    hf=floor(h);

	double delta_x = x-xf,
	       delta_y = y-yf,
	       delta_h = h-hf;

	for(int i=0;i<2;i++)
	{
		if((yf+i)>=0 && (yf+i)<4)
		{
			double wy = w_mag * (i? delta_y : 1-delta_y);
			for(int j=0;j<2;j++)
			{
				if((xf+j)>=0 && (xf+j)<4)
				{
					double wx = wy * (j? delta_x : 1-delta_x);
					hist[4*(yf+i)+(xf+j)][hf % 8] += wx * (1-delta_h);
					hist[4*(yf+i)+(xf+j)][(hf+1) % 8] += wx * delta_h;
				}
			}
		}
	}
}

double SIFT::sqr(double a)
{
	return a*a;
}

